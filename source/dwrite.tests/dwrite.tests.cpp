// dwrite.tests.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
int _tmain(int argc, _TCHAR* argv[])
{
	HRESULT hr;
	CComPtr<IDWriteFactory> pDWriteFactory_;
	hr = DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(&pDWriteFactory_)
	);
	return 0;
}

