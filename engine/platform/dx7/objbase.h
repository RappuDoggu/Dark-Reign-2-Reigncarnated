#pragma once
#include "windows.h"
#define STDMETHOD(method) virtual HRESULT method
#define STDMETHOD_(type,method) virtual type method
#define STDMETHODIMP HRESULT
#define STDMETHODIMP_(type) type
#define PURE =0
#define THIS_
#define THIS void
#define DECLARE_INTERFACE_(name,base) struct name : public base
#define DECLARE_INTERFACE(name) struct name
#define DEFINE_GUID(name,a,b,c,d,e,f,g,h,i,j,k) inline constexpr GUID name={a,b,c,{d,e,f,g,h,i,j,k}}
struct IUnknown {
 virtual HRESULT QueryInterface(REFIID,void **)=0;
 virtual ULONG AddRef()=0;
 virtual ULONG Release()=0;
};
using LPUNKNOWN=IUnknown *;
