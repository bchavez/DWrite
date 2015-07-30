// dwrite.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "dwrite.h"

#define INFO_BUFFER_SIZE 32767

struct Params {
  D2D1_TEXT_ANTIALIAS_MODE AntialiasMode;
  IDWriteRenderingParams *RenderingParams;

  FLOAT ClearTypeLevel;
  FLOAT EnhancedContrast;
  FLOAT Gamma;
  DWRITE_RENDERING_MODE RenderingMode;
};

typedef HRESULT (WINAPI *pDWCF)(DWRITE_FACTORY_TYPE, REFIID, IUnknown**);
pDWCF wrappedMethod;

int Threshold = 16;
Params Small = {D2D1_TEXT_ANTIALIAS_MODE_DEFAULT, NULL,
                0.5f, 0.5f, 1.8f, DWRITE_RENDERING_MODE_DEFAULT};
Params Large = {D2D1_TEXT_ANTIALIAS_MODE_DEFAULT, NULL,
                0.5f, 0.5f, 1.8f, DWRITE_RENDERING_MODE_DEFAULT};

void Update();
bool Initialized = FALSE;

#ifdef DEBUG
#define DBG WriteDebugInfo
VOID WINAPI WriteDebugInfo(const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  vprintf(format, ap);
  fflush(NULL);
  va_end(ap);
}
#else
#define DBG __noop
#endif

//////////////////////////////////////////////////////////////////////////////

void (WINAPI Detour::* Detour::TrueDrawGlyphRun)(
  D2D1_POINT_2F baselineOrigin,
  const DWRITE_GLYPH_RUN *glyphRun,
  ID2D1Brush *foregroundBrush,
  DWRITE_MEASURING_MODE measuringMode = DWRITE_MEASURING_MODE_NATURAL) = NULL;

void WINAPI Detour::FakeDrawGlyphRun(
  D2D1_POINT_2F baselineOrigin,
  const DWRITE_GLYPH_RUN *glyphRun,
  ID2D1Brush *foregroundBrush,
  DWRITE_MEASURING_MODE measuringMode = DWRITE_MEASURING_MODE_NATURAL)
{
  // DBG("#   FakeDrawGlyphRun: START fontEmSize=%f\n", glyphRun->fontEmSize);
  Params *font;
  font = (glyphRun->fontEmSize < Threshold) ? &Small : &Large;

  this->SetTextRenderingParams(font->RenderingParams);
  if (font->AntialiasMode < 0) {
    this->SetTextAntialiasMode(this->GetTextAntialiasMode());
  } else {
    this->SetTextAntialiasMode(font->AntialiasMode);
  }

  (this->*TrueDrawGlyphRun)(baselineOrigin,
                            glyphRun,
                            foregroundBrush,
                            DWRITE_MEASURING_MODE_GDI_CLASSIC);
  // DBG("#   FakeDrawGlyphRun: END\n");
}

//////////////////////////////////////////////////////////////////////////////

typedef HRESULT (WINAPI * __D2D1CreateFactory)(
  D2D1_FACTORY_TYPE factoryType,
  REFIID riid,
  const D2D1_FACTORY_OPTIONS *pFactoryOptions,
  void **ppIFactory);

