#include "pch.h"
#include "Test.h"

using namespace concurrency;
using namespace Microsoft::WRL;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;

void Test::RunYUV()
{
    //
    // !!! Code not working !!!
    //

    // 5Mpx
    const unsigned int width = 2592;
    const unsigned int height = 1936;

    //
    // Setup WIC
    //

    ComPtr<IWICImagingFactory> wicFactory;
    CHK(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory)));

    // Y source
    ComPtr<IWICBitmap> wicBitmapSrc;
    CHK(wicFactory->CreateBitmap(width, height, GUID_WICPixelFormat8bppAlpha, WICBitmapCacheOnLoad, &wicBitmapSrc));

    // Y destination
    ComPtr<IWICBitmap> wicBitmapDst;
    CHK(wicFactory->CreateBitmap(width, height, GUID_WICPixelFormat8bppAlpha, WICBitmapCacheOnLoad, &wicBitmapDst));

    //
    // Fill-in the source bitmap with some pattern
    //

    {
        WICRect rect = { 0, 0, width, height };
        ComPtr<IWICBitmapLock> wicBitmapLockSrc;
        CHK(wicBitmapDst->Lock(&rect, WICBitmapLockWrite, &wicBitmapLockSrc));

        unsigned char *data = nullptr;
        unsigned int size;
        unsigned int stride;
        CHK(wicBitmapLockSrc->GetDataPointer(&size, &data));
        CHK(wicBitmapLockSrc->GetStride(&stride));

        for (int i = 0; i < (int)height; i++)
        {
            for (int j = 0; j < (int)width; j++)
            {
                data[j] = 255 * ((i / 100) % 2) * ((j / 100) % 2); // 100x100 squares
            }
            data += stride;
        }
    }

    //
    // Setup DX
    //

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };
    D3D_FEATURE_LEVEL featureLevel;

    ComPtr<ID3D11Device> d3dDevice;
    ComPtr<ID3D11DeviceContext> d3dContext;
    CHK(D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_WARP, // D3D_DRIVER_TYPE_HARDWARE
        0,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT, // D3D11_CREATE_DEVICE_DEBUG
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &d3dDevice,
        &featureLevel,
        &d3dContext
        ));

    //
    // Setup D2D
    //

    ComPtr<ID2D1Factory1> d2dFactory;
    CHK(D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, IID_PPV_ARGS(&d2dFactory)));

    ComPtr<ID2D1Device> d2dDevice;
    ComPtr<ID2D1DeviceContext> d2dContext;
    CHK(d2dFactory->CreateDevice(As<IDXGIDevice>(d3dDevice).Get(), &d2dDevice));
    CHK(d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS, &d2dContext));

    D2D1_BITMAP_PROPERTIES1 d2dBitmapSrcProps =
    {
        {
            DXGI_FORMAT_A8_UNORM,
            D2D1_ALPHA_MODE_STRAIGHT
        },
        0.f,
        0.f,
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        nullptr
    };
    ComPtr<ID2D1Bitmap1> d2dBitmapDst;
    CHK(d2dContext->CreateBitmapFromWicBitmap(wicBitmapDst.Get(), d2dBitmapSrcProps, &d2dBitmapDst));
    d2dContext->SetTarget(d2dBitmapDst.Get());

    ComPtr<ID2D1Bitmap> d2dBitmapSrc;
    {
        WICRect rect = { 0, 0, width, height };
        ComPtr<IWICBitmapLock> wicBitmapLockSrc;
        CHK(wicBitmapSrc->Lock(&rect, WICBitmapLockRead, &wicBitmapLockSrc));

        D2D1_BITMAP_PROPERTIES d2dBitmapProps =
        {
            {
                DXGI_FORMAT_A8_UNORM,
                D2D1_ALPHA_MODE_STRAIGHT
            },
            0.f,
            0.f
        };
        CHK(d2dContext->CreateSharedBitmap(__uuidof(wicBitmapLockSrc), wicBitmapLockSrc.Get(), &d2dBitmapProps, &d2dBitmapSrc));
    }

    //
    // Render destination bitmap
    //

    d2dContext->BeginDraw();
    d2dContext->SetTransform(D2D1::Matrix3x2F::Identity());
    d2dContext->DrawImage(d2dBitmapSrc.Get());
    CHK(d2dContext->EndDraw());

    //
    // Encode to JPEG
    //

    create_task(KnownFolders::PicturesLibrary->CreateFileAsync(L"TestWicD2dWarpYUV.jpg", CreationCollisionOption::ReplaceExisting)).then([](StorageFile^ file)
    {
        return file->OpenAsync(FileAccessMode::ReadWrite);
    }).then([wicFactory, wicBitmapDst, width, height](IRandomAccessStream^ stream)
    {
        ComPtr<IWICBitmapEncoder> wicEncoder;
        CHK(wicFactory->CreateEncoder(GUID_ContainerFormatJpeg, nullptr, &wicEncoder));

        ComPtr<IStream> wrappedStream;
        CHK(CreateStreamOverRandomAccessStream(As<IUnknown>(stream).Get(), IID_PPV_ARGS(&wrappedStream)));

        CHK(wicEncoder->Initialize(wrappedStream.Get(), WICBitmapEncoderNoCache));

        WICRect rect = { 0, 0, width, height };
        ComPtr<IWICBitmapLock> wicBitmapLockDst;
        CHK(wicBitmapDst->Lock(&rect, WICBitmapLockRead, &wicBitmapLockDst));

        unsigned char *data = nullptr;
        unsigned int size;
        unsigned int stride;
        CHK(wicBitmapLockDst->GetDataPointer(&size, &data));
        CHK(wicBitmapLockDst->GetStride(&stride));

        std::vector<unsigned char> planeUV(stride * height / 2, 0); // Green UV plane

        WICBitmapPlane planes[2] = {};
        planes[0].Format = GUID_WICPixelFormat8bppY;
        planes[0].pbBuffer = data;
        planes[0].cbBufferSize = stride * height;
        planes[0].cbStride = stride;
        planes[1].Format = GUID_WICPixelFormat16bppCbCr;
        planes[1].pbBuffer = planeUV.data();
        planes[1].cbBufferSize = planeUV.size();
        planes[1].cbStride = stride;

        ComPtr<IWICBitmapFrameEncode> wicFrame;
        WICPixelFormatGUID wicFormat = GUID_WICPixelFormat24bppBGR;
        CHK(wicEncoder->CreateNewFrame(&wicFrame, nullptr));
        CHK(wicFrame->Initialize(nullptr));
        CHK(wicFrame->SetPixelFormat(&wicFormat));
        CHK(wicFrame->SetSize(width, height));

        CHK(As<IWICPlanarBitmapFrameEncode>(wicFrame)->WritePixels(height, planes, 2));
        CHK(wicFrame->Commit());
        CHK(wicEncoder->Commit());
    });
}

