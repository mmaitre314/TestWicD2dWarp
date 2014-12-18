//
// pch.h
// Header for standard system include files.
//

#pragma once

#include <collection.h>
#include <ppltasks.h>
#include <vector>
#include <wrl.h>
#include <wincodec.h>
#include <D3D11.h>
#include <D2d1_1.h>
#include <d2d1effects.h>
#include <shcore.h>
#include <windows.storage.h>
#include <windows.storage.streams.h>

#include "App.xaml.h"

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
