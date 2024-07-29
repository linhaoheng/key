#pragma once
/***********************************************************************
功能：Direct2D 1.1应用程序类，使用时可以继承这个类
作者：Ray1024
网址：http://www.cnblogs.com/Ray1024/
***********************************************************************/
#include "framework.h"
#include "D2D1Timer.h"

/******************************************************************
需要用到的宏定义
******************************************************************/
//模板，，释放指针 ppT 并将其设置为等于 NULL。
template<class Interface>
inline void SafeRelease(Interface** pInterfaceToRelease)
{
    if (*pInterfaceToRelease != NULL)
    {
        (*pInterfaceToRelease)->Release();

        (*pInterfaceToRelease) = NULL;
    }
}

#ifndef Assert
#if defined( DEBUG ) || defined( _DEBUG )
#define Assert(b) if (!(b)) {OutputDebugStringA("Assert: " #b "\n");}
#else
#define Assert(b)
#endif //DEBUG || _DEBUG
#endif

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)//将__ImageBase的地址强制类型转换为HINSTANCE类型
#endif


#include <math.h>

/******************************************************************
DemoApp
******************************************************************/
class DemoApp   //类
{
public:                                                       // 公共访问是允许的最高访问级别
    DemoApp();                                                // 构造函数
    ~DemoApp();                                               // 析构函数

    HRESULT Initialize();                                     //初始化创建窗口

    void Run();                                    //运行消息循环

private:
    HRESULT CreateDeviceIndependentResources();               //创建与设备无关的资源(创建D2D工厂和WIC工厂）

    HRESULT CreateDeviceResources();                          //创建设备资源（ 创建 D2D设备和DXGI设备与D2D设备上下文 ）

    HRESULT CreateWindowSizeDependentResources();                //创建与窗口大小相关的资源

    void DiscardDeviceResources();                            // 丢弃设备相关资源

    void CalculateFrameStats();                               // 计算帧数信息

    void UpdateScene(float dt) {};                            // 可以重写此函数来实现你想做的每帧执行的操作

    HRESULT OnRender();                                          //在渲染上

    //void OnResize(UINT width, UINT height);                   //调整大小

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message,  //窗口过程
        WPARAM wParam, LPARAM lParam);

    HRESULT LoadResourceBitmap(                               //加载资源位图 HRESULT是函数返回值
        ID2D1RenderTarget*   pRenderTarget,                   //渲染目标
        IWICImagingFactory*  pIWICFactory,                    //WIC工厂
        PCWSTR               resourceName,                    //是一个指针，是个常量，   资源名   
        PCWSTR               resourceType,                    //资源类型
        UINT                 destinationWidth,                //不带符号整形，复制目标粘贴后的宽
        UINT                 destinationHeight,               //不带符号整形，复制目标粘贴后的高
        ID2D1Bitmap**        ppBitmap);                       //表示已绑定到 ID2D1RenderTarget 的位图

protected:
    // 用到的资源-----------------------------------------------------

    D2D1Timer                m_timer;                       // 用于记录deltatime和游戏时间

    BOOL                     m_fRunning = TRUE;             // 是否运行

    HWND                     m_hwnd;                        // 窗口句柄

   

    std::string				 m_wndCaption = "";          // 窗口标题

    ID2D1Bitmap*             m_pBitmap[87] = {};		    // 位图

    ID3D11Device*            m_pD3DDevice;                  // D3D 设备

    ID3D11DeviceContext*     m_pD3DDeviceContext;           // D3D 设备上下文

    ID2D1Factory1*           m_pD2DFactory = nullptr;       // D2D 工厂

    ID2D1Device*             m_pD2DDevice = nullptr;        // D2D 设备

    ID2D1DeviceContext*      m_pD2DDeviceContext = nullptr; // D2D 设备上下文

    ID2D1HwndRenderTarget*   m_pRT;                         // 呈现器

    IDWriteFactory*          m_pDWriteFactory;	            // DWrite工厂

    IWICImagingFactory2*     m_pWICFactory = nullptr;       // WIC 工厂

    IDXGISwapChain1*         m_pSwapChain = nullptr;        // DXGI 交换链

    ID2D1Bitmap1*            m_pD2DTargetBimtap = nullptr;  // D2D 位图 储存当前显示的位图

    D3D_FEATURE_LEVEL        m_featureLevel;                //  所创设备特性等级

    DXGI_PRESENT_PARAMETERS  m_parameters;                  // 手动交换链

    RECT                     rc;                            // 声明矩形rc   
    int key[86] = { 27, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 192, 49, 50, 51, 52, 53, 54,
        55, 56, 57, 48, 189, 187, 8, 9, 81, 87, 69, 82, 84, 89, 85, 73, 79, 80, 219, 221,
        220, 20, 65, 83, 68, 70, 71, 72, 74, 75, 76, 186, 222, 13, 160, 90, 88, 67, 86, 66,
        78, 77, 188, 190, 191, 161, 162, 91, 164, 32, 165, 93, 163, 44, 145, 19, 45, 36, 33, 46, 35, 34, 38, 37, 40, 39 };
};