void Test::RunRGB()
{
    // 5Mpx
    const unsigned int width = 2592;
    const unsigned int height = 1936;

    //
    // Setup WIC
    //

    ComPtr<IWICImagingFactory> wicFactory;
    CHK(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory)));

    (void)EventWriteString(g_ETWHandle, TRACE_LEVEL_INFORMATION, 0, L"Initializing source bitmap");

    // source
    ComPtr<IWICBitmap> wicBitmapSrc;
    CHK(wicFactory->CreateBitmap(width, height, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad, &wicBitmapSrc));

    //
    // Fill-in the source bitmap with some pattern
    //

    {
        WICRect rect = { 0, 0, width, height };
        ComPtr<IWICBitmapLock> wicBitmapLockSrc;
        CHK(wicBitmapSrc->Lock(&rect, WICBitmapLockWrite, &wicBitmapLockSrc));

        unsigned char *data = nullptr;
        unsigned int size;
        unsigned int stride;
        CHK(wicBitmapLockSrc->GetDataPointer(&size, &data));
        CHK(wicBitmapLockSrc->GetStride(&stride));

        for (int i = 0; i < (int)height; i++)
        {
            for (int j = 0; j < (int)width; j++)
            {
                unsigned char value = 255 * ((i / 100) % 2) * ((j / 100) % 2); // 100x100 squares
                data[4 * j + 0] = value;
                data[4 * j + 1] = value;
                data[4 * j + 2] = value;
                data[4 * j + 3] = 0xFF;
            }
            data += stride;
        }
    }

    (void)EventWriteString(g_ETWHandle, TRACE_LEVEL_INFORMATION, 0, L"Setup DX");

    //
    // Setup DX
    //

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };
    D3D_FEATURE_LEVEL featureLevel;

    ComPtr<ID3D11Device> d3dDevice;
    ComPtr<ID3D11DeviceContext> d3dContext;
    CHK(D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_WARP, // D3D_DRIVER_TYPE_HARDWARE
        0,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &d3dDevice,
        &featureLevel,
        &d3dContext
        ));

    //
    // Setup D2D
    //

    (void)EventWriteString(g_ETWHandle, TRACE_LEVEL_INFORMATION, 0, L"Setup D2D");

    ComPtr<ID2D1Factory1> d2dFactory;
    CHK(D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, IID_PPV_ARGS(&d2dFactory)));

    ComPtr<ID2D1Device> d2dDevice;
    ComPtr<ID2D1DeviceContext> d2dContext;
    CHK(d2dFactory->CreateDevice(As<IDXGIDevice>(d3dDevice).Get(), &d2dDevice));
    CHK(d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2dContext));

    D2D1_BITMAP_PROPERTIES1 d2dBitmapDstProps =
    {
        {
            DXGI_FORMAT_B8G8R8A8_UNORM,
            D2D1_ALPHA_MODE_PREMULTIPLIED
        },
        0.f,
        0.f,
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        nullptr
    };
    ComPtr<ID2D1Bitmap1> d2dBitmapDst;
    CHK(d2dContext->CreateBitmap(D2D1::SizeU(width, height), nullptr, 0, d2dBitmapDstProps, &d2dBitmapDst));
    d2dContext->SetTarget(d2dBitmapDst.Get());

    ComPtr<ID2D1Bitmap> d2dBitmapSrc;
    {
        WICRect rect = { 0, 0, width, height };
        ComPtr<IWICBitmapLock> wicBitmapLockSrc;
        CHK(wicBitmapSrc->Lock(&rect, WICBitmapLockRead, &wicBitmapLockSrc));

        D2D1_BITMAP_PROPERTIES d2dBitmapProps =
        {
            {
                DXGI_FORMAT_B8G8R8A8_UNORM,
                D2D1_ALPHA_MODE_IGNORE
            },
            0.f,
            0.f
        };
        CHK(d2dContext->CreateSharedBitmap(__uuidof(wicBitmapLockSrc), wicBitmapLockSrc.Get(), &d2dBitmapProps, &d2dBitmapSrc));
    }

    ComPtr<ID2D1Effect> perspective;
    d2dContext->CreateEffect(CLSID_D2D13DPerspectiveTransform, &perspective);
    perspective->SetInput(0, d2dBitmapSrc.Get());
    CHK(perspective->SetValue(D2D1_3DPERSPECTIVETRANSFORM_PROP_ROTATION, D2D1::Vector3F(10.f, 10.f, 0.f)));

    //
    // Render destination bitmap
    //

    (void)EventWriteString(g_ETWHandle, TRACE_LEVEL_INFORMATION, 0, L"Render");

    d2dContext->BeginDraw();
    d2dContext->SetTransform(D2D1::Matrix3x2F::Identity());
    d2dContext->DrawImage(perspective.Get());
    CHK(d2dContext->EndDraw());

    //
    // Get a CPU-accessible bitmap
    //

    (void)EventWriteString(g_ETWHandle, TRACE_LEVEL_INFORMATION, 0, L"Copy to CPU-accessible bitmap");

    D2D1_BITMAP_PROPERTIES1 d2dBitmapDstProps2 =
    {
        {
            DXGI_FORMAT_B8G8R8A8_UNORM,
            D2D1_ALPHA_MODE_PREMULTIPLIED
        },
        0.f,
        0.f,
        D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        nullptr
    };
    ComPtr<ID2D1Bitmap1> d2dBitmapDst2;
    CHK(d2dContext->CreateBitmap(D2D1::SizeU(width, height), nullptr, 0, d2dBitmapDstProps2, &d2dBitmapDst2));
    CHK(d2dBitmapDst2->CopyFromBitmap(nullptr, d2dBitmapDst.Get(), nullptr));

    // Verify CPU access
    D2D1_MAPPED_RECT rect;
    CHK(d2dBitmapDst2->Map(D2D1_MAP_OPTIONS_READ, &rect));
    CHK(d2dBitmapDst2->Unmap());

    //
    // Encode to JPEG
    //

    (void)EventWriteString(g_ETWHandle, TRACE_LEVEL_INFORMATION, 0, L"Saving as JPEG");

    create_task(KnownFolders::PicturesLibrary->CreateFileAsync(L"TestWicD2dWarpRGB.jpg", CreationCollisionOption::ReplaceExisting)).then([](StorageFile^ file)
    {
        return file->OpenAsync(FileAccessMode::ReadWrite);
    }).then([wicFactory, d2dBitmapDst2, d2dDevice, width, height](IRandomAccessStream^ stream)
    {
        ComPtr<IStream> wrappedStream;
        CHK(CreateStreamOverRandomAccessStream(As<IUnknown>(stream).Get(), IID_PPV_ARGS(&wrappedStream)));

        ComPtr<IWICBitmapEncoder> wicEncoder;
        CHK(wicFactory->CreateEncoder(GUID_ContainerFormatJpeg, nullptr, &wicEncoder));
        CHK(wicEncoder->Initialize(wrappedStream.Get(), WICBitmapEncoderNoCache));

        ComPtr<IWICBitmapFrameEncode> wicFrame;
        CHK(wicEncoder->CreateNewFrame(&wicFrame, nullptr));
        CHK(wicFrame->Initialize(nullptr));

        ComPtr<IWICImageEncoder> wicImageEncoder;
        CHK(As<IWICImagingFactory2>(wicFactory)->CreateImageEncoder(d2dDevice.Get(), &wicImageEncoder));
        CHK(wicImageEncoder->WriteFrame(d2dBitmapDst2.Get(), wicFrame.Get(), nullptr));

        CHK(wicFrame->Commit());
        CHK(wicEncoder->Commit());
        CHK(wrappedStream->Commit(STGC_DEFAULT));

        (void)EventWriteString(g_ETWHandle, TRACE_LEVEL_INFORMATION, 0, L"Done");
    });
}