void Initialize()
{
  DBG("! Initialize:\n");
  if (Initialized) {
    return;
  }
  CoInitialize(NULL);

    TCHAR infoBuf[INFO_BUFFER_SIZE];
    UINT result;
    result = GetSystemDirectory(infoBuf, INFO_BUFFER_SIZE);
    // TODO: check return value
    TCHAR fullPath[INFO_BUFFER_SIZE];
    swprintf_s(fullPath, INFO_BUFFER_SIZE, L"%s\\DWrite.dll", infoBuf);
    // TODO: check return value
    HMODULE library = LoadLibrary(fullPath);
    // TODO: check return value
    wrappedMethod = (pDWCF) GetProcAddress(library, "DWriteCreateFactory");

  //
  __D2D1CreateFactory _D2D1CreateFactory = (__D2D1CreateFactory)
    GetProcAddress(LoadLibraryW(L"d2d1.dll"), "D2D1CreateFactory");
  if (!_D2D1CreateFactory) {
    return;
  }

  CComPtr<ID2D1Factory> d2d_factory;
  _D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                     __uuidof(ID2D1Factory), NULL, (void**)&d2d_factory);

  //
  CComPtr<IWICImagingFactory> factory;
  factory.CoCreateInstance(CLSID_WICImagingFactory);
  CComPtr<IWICBitmap> bitmap;
  factory->CreateBitmap(32,
                        32,
                        GUID_WICPixelFormat32bppPBGRA,
                        WICBitmapCacheOnLoad,
                        &bitmap);
  const D2D1_PIXEL_FORMAT format =
    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                      D2D1_ALPHA_MODE_PREMULTIPLIED);
  const D2D1_RENDER_TARGET_PROPERTIES properties =
    D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
                                 format,
                                 0.0f,
                                 0.0f,
                                 D2D1_RENDER_TARGET_USAGE_NONE);
  CComPtr<ID2D1RenderTarget>target;
  d2d_factory->CreateWicBitmapRenderTarget(bitmap,
                                           properties,
                                           &target);

  //
  Detour::TrueDrawGlyphRun = brute_cast<void(WINAPI Detour::*)(
    D2D1_POINT_2F,
    const DWRITE_GLYPH_RUN *,
    ID2D1Brush *,
    DWRITE_MEASURING_MODE)>(
      (*reinterpret_cast<void***>(target.p))[29]);

  DetourTransactionBegin();
  DetourUpdateThread(GetCurrentThread());
  DetourAttach(&(PVOID&)Detour::TrueDrawGlyphRun,
               (PVOID)(&(PVOID&)Detour::FakeDrawGlyphRun));
  DetourTransactionCommit();

  Initialized = TRUE;
  Update();
  DBG("! Initialize: END\n");
  return;
}

void PrefsToParams(
    FLOAT enhancedContrast,
    FLOAT clearTypeLevel,
    DWRITE_RENDERING_MODE renderingMode,
    D2D1_TEXT_ANTIALIAS_MODE antiAliasMode,
    Params *aParams)
{
  DBG("PrefsToParams:\n");
  //
  CComPtr<IDWriteFactory> dw_factory;
  wrappedMethod(DWRITE_FACTORY_TYPE_SHARED,
                      __uuidof(IDWriteFactory),
                      (IUnknown**)&dw_factory);
  CComPtr<IDWriteRenderingParams> dwrpm;
  dw_factory->CreateRenderingParams(&dwrpm);

  //
  aParams->EnhancedContrast = enhancedContrast;

  aParams->ClearTypeLevel = clearTypeLevel;

  aParams->RenderingMode = renderingMode;

  aParams->AntialiasMode = antiAliasMode;

  if (aParams->RenderingParams) {
    aParams->RenderingParams->Release();
  }

  dw_factory->CreateCustomRenderingParams(
    dwrpm->GetGamma(),
    aParams->EnhancedContrast,
    aParams->ClearTypeLevel,
    dwrpm->GetPixelGeometry(),
    aParams->RenderingMode,
    &aParams->RenderingParams);

  DBG("PrefsToParams: %f %f %x %x\n",
      aParams->EnhancedContrast, aParams->ClearTypeLevel,
      aParams->RenderingMode, aParams->AntialiasMode);
}

void Update()
{
    // TODO: Read the parameters from the registry
    PrefsToParams(0.0f, 0.0f, DWRITE_RENDERING_MODE_ALIASED, D2D1_TEXT_ANTIALIAS_MODE_DEFAULT, &Small);
    PrefsToParams(0.0f, 0.0f, DWRITE_RENDERING_MODE_ALIASED, D2D1_TEXT_ANTIALIAS_MODE_DEFAULT, &Large);
}

void Finalize()
{
  DBG("! Finalize:\n");
  if (!Initialized) {
    return;
  }

  DetourTransactionBegin();
  DetourUpdateThread(GetCurrentThread());
  DetourDetach(&(PVOID&)Detour::TrueDrawGlyphRun,
               (PVOID)(&(PVOID&)Detour::FakeDrawGlyphRun));
  DetourTransactionCommit();

  CoUninitialize();

  Initialized = FALSE;
  DBG("! Finalize: END\n");
  return;
}

static HMODULE s_hDll;
HMODULE WINAPI Detoured() {
  return s_hDll;
}

DWRITE_API HRESULT DWriteCreateFactory(DWRITE_FACTORY_TYPE factoryType, REFIID iid, IUnknown **factory)
{
    Initialize();
    HRESULT hr = wrappedMethod(factoryType, iid, factory);
    // TODO: check hr
    return hr;
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DBG("DLL_PROCESS_ATTACH: START\n");
        s_hDll = hModule;
        DBG("DLL_PROCESS_ATTACH: END\n");
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        DBG("DLL_PROCESS_DETACH: START\n");
        Finalize();
        DBG("DLL_PROCESS_DETACH: END\n");
        break;
    }
    return TRUE;
}
