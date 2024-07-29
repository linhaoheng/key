// main.cpp : 定义应用程序的入口点。
//
#include "DemoApp.h"

#define MAX_LOADSTRING 100
//-----------------------------------------------------------------
// 功能：key
// 作者：林豪横
//-----------------------------------------------------------------

/******************************************************************
程序入口
******************************************************************/
int WINAPI WinMain(
    HINSTANCE	/* hInstance */,
    HINSTANCE	/* hPrevInstance */,
    LPSTR		/* lpCmdLine */,
    int			/* nCmdShow */)
{
    // 忽略返回值，因为我们希望即使在 HeapSetInformation(指定的堆启用功能) 不太可能失败的情况下也继续运行
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0); //为指定的堆启用功能。

    if (SUCCEEDED(CoInitialize(NULL)))          //如果成功（初始化当前线程上的 COM 库，并将并发模型标识为单线程单元 (STA) 。）
    {
        {
            DemoApp app;                        //演示应用程序

            if (SUCCEEDED(app.Initialize()))    //如果成功(初始化）
            {
                app.Run();                      //运行消息循环
            }
        }
        CoUninitialize();                       //关闭当前线程上的 COM 库，卸载线程加载的所有 DLL，释放线程维护的任何其他资源，并强制关闭线程上的所有 RPC 连接。
    }

    return 0;
}

//-----------------------------------------------------------------
// 从文件读取位图
/*HRESULT LoadBitmapFromFile(
    ID2D1DeviceContext* IN pRenderTarget,
    IWICImagingFactory2* IN pIWICFactory,
    PCWSTR IN uri,
    UINT OPTIONAL width,
    UINT OPTIONAL height,
    ID2D1Bitmap1** OUT ppBitmap)
{
    IWICBitmapDecoder* pDecoder = nullptr;
    IWICBitmapFrameDecode* pSource = nullptr;
    IWICStream* pStream = nullptr;
    IWICFormatConverter* pConverter = nullptr;
    IWICBitmapScaler* pScaler = nullptr;

    HRESULT hr = pIWICFactory->CreateDecoderFromFilename(
        uri,
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &pDecoder);

    if (SUCCEEDED(hr))
    {
        hr = pDecoder->GetFrame(0, &pSource);
    }
    if (SUCCEEDED(hr))
    {
        hr = pIWICFactory->CreateFormatConverter(&pConverter);
    }

    if (SUCCEEDED(hr))
    {
        if (width != 0 || height != 0)
        {
            UINT originalWidth, originalHeight;
            hr = pSource->GetSize(&originalWidth, &originalHeight);
            if (SUCCEEDED(hr))
            {
                if (width == 0)
                {
                    FLOAT scalar = static_cast<FLOAT>(height) / static_cast<FLOAT>(originalHeight);
                    width = static_cast<UINT>(scalar * static_cast<FLOAT>(originalWidth));
                }
                else if (height == 0)
                {
                    FLOAT scalar = static_cast<FLOAT>(width) / static_cast<FLOAT>(originalWidth);
                    height = static_cast<UINT>(scalar * static_cast<FLOAT>(originalHeight));
                }

                hr = pIWICFactory->CreateBitmapScaler(&pScaler);
                if (SUCCEEDED(hr))
                {
                    hr = pScaler->Initialize(
                        pSource,
                        width,
                        height,
                        WICBitmapInterpolationModeCubic);
                }
                if (SUCCEEDED(hr))
                {
                    hr = pConverter->Initialize(
                        pScaler,
                        GUID_WICPixelFormat32bppPBGRA,
                        WICBitmapDitherTypeNone,
                        nullptr,
                        0.f,
                        WICBitmapPaletteTypeMedianCut);
                }
            }
        }
        else
        {
            hr = pConverter->Initialize(
                pSource,
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                nullptr,
                0.f,
                WICBitmapPaletteTypeMedianCut);
        }
    }
    if (SUCCEEDED(hr))
    {
        hr = pRenderTarget->CreateBitmapFromWicBitmap(
            pConverter,
            nullptr,
            ppBitmap);
    }

    SafeRelease(pDecoder);
    SafeRelease(pSource);
    SafeRelease(pStream);
    SafeRelease(pConverter);
    SafeRelease(pScaler);

    return hr;
}*/

/*
//-----------------------------------------------------------------
// 事件处理
void    handleevents()
{
    MSG CurrMsg;

    while (PeekMessageA(&CurrMsg, 0, 0, 0, 1))
    {
        TranslateMessage(&CurrMsg);
        DispatchMessageA(&CurrMsg);
    }
}
//-----------------------------------------------------------------
//程序延时
bool    Programdelay(int timeinterval)//时间间隔

{
    HANDLE   hTimer;                //时间句柄
    LARGE_INTEGER time;             //时间

    time.QuadPart = -10 * timeinterval * 1000 * 1;  //时间.成员时间
    hTimer = CreateWaitableTimerA(0, false, 0);      //时间句柄
    SetWaitableTimer(&hTimer, &time, 0, 0, 0, false);
    while (MsgWaitForMultipleObjects(1, &hTimer, false, -1, 255) != 0)
    {
        handleevents();//处理事件
    }
    CloseHandle(&hTimer);
    return true;
}
*/

