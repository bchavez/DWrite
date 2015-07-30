// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the DWRITE_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// DWRITE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef DWRITE_EXPORTS
#define DWRITE_API __declspec(dllexport)
#define DWRITE_EXPORT __declspec(dllexport)
#else
#define DWRITE_API __declspec(dllimport) WINAPI
#endif

#include <dwrite.h>
