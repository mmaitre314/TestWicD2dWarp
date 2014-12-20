#pragma once

#include "targetver.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <collection.h>
#include <ppltasks.h>
#include <vector>
#include <time.h>
#include <wrl.h>
#include <wincodec.h>
#include <D3D11.h>
#include <D2d1_1.h>
#include <d2d1effects.h>
#include <shcore.h>
#include <evntprov.h>
#include <evntrace.h>
#include <windows.storage.h>
#include <windows.storage.streams.h>

//
// Error handling
//

inline void CHK(_In_ HRESULT hr)
{
    if (FAILED(hr))
    {
        throw ref new Platform::COMException(hr);
    }
}

// 
// Casts
// 

template<typename T, typename U>
Microsoft::WRL::ComPtr<T> As(const Microsoft::WRL::ComPtr<U>& in)
{
    Microsoft::WRL::ComPtr<T> out;
    CHK(in.As(&out));
    return out;
}

template<typename T, typename U>
Microsoft::WRL::ComPtr<T> As(U* in)
{
    Microsoft::WRL::ComPtr<T> out;
    CHK(in->QueryInterface(IID_PPV_ARGS(&out)));
    return out;
}

template<typename T, typename U>
Microsoft::WRL::ComPtr<T> As(U^ in)
{
    Microsoft::WRL::ComPtr<T> out;
    CHK(reinterpret_cast<IInspectable*>(in)->QueryInterface(IID_PPV_ARGS(&out)));
    return out;
}

__declspec(selectany) REGHANDLE g_ETWHandle = 0;
__declspec(selectany) BOOL g_bEnabled = false;
__declspec(selectany) UCHAR g_nLevel = 0;

// Not store compliant
extern "C" {
    ULONG EVNTAPI EventWriteString(
        _In_ REGHANDLE RegHandle,
        _In_ UCHAR Level,
        _In_ ULONGLONG Keyword,
        _In_ PCWSTR String
        );
}
