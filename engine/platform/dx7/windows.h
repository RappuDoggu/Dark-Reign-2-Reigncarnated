#pragma once
// ABI declarations used by the original DX7 headers. No Windows service is
// implemented here: the native renderer must supply the actual device objects.
#include <cstdint>
#include <cstddef>
#define WINAPI
#define CALLBACK
#define PASCAL
#define FAR
#define NEAR
#define APIENTRY
#define REIGN_DX7_DECLARATIONS 1
using BYTE=std::uint8_t;
using WORD=std::uint16_t;
using DWORD=std::uint32_t;
using LONG=std::int32_t;
using ULONG=std::uint32_t;
using UINT=std::uint32_t;
using INT=std::int32_t;
using BOOL=std::int32_t;
using HRESULT=std::int32_t;
using FLOAT=float;
using WCHAR=std::uint16_t;
using HANDLE=void *;
using HWND=void *;
using HDC=void *;
using HMONITOR=void *;
using HINSTANCE=void *;
using LPVOID=void *;
using LPCVOID=const void *;
using LPSTR=char *;
using LPCSTR=const char *;
using LPWSTR=WCHAR *;
using LPCWSTR=const WCHAR *;
using LPDWORD=DWORD *;
using LPWORD=WORD *;
using LPBOOL=BOOL *;
using LPBYTE=BYTE *;
using LPLONG=LONG *;
struct GUID { DWORD Data1; WORD Data2,Data3; BYTE Data4[8]; };
using LPGUID=GUID *;
using REFGUID=const GUID &;
using REFIID=const GUID &;
struct RECT { LONG left,top,right,bottom; };
using LPRECT=RECT *;
struct POINT { LONG x,y; };
using LPPOINT=POINT *;
union LARGE_INTEGER { struct { DWORD LowPart; LONG HighPart; }; std::int64_t QuadPart; };
struct PALETTEENTRY { BYTE peRed,peGreen,peBlue,peFlags; };
using LPPALETTEENTRY=PALETTEENTRY *;
struct RGNDATA;
using LPRGNDATA=RGNDATA *;
#define MAKE_HRESULT(severity,facility,code) static_cast<HRESULT>((DWORD(severity)<<31)|(DWORD(facility)<<16)|DWORD(code))
#define S_OK HRESULT(0)
#define E_FAIL HRESULT(0x80004005u)
#define E_NOINTERFACE HRESULT(0x80004002u)
#define E_NOTIMPL HRESULT(0x80004001u)
#define E_INVALIDARG HRESULT(0x80070057u)
#define FAILED(hr) ((hr)<0)
#define SUCCEEDED(hr) ((hr)>=0)
#define MAX_PATH 260
static_assert(sizeof(DWORD)==4 && sizeof(LONG)==4 && sizeof(HRESULT)==4);
#define DECLARE_HANDLE(name) using name=void *
using REFCLSID=const GUID &;
struct SIZE { LONG cx,cy; };
using LPSIZE=SIZE *;

#define _fastcall
struct RGBQUAD { BYTE rgbBlue,rgbGreen,rgbRed,rgbReserved; };
#define _cdecl
using WPARAM=std::uintptr_t;
using LPARAM=std::intptr_t;
using LRESULT=std::intptr_t;
struct LOGFONT;
