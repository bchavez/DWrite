// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <unknwn.h>
#include <atlbase.h>
#include <d2d1.h>
#include <wincodec.h>
#include <detours.h>
#include "brute_cast.h"


class Detour : public ID2D1RenderTarget
{
public:
  static void (WINAPI Detour::* TrueDrawGlyphRun)(
    D2D1_POINT_2F baselineOrigin,
    const DWRITE_GLYPH_RUN *glyphRun,
    ID2D1Brush *foregroundBrush,
    DWRITE_MEASURING_MODE measuringMode);

  void WINAPI FakeDrawGlyphRun(
    D2D1_POINT_2F baselineOrigin,
    const DWRITE_GLYPH_RUN *glyphRun,
    ID2D1Brush *foregroundBrush,
    DWRITE_MEASURING_MODE measuringMode);
};
