#include "DemoApp.h"
#define KEY_DOWN(VK) ((GetAsyncKeyState(VK)& 0x8000 ? 1:0))
#define KEYUP(VK) ((GetAsyncKeyState(VK) & 0x8000) ? 0 : 1)
#define KEY_DOWN_FOREGROUND(hWnd,vk) (KEY_DOWN(vk) && GetForegroundWindow() == hWnd) //前景窗口判断
#pragma warning(disable:4996)

/***********************************************************************
功能：Direct2D 按键显示
作者：林豪横
***********************************************************************/

//-----------------------------------------------------------------
//声明函数

void InitTray(HWND hWnd);
void  keysound();
void  ForbiddenRepeatedlyRun();
void SetMouseIn(HWND hwnd, int TouMing);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
bool CheckFullscreen();
/*
//双击显示标题栏用的，应该是取放函数
int wstyle;
void SetStyle(int f)
{
    wstyle = f;
}
int GetStyle()
{
    return wstyle;
}
*/

/*
__declspec(selectany) HHOOK hkKey = NULL;

//安装钩子
extern "C" __declspec(dllexport) BOOL InstallHook();

//卸载钩子
extern "C" __declspec(dllexport) BOOL UninstallHook();

//钩子处理函数
LRESULT CALLBACK ProcessHook(int nCode, WPARAM wParam, LPARAM lParam);
*/
//-----------------------------------------------------------------
//-----------------------------------------------------------------
//声明全局变量
bool xianchengguanbi = false;
double          width = 413, height = 149;             //窗口宽高

double          ScrWid = GetSystemMetrics(SM_CXSCREEN); //屏幕宽
double          ScrHei = GetSystemMetrics(SM_CYSCREEN); //屏幕高
//double          ScrWid    = GetSystemMetrics(SM_CXFULLSCREEN); //不包含任务栏屏幕宽,但是不准确
//double          ScrHei    = GetSystemMetrics(SM_CYFULLSCREEN); //不包含任务栏屏幕高
//获取可用桌面大小
RECT rect;



int             Alpha;                                     //时钟周期用到的淡入效果透明值
HANDLE          hThread;
UINT            nElapse1, uElapse2;                         //时钟周期
//创建托盘
NOTIFYICONDATA  trayIcon;                                  //托盘属性  
HMENU           hMenu;                                     //托盘菜单


// 开启Alpha通道混合
// https://github.com/glfw/glfw/blob/master/src/win32_window.c#L368C27-L368C27
bool EnableAlphaCompositing(HWND hWnd)
{
    if (!::IsWindowsVistaOrGreater()) { return false; }

    BOOL is_composition_enable = false;
    ::DwmIsCompositionEnabled(&is_composition_enable);
    if (!is_composition_enable) { return true; }

    DWORD current_color = 0;
    BOOL is_opaque = false;
    ::DwmGetColorizationColor(&current_color, &is_opaque);

    if (!is_opaque || IsWindows8OrGreater())
    {
        HRGN region = ::CreateRectRgn(0, 0, -1, -1);
        DWM_BLURBEHIND bb = { 0 };
        bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
        bb.hRgnBlur = region;
        bb.fEnable = true;
        ::DwmEnableBlurBehindWindow(hWnd, &bb);
        ::DeleteObject(region);
        return true;
    }
    else // For Window7
    {
        DWM_BLURBEHIND bb = { 0 };
        bb.dwFlags = DWM_BB_ENABLE;
        ::DwmEnableBlurBehindWindow(hWnd, &bb);
        return false;
    }
}


//-----------------------------------------------------------------
/******************************************************************
DemoApp实现
******************************************************************/

DemoApp::DemoApp() : //演示应用程序
    m_hwnd(NULL),
    m_pD2DFactory(NULL),            //d2d工厂
    m_pWICFactory(NULL)             //WIC工厂
    //m_pDWriteFactory(NULL)        //DW工厂
{
    //TFEffect = 0;

    memset(                         //将某一块内存中的内容全部设置为指定的值
        &rc,                        //指向要填充的内存区域的指针。
        0,                          //要设置的值，通常是一个无符号字符。
        sizeof(rc));                //要被设置为该值的字符数。
}

DemoApp::~DemoApp()
{
    this->DiscardDeviceResources();
    SafeRelease(&m_pWICFactory);                 //释放 COM 接口指针。
    SafeRelease(&m_pD2DFactory);
    //SafeRelease(&m_pDWriteFactory);
}

//-----------------------------------------------------------------
// 从应用程序资源文件加载图像。从资源文件加载D2D位图
//-----------------------------------------------------------------
HRESULT DemoApp::LoadResourceBitmap(                    //加载资源位图 HRESULT是函数返回值
    ID2D1RenderTarget* pRenderTarget,                   //渲染目标
    IWICImagingFactory* pIWICFactory,                   //WIC工厂
    PCWSTR resourceName,                                //是一个指针，是个常量，   资源名   
    PCWSTR resourceType,                                //资源类型
    UINT destinationWidth,                              //不带符号整形，复制目标粘贴后的宽
    UINT destinationHeight,                             //不带符号整形，复制目标粘贴后的高
    ID2D1Bitmap** ppBitmap)                             //表示已绑定到 ID2D1RenderTarget 的位图

{
    IWICBitmapDecoder* pDecoder = NULL;                 //公开表示解码器的方法。
    IWICBitmapFrameDecode* pSource = NULL;              //是帧级接口，用于访问实际图像位
    IWICStream* pStream = NULL;                         //表示用于引用映像和元数据内容的 WINDOWS 映像组件 (WIC) 流
    IWICFormatConverter* pConverter = NULL;             //表示一个 IWICBitmapSource(公开引用从中检索像素但无法写回的源的方法。) ，它将图像数据从一种像素格式转换为另一种像素格式，处理到索引格式的抖动和半调、调色板转换和 alpha 阈值。
    IWICBitmapScaler* pScaler = NULL;                   //表示使用重采样或筛选算法调整输入位图的大小版本。

    HRSRC imageResHandle = NULL;                        //资源句柄，图像处理程序
    HGLOBAL imageResDataHandle = NULL;                  //内存句柄,图像数据处理程序
    void* pImageFile = NULL;                            //无类型指针，图片文件
    DWORD imageFileSize = 0;                            //32位无符号整数

    // Locate the resource.确定资源
    imageResHandle = FindResourceW(                     //确定指定模块中具有指定类型和名称的资源的位置。
        HINST_THISCOMPONENT,                            //模块的句柄，其可移植可执行文件或随附的 MUI 文件包含资源。
        resourceName,                                   //资源的名称。
        resourceType);                                  //资源类型
    HRESULT hr = imageResHandle ? S_OK : E_FAIL;        //HRESULT是函数返回值,资源句柄,S_OK是COM服务器返回正确,E_FAIL是必须处理的错误。表达式?(表达式为真时的值):(表达式为假时的值)
    if (SUCCEEDED(hr))         //如果成功（hr）
    {
        // Load the resource.加载资源
        imageResDataHandle = LoadResource(              //内存句柄，加载资源，检索可用于获取指向内存中指定资源第一个字节的指针的句柄。
            HINST_THISCOMPONENT,                        //其可执行文件包含资源的模块的句柄。
            imageResHandle);                            //资源句柄

        hr = imageResDataHandle ? S_OK : E_FAIL;        //内存句柄,表达式?(表达式为真时的值):(表达式为假时的值)
    }



    //3.锁定资源并计算图像的大小。
    if (SUCCEEDED(hr))        //如果成功（hr）
    {
        // Lock it to get a system memory pointer.锁定它以获得一个系统内存指针
        pImageFile = LockResource(imageResDataHandle); //图片文件,检索指向内存中指定资源的指针(内存句柄)

        hr = pImageFile ? S_OK : E_FAIL;               //内存句柄
    }
    if (SUCCEEDED(hr))                                 //如果成功（hr）
    {
        // Calculate the size.计算尺寸
        imageFileSize = SizeofResource(                //图像文件大小
            HINST_THISCOMPONENT,                       //其可执行文件包含资源的模块的句柄。
            imageResHandle);                           //资源句柄

        hr = imageFileSize ? S_OK : E_FAIL;            //资源句柄 ,表达式?(表达式为真时的值):(表达式为假时的值)
    }





    // 4.使用 IWICImagingFactory::CreateStream 方法创建 IWICStream 对象。
    if (SUCCEEDED(hr))                 //如果成功（hr）
    {
        // Create a WIC stream to map onto the memory.创建                一个 WIC 流映射到内存
        hr = pIWICFactory->CreateStream(&pStream);    // 创建 IWICStream 类的新实例。
    }
    if (SUCCEEDED(hr))               //如果成功（hr）
    {
        // Initialize the stream with the memory pointer and size.使用内存指针和大小初始化流
        hr = pStream->InitializeFromMemory(          //初始化流以将内存块视为流。 流不能增长到超过缓冲区大小。
            reinterpret_cast<BYTE*>(pImageFile),     //允许将任何指针转换为任何其他指针类型。 也允许将任何整数类型转换为任何指针类型以及反向转换。//指向用于初始化流的缓冲区的指针。
            imageFileSize                            //缓冲区的大小。
        );
    }




    //5.使用 IWICImagingFactory::CreateDecoderFromStream 方法创建 IWICBitmapDecoder。
    if (SUCCEEDED(hr))                              //如果成功（hr）
    {
        // Create a decoder for the stream.为流创建解码器
        hr = pIWICFactory->CreateDecoderFromStream( //基于给定的 IStream 创建 IWICBitmapDecoder 类的新实例。
            pStream,                                //要从中创建解码器的流
            NULL,                                   //首选解码器供应商的 GUID。 如果没有首选供应商，请使用 NULL 。
            WICDecodeMetadataCacheOnLoad,           //创建解码器时要使用的 WICDecodeOptions (指定解码选项）。
            &pDecoder                               //接收指向新 IWICBitmapDecoder(公开表示解码器的方法) 的指针的指针。
        );
    }



    //6.从图像中检索某一帧并将该帧存储在 IWICBitmapFrameDecode(是帧级接口，用于访问实际图像位) 对象中。
    if (SUCCEEDED(hr))                             //如果成功（hr）
    {
        // Create the initial frame.创建初始帧
        hr = pDecoder->GetFrame(                   //检索图像的指定帧。
            0,                                     //要检索的特定帧。
            &pSource);                             //一个指针，用于接收指向 IWICBitmapFrameDecode(访问实际图像位) 的指针。
    }




    /*7.必须先将图像转换为 32bppPBGRA 像素格式，然后 Direct2D 才能使用该图像。
        若要转换图像格式，请使用IWICImagingFactory::CreateFormatConverter 方法创IWICFormatConverter 对象，
        然后使用 IWICFormatConverter 对象的Initialize 方法执行转换*/
    if (SUCCEEDED(hr))                            //如果成功（hr）
    {
        // Convert the image format to 32bppPBGRA.将图像格式转换为32bppPBGRA
        // (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).值： 88 一种四分量 32 位无符号规范化整数格式，每个颜色通道支持 8 位，8 位未使用。+值：1alpha 值已预乘。 每种颜色首先按 alpha 值缩放。 alpha 值本身在直乘和预乘 alpha 中是相同的。 通常，没有颜色通道值大于 alpha 通道值。 如果采用预乘格式的颜色通道值大于 alpha 通道，则标准源间混合数学将产生加法混合。
        hr = pIWICFactory->CreateFormatConverter(&pConverter);  //创建 IWICFormatConverter 类的新实例。
    }
    if (SUCCEEDED(hr))                            //如果成功（hr）
    {

        hr = pConverter->Initialize(              //初始化格式转换器
            pSource,                              //要转换的输入位图
            GUID_WICPixelFormat32bppPBGRA,        //目标像素格式 GUID。
            WICBitmapDitherTypeNone,              //用于转换的 WICBitmapDitherType(指定在图像格式之间转换时要应用的 dither 算法的类型) 。
            NULL,                                 //用于转换的调色板。
            0.f,                                  //用于转换的 alpha 阈值。
            WICBitmapPaletteTypeMedianCut         //用于转换的调色板转换类型。
        );
    }




    /*8.最后，使用 CreateBitmapFromWicBitmap 方法创建 ID2D1Bitmap 对象，
        该对象可通过呈现器目标绘制并与其他 Direct2D 对象一起使用。*/
    if (SUCCEEDED(hr))                           //如果成功（hr）
    {
        //create a Direct2D bitmap from the WIC bitmap.从 WIC 位图创建 Direct2D 位图
        hr = pRenderTarget->CreateBitmapFromWicBitmap(   //通过复制指定的 Microsoft Windows 映像组件 (WIC) 位图来创建 ID2D1Bitmap。
            pConverter,                          //要复制的 WIC 位图。
            NULL,                                //要创建的位图的像素格式和 DPI。
            ppBitmap                             //此方法返回时，包含指向新位图的指针的地址。 此参数未经初始化即被传递。
        );

    }

    SafeRelease(&pDecoder);                     //释放 COM 接口指针。
    SafeRelease(&pSource);
    SafeRelease(&pStream);
    SafeRelease(&pConverter);
    SafeRelease(&pScaler);

    return hr;
}
//-----------------------------------------------------------------
// 初始化创建窗口
HRESULT DemoApp::Initialize()
{
    ForbiddenRepeatedlyRun();
    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)keysound, NULL, 0, NULL);//创建线程
    SuspendThread(hThread);  //线程挂起
    //thread t2(keysound);   //创建音效线程
    //t2.detach();           //启动音效线程
    HRESULT hr;              //声明一个变量
    //获取可用桌面大小
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
    double          ScrWidZ = rect.right - rect.left;
    double          ScrHeiZ = rect.bottom - rect.top;
    //register window class。注册窗口类
    WNDCLASSEX wcex    = { sizeof(WNDCLASSEX) };
    wcex.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;   //窗口样式
    wcex.lpfnWndProc   = DemoApp::WndProc;                       //消息处理函数
    wcex.cbClsExtra    = 0;                                      //窗口类结构后的附加字节数
    wcex.cbWndExtra    = sizeof(LONG_PTR);
    wcex.hInstance     = HINST_THISCOMPONENT;                    //窗口属于的模块句柄
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);            //鼠标样式
    wcex.hbrBackground = NULL;                                   // (HBRUSH)CreateSolidBrush(0x000000);     //背景画刷句柄
    wcex.lpszMenuName  = NULL;                                   //菜单
    wcex.lpszClassName = L"Key";                                 //窗口类名称
    wcex.hIcon         = LoadIconW(0, IDI_APPLICATION);          //窗口图标
    wcex.hIconSm       = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));//窗口小图标

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(nullptr, L"注册窗口类失败!", L"错误", MB_OK);
        return 0;
    }

    // 创建应用程序窗口

    hr = CreateDeviceIndependentResources(); //创建与设备无关的资源
    if (SUCCEEDED(hr))
    {
        CreateDeviceResources();            //创建设备资源
        // 因为 CreateWindow 函数的大小以像素为单位
        // 我们得到系统的 DPI 并用它来缩放窗口大小
        FLOAT dpiX, dpiY;    //浮点型
        //检索当前桌面每英寸点数(DPI) 。 若要刷新此值，请调用 ReloadSystemMetrics。
        m_pD2DFactory->GetDesktopDpi(&dpiX, &dpiY);
        double x = ScrWidZ - width;                               //窗口初始位置
        double y = ScrHeiZ - height;
        m_hwnd = CreateWindowExA(
            WS_EX_NOACTIVATE,                                     //窗口扩展样式
            "Key",                                                //窗体类名
            m_wndCaption.c_str(),                                 //窗口标题
            WS_VISIBLE | WS_POPUP,                                //窗口样式
            x,                                                    //窗口初始屏幕X坐标
            y,                                                    //窗口初始屏幕X坐标
            static_cast<UINT>(ceil(413.f * dpiX / 96.f)),         //窗口初始宽度
            static_cast<UINT>(ceil(149.f * dpiX / 96.f)),         //窗口初始高度
            NULL,                                                 //父窗体句柄
            NULL,                                                 //菜单的句柄或是子窗口的标识符
            HINST_THISCOMPONENT,                                  //应用程序实例的句柄
            this                                                  //指向窗口的创建数据
        );

        if (m_hwnd == nullptr)
        {
            MessageBox(nullptr, L"创建窗口失败!", L"错误", MB_OK);
            return 0;
        }

        // 开启Alpha通道混合
        EnableAlphaCompositing(m_hwnd);

        nElapse1 = 10;                          //时钟周期
        SetTimer(m_hwnd, 1, nElapse1, NULL);    //创建时钟


        hr = m_hwnd ? S_OK : E_FAIL;            //窗口句柄
        if (SUCCEEDED(hr))                      //如果成功（hr）
        {
            CreateWindowSizeDependentResources();
            ShowWindow(m_hwnd, SW_SHOWNORMAL);  //显示窗口
            UpdateWindow(m_hwnd);               //如果窗口的更新区域不为空， UpdateWindow 函数通过向窗口发送 WM_PAINT 消息来更新指定窗口的工作区。
        }
    }
    InitTray(m_hwnd);                       //实例化托盘  
    return hr;                              //返回
}
//-----------------------------------------------------------------
//创建与设备无关的资源(创建D2D工厂和WIC工厂）
HRESULT DemoApp::CreateDeviceIndependentResources()
{
    HRESULT hr;                                  //声明变量hr

    // 创建D2D工厂
    hr = D2D1CreateFactory(                      //创建可用于创建 Direct2D 资源的工厂对象。
        D2D1_FACTORY_TYPE_SINGLE_THREADED,       //工厂的线程模型及其创建的资源。
        &m_pD2DFactory);                         //对使用 获取的 ID2D1Factory 的 IID 的 __uuidof(ID2D1Factory)引用。

    /*if (m_pDWriteFactory == NULL && SUCCEEDED(hr))//如果DW工厂没了，且成功（hr）
    {
        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pDWriteFactory),
            reinterpret_cast<IUnknown**>(&m_pDWriteFactory));
    }*/

    if (m_pWICFactory == NULL && SUCCEEDED(hr))  //如果WIC工厂没了，且成功（hr）
    {
        if (SUCCEEDED(                           //如果成功
            CoCreateInstance(                    //创建并默认初始化与指定 CLSID 关联的类的单个对象。
                CLSID_WICImagingFactory2,        //与将用于创建对象的数据和代码关联的 CLSID。
                nullptr,                         //如果 为 NULL，则指示对象不是作为聚合的一部分创建的。 如果不是 NULL，则指向聚合对象的 IUnknown 接口的指针
                CLSCTX_INPROC_SERVER,            //管理新创建对象的代码将在其中运行的上下文。 这些值取自枚举 CLSCTX。
                IID_PPV_ARGS(&m_pWICFactory)     //对要用于与对象通信的接口标识符的引用。              
            )
        ))
            return true;                         //返回z
    }

    return hr;                                   //返回资源句柄
}
//-----------------------------------------------------------------
//创建设备资源（ 创建 D2D设备和DXGI设备与D2D设备上下文 ）
HRESULT DemoApp::CreateDeviceResources()
{

    IDXGIFactory2* pDxgiFactory = nullptr;    // DXGI 工厂

    IDXGIDevice1* pDxgiDevice   = nullptr;    // DXGI 设备

    HRESULT            hr = S_OK;


    // 创建 D3D11设备与设备上下文 
    if (SUCCEEDED(hr))
    {
        // D3D11 创建flag 
        // 一定要有D3D11_CREATE_DEVICE_BGRA_SUPPORT，否则创建D2D设备上下文会失败
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
        // Debug状态 有D3D DebugLayer就可以取消注释
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1
        };
        // 创建设备
        hr = D3D11CreateDevice(
            nullptr,					// 设为空指针选择默认设备
            D3D_DRIVER_TYPE_HARDWARE,   // 强行指定硬件渲染
            nullptr,					// 强行指定WARP渲染 D3D_DRIVER_TYPE_WARP 没有软件接口
            creationFlags,				// 创建flag
            featureLevels,				// 欲使用的特性等级列表
            ARRAYSIZE(featureLevels),   // 特性等级列表长度
            D3D11_SDK_VERSION,		    // SDK 版本
            &m_pD3DDevice,				// 返回的D3D11设备指针
            &m_featureLevel,			// 返回的特性等级
            &m_pD3DDeviceContext);      // 返回的D3D11设备上下文指针
    }

    // 创建 IDXGIDevice
    if (SUCCEEDED(hr))
        hr = m_pD3DDevice->QueryInterface(IID_PPV_ARGS(&pDxgiDevice));
    // 创建D2D设备
    if (SUCCEEDED(hr))
        hr = m_pD2DFactory->CreateDevice(pDxgiDevice, &m_pD2DDevice);
    // 创建D2D设备上下文
    if (SUCCEEDED(hr))
        hr = m_pD2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_pD2DDeviceContext);

    SafeRelease(&pDxgiDevice);
    SafeRelease(&pDxgiFactory);

    return hr;
}
//-----------------------------------------------------------------
//创建与窗口大小相关的资源
HRESULT    DemoApp::CreateWindowSizeDependentResources()
{
    // 我们得到系统的 DPI 并用它来缩放窗口大小
    FLOAT dpiX, dpiY;    //浮点型

    IDXGIAdapter* pDxgiAdapter    = nullptr;      // DXGI 适配器

    IDXGIFactory2* pDxgiFactory   = nullptr;      // DXGI 工厂

    IDXGISurface* pDxgiBackBuffer = nullptr;      // DXGI Surface 后台缓冲

    IDXGIDevice1* pDxgiDevice     = nullptr;      // DXGI 设备

    HRESULT hr = S_OK;

    // 清除之前窗口的呈现器相关设备
    m_pD2DDeviceContext->SetTarget(nullptr);

    m_pD3DDeviceContext->Flush();

    RECT rect = { 0 }; GetClientRect(m_hwnd, &rect);

    if (m_pSwapChain != nullptr)
    {
        // 如果交换链已经创建，则重设缓冲区
        hr = m_pSwapChain->ResizeBuffers(
            2, // Double-buffered swap chain.
            lround(rect.right - rect.left),
            lround(rect.bottom - rect.top),
            DXGI_FORMAT_B8G8R8A8_UNORM,
            0);

        assert(hr == S_OK);
    }
    else
    {
        // 否则用已存在的D3D设备创建一个新的交换链
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
        swapChainDesc.Width              = lround(rect.right - rect.left);         //宽度
        swapChainDesc.Height             = lround(rect.bottom - rect.top);         //高度
        swapChainDesc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM;             //格式
        swapChainDesc.Stereo             = false;                                  //全屏或立体
        swapChainDesc.SampleDesc.Count   = 1;                                      //采样_数量
        swapChainDesc.SampleDesc.Quality = 0;                                      //采样_质量
        swapChainDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;        //使用方法(DXGI_用法_渲染目标输出)
        swapChainDesc.BufferCount        = 2;                                      //缓冲区数
        swapChainDesc.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;               //效果(DXGI_交换效果_丢弃)
        swapChainDesc.Flags              = 0;                                      //标志
        swapChainDesc.Scaling            = DXGI_SCALING_STRETCH;                   //缩放
        swapChainDesc.AlphaMode          = DXGI_ALPHA_MODE_IGNORE;                 //透明模式

        // 获取 IDXGIDevice
        if (SUCCEEDED(hr))
        {
            hr = m_pD3DDevice->QueryInterface(IID_PPV_ARGS(&pDxgiDevice));
        }
        // 获取Dxgi适配器 可以获取该适配器信息
        if (SUCCEEDED(hr))
        {
            hr = pDxgiDevice->GetAdapter(&pDxgiAdapter);
        }
        // 获取Dxgi工厂
        if (SUCCEEDED(hr))
        {
            hr = pDxgiAdapter->GetParent(IID_PPV_ARGS(&pDxgiFactory));
        }
        // 创建交换链
        if (SUCCEEDED(hr))
        {
            hr = pDxgiFactory->CreateSwapChainForHwnd(      //创建交换链自窗口
                m_pD3DDevice,                               //参数_DXGI设备
                m_hwnd,                                     //参数_窗口句柄
                &swapChainDesc,                             //交换链结构
                nullptr,                                    //
                nullptr,                                    //
                &m_pSwapChain);                             //参数_DXGI交换链
        }
        // 确保DXGI队列里边不会超过一帧
        if (SUCCEEDED(hr))
        {
            hr = pDxgiDevice->SetMaximumFrameLatency(1);
        }
    }

    // 设置屏幕方向
    /*if (SUCCEEDED(hr))
    {
        hr = m_pSwapChain->SetRotation(DXGI_MODE_ROTATION_IDENTITY);
    }*/
    // 利用交换链获取Dxgi表面
    if (SUCCEEDED(hr))
    {
        hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pDxgiBackBuffer));
    }

    //检索当前桌面每英寸点数(DPI) 。 若要刷新此值，请调用 ReloadSystemMetrics。
    m_pD2DFactory->GetDesktopDpi(&dpiX, &dpiY);
    // 利用Dxgi表面创建位图
    if (SUCCEEDED(hr))
    {
        D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,    //位图选项
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,                    //像素格式
                D2D1_ALPHA_MODE_PREMULTIPLIED),                              //透明模式
            dpiX,
            dpiY);
        hr = m_pD2DDeviceContext->CreateBitmapFromDxgiSurface(
            pDxgiBackBuffer,
            &bitmapProperties,
            &m_pD2DTargetBimtap);
    }
    // 设置
    if (SUCCEEDED(hr))
    {
        // 设置 Direct2D 渲染目标
        m_pD2DDeviceContext->SetTarget(m_pD2DTargetBimtap);
    }

    SafeRelease(&pDxgiDevice);
    SafeRelease(&pDxgiAdapter);
    SafeRelease(&pDxgiFactory);
    SafeRelease(&pDxgiBackBuffer);
    if (SUCCEEDED(hr))
    {
        for (int i = 0; i <= 86; i++)
        {
            LoadResourceBitmap(m_pD2DDeviceContext, m_pWICFactory, MAKEINTRESOURCE(i + 1000), L"PNG", 0, 0, &m_pBitmap[i]);
        }
    }
    return hr;
}
//-----------------------------------------------------------------
// 丢弃设备资源
void    DemoApp::DiscardDeviceResources()
{
    SafeRelease(&m_pD2DTargetBimtap);
    SafeRelease(&m_pSwapChain);
    SafeRelease(&m_pD2DDeviceContext);
    SafeRelease(&m_pD2DDevice);
    SafeRelease(&m_pD3DDevice);
    SafeRelease(&m_pD3DDeviceContext);

    for (int i = 0; i <= 86; i++)
    {
        SafeRelease(&m_pBitmap[i]);
    }
}
//-----------------------------------------------------------------
// 计算每秒平均帧数的代码，还计算了绘制一帧的平均时间
unsigned int hour = 0, minute = 0, second = 0;
void    DemoApp::CalculateFrameStats()
{
    // 计算每秒平均帧数的代码，还计算了绘制一帧的平均时间
    // 这些统计信息会显示在窗口标题栏中
    static int frameCnt = 0;
    static float timeElapsed = 0.0f;

    frameCnt++;

    unsigned int timer = clock() / CLOCKS_PER_SEC;
    second = (timer % 60);
    minute = (timer / 60) % 60;
    hour   = (timer / 60) / 60;

    // 计算一秒时间内的平均值
    if ((m_timer.TotalTime() - timeElapsed) >= 1.0f)
    {
        float fps = (float)frameCnt; // fps = frameCnt / 1
        float mspf = 1000.0f / fps;

        std::ostringstream outs;
        outs.precision(6);

        outs << "Time: " << setw(2) << setfill('0') << hour << ":" << setw(2) << setfill('0') << minute << ":" << setw(2) << setfill('0')
            << second << " | " <<
            "FPS: " << fps << " | " << mspf << " (ms)";

        //outs << m_wndCaption.c_str() << " | "<< "FPS: " << fps << " | "<< "Frame Time: " << mspf << " (ms)";

        SetWindowTextA(m_hwnd, outs.str().c_str());

        // 为了计算下一个平均值重置一些值
        frameCnt = 0;
        timeElapsed += 1.0f;
    }
}
//-----------------------------------------------------------------
//窗口消息过程
LRESULT CALLBACK DemoApp::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;                                     //结果=0
    DemoApp* pDemoApp;

    bool wasHandled = false;                            //已经处理好了=假 

    UINT state;                                         //判断窗口置顶穿透等状态

    static POINT OldPos, NewPos;                        //鼠标光标的位置，用于取鼠标偏移量以移动窗口，用于取鼠标偏移量以缩放窗口
    static double   xOffset, yOffset;                   //鼠标的偏移量，用于缩放窗口
    static RECT  rWindow;                               //窗口矩形，用于缩放窗口，用于移动窗口和显示标题栏
    double whb = width / height;                        //窗口的宽高比，用雨固定窗口比例
    double          ScrWidZ = rect.right - rect.left;
    double          ScrHeiZ = rect.bottom - rect.top;
    // 不要修改TaskbarCreated，这是系统任务栏自定义的消息
    UINT WM_TASKBARCREATED = RegisterWindowMessage(TEXT("TaskbarCreated"));

    if (message == WM_CREATE)                               //放置窗口初始化代码，如建立各种子窗口（状态栏和工具栏等）
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;        //定义传递给应用程序的窗口过程的初始化参数。 这些成员与 CreateWindowEx 函数的参数相同。
        DemoApp* pDemoApp = (DemoApp*)pcs->lpCreateParams;  //创建参数

        ::SetWindowLongPtrW(                                //更改指定窗口的属性。
            hwnd,                                           //窗口的句柄
            GWLP_USERDATA,                                  //要设置的值的从零开始的偏移量。 有效值介于零到额外窗口内存的字节数之间，减去 LONG_PTR的大小。 若要设置任何其他值，请指定以下值之一。
            PtrToUlong(pDemoApp)                            //替换值。
        );

        result = 1;                                         //结果=1
    }
    else
    {
        DemoApp* pDemoApp = reinterpret_cast<DemoApp*>(static_cast<LONG_PTR>(     // //允许将任何指针转换为任何其他指针类型。 也允许将任何整数类型转换为任何指针类型以及反向转换。//指向用于初始化流的缓冲区的指针。
            ::GetWindowLongPtrW(                            //检索有关指定窗口的信息。 函数还会将指定偏移量的值检索到额外的窗口内存中。
                hwnd,                                       //窗口的句柄
                GWLP_USERDATA                               //要检索的值的从零开始的偏移量。 有效值介于零到额外窗口内存的字节数之间，减去 LONG_PTR的大小。 若要检索任何其他值，请指定以下值之一。
            )));

        if (pDemoApp)
        {
            switch (message)
            {
                /*
            case WM_PAINT:
            {
                pDemoApp->OnRender();
            }
            case WM_SIZE:                                   //窗口大小被改变
            {
                UINT width = LOWORD(lParam);                //不带符号整型
                UINT height = HIWORD(lParam);
                pDemoApp->OnResize(width, height);          //调整大小
            }

            case WM_DISPLAYCHANGE:                          //显示器的分辨率改变后发送此消息给所有的窗口
            {
                InvalidateRect(hwnd, 0, 0);//更新窗口
            }
            result = 0;                                     //结果=0
            wasHandled = true;                              //已经处理好了=真
            break;
            */

            case WM_TIMER:                     //计时器函数
            {

                if (wParam == 1)

                {
                    if (Alpha > 250)
                    {
                        nElapse1 = 0;
                        KillTimer(hwnd, 1);  //释放时钟
                    }
                    else
                    {
                        Alpha = Alpha + 15;
                    }
                    SetMouseIn(hwnd, Alpha);
                }
                if (wParam == 2)
                {
                    if (Alpha <= 10)
                    {
                        SendMessage(hwnd, WM_CLOSE, wParam, lParam);
                    }
                    else
                    {
                        SetMouseIn(hwnd, Alpha);
                    }
                    Alpha = Alpha - 15;
                }
            }
            result = 1;                                     //结果=1
            wasHandled = true;                              //已经处理好了=真
            break;
            case WM_LBUTTONDOWN:                            //按下左键
            {
                SetCapture(hwnd);
                OldPos.x = LOWORD(lParam);
                OldPos.y = HIWORD(lParam);
            }
            result = 1;                                     //结果=1
            wasHandled = true;                              //已经处理好了=真
            break;

            case WM_LBUTTONUP:                              //抬起左键
            {
                ReleaseCapture();
                if (!CheckFullscreen())//如果没有全屏窗口  
                {
                    if (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CAPTION)
                    {
                        if (rWindow.bottom > ScrHeiZ)//下
                            SetWindowPos(hwnd, NULL, rWindow.left, ScrHeiZ + rWindow.top - rWindow.bottom + 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0)//左
                            SetWindowPos(hwnd, NULL, -8, rWindow.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.top < 0)//上
                            SetWindowPos(hwnd, NULL, rWindow.left, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.right > ScrWidZ)//右
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right + 8, rWindow.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

                        if (rWindow.right > ScrWidZ && rWindow.bottom > ScrHeiZ)//右下
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right + 8, ScrHeiZ + rWindow.top - rWindow.bottom + 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0 && rWindow.bottom > ScrHeiZ)//左下
                            SetWindowPos(hwnd, NULL, -8, ScrHeiZ + rWindow.top - rWindow.bottom + 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.top < 0 && rWindow.right > ScrWidZ)//右上
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right + 8, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0 && rWindow.top < 0)//左上
                            SetWindowPos(hwnd, NULL, -8, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

                        if (rWindow.right - rWindow.left > ScrWidZ && rWindow.bottom - rWindow.top > ScrHeiZ)
                            SetWindowPos(hwnd, NULL, -8, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                    }
                    else
                    {
                        if (rWindow.bottom > ScrHeiZ)//下
                            SetWindowPos(hwnd, NULL, rWindow.left, ScrHeiZ + rWindow.top - rWindow.bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0)//左
                            SetWindowPos(hwnd, NULL, 0, rWindow.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.top < 0)//上
                            SetWindowPos(hwnd, NULL, rWindow.left, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.right > ScrWidZ)//右
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right, rWindow.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

                        if (rWindow.right > ScrWidZ && rWindow.bottom > ScrHeiZ)//右下
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right, ScrHeiZ + rWindow.top - rWindow.bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0 && rWindow.bottom > ScrHeiZ)//左下
                            SetWindowPos(hwnd, NULL, 0, ScrHeiZ + rWindow.top - rWindow.bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.top < 0 && rWindow.right > ScrWidZ)//右上
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0 && rWindow.top < 0)//左上
                            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

                        if (rWindow.right - rWindow.left > ScrWidZ && rWindow.bottom - rWindow.top > ScrHeiZ)//窗口宽大于屏幕宽或窗口高大于屏幕高
                            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                    }
                    
                }


            }
            result = 1;                                     //结果=1
            wasHandled = true;                              //已经处理好了=真
            break;

            case WM_RBUTTONDOWN:                            //按下右键
            {
                SetCapture(hwnd);
                GetCursorPos(&OldPos);

            }
            result = 1;                                     //结果=1
            wasHandled = true;                              //已经处理好了=真
            break;

            case WM_RBUTTONUP:                              //抬起右键
            {
                ReleaseCapture();

                if (!CheckFullscreen())//如果没有全屏窗口  
                {
                    if (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CAPTION)
                    {
                        if (rWindow.bottom > ScrHeiZ)//下
                            SetWindowPos(hwnd, NULL, rWindow.left, ScrHeiZ + rWindow.top - rWindow.bottom + 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0)//左
                            SetWindowPos(hwnd, NULL, -8, rWindow.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.top < 0)//上
                            SetWindowPos(hwnd, NULL, rWindow.left, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.right > ScrWidZ)//右
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right + 8, rWindow.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

                        if (rWindow.right > ScrWidZ && rWindow.bottom > ScrHeiZ)//右下
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right + 8, ScrHeiZ + rWindow.top - rWindow.bottom + 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0 && rWindow.bottom > ScrHeiZ)//左下
                            SetWindowPos(hwnd, NULL, -8, ScrHeiZ + rWindow.top - rWindow.bottom + 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.top < 0 && rWindow.right > ScrWidZ)//右上
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right + 8, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0 && rWindow.top < 0)//左上
                            SetWindowPos(hwnd, NULL, -8, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

                        if (rWindow.right - rWindow.left > ScrWidZ && rWindow.bottom - rWindow.top > ScrHeiZ)
                            SetWindowPos(hwnd, NULL, -8, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                    }
                    else
                    {
                        if (rWindow.bottom > ScrHeiZ)//下
                            SetWindowPos(hwnd, NULL, rWindow.left, ScrHeiZ + rWindow.top - rWindow.bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0)//左
                            SetWindowPos(hwnd, NULL, 0, rWindow.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.top < 0)//上
                            SetWindowPos(hwnd, NULL, rWindow.left, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.right > ScrWidZ)//右
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right, rWindow.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

                        if (rWindow.right > ScrWidZ && rWindow.bottom > ScrHeiZ)//右下
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right, ScrHeiZ + rWindow.top - rWindow.bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0 && rWindow.bottom > ScrHeiZ)//左下
                            SetWindowPos(hwnd, NULL, 0, ScrHeiZ + rWindow.top - rWindow.bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.top < 0 && rWindow.right > ScrWidZ)//右上
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0 && rWindow.top < 0)//左上
                            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

                        if (rWindow.right - rWindow.left > ScrWidZ && rWindow.bottom - rWindow.top > ScrHeiZ)//窗口宽大于屏幕宽或窗口高大于屏幕高
                            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                    }
                }
            }
            result = 1;                                     //结果=1
            wasHandled = true;                              //已经处理好了=真
            break;

            //鼠标移动
            case WM_MOUSEMOVE:
            {
                GetWindowRect(hwnd, &rWindow);
                if (GetCapture() == hwnd && wParam == MK_LBUTTON)   //左键按下移动窗口
                {
                    SetWindowPos(hwnd,
                        NULL, rWindow.left + (short)LOWORD(lParam) - OldPos.x,
                        rWindow.top + (short)HIWORD(lParam) - OldPos.y,
                        0,
                        0,
                        SWP_NOSIZE | SWP_NOZORDER);
                }
                if (GetCapture() == hwnd && wParam == MK_RBUTTON)   //右键按下缩放窗口
                {
                    GetCursorPos(&NewPos);
                    xOffset = NewPos.x - OldPos.x;
                    yOffset = NewPos.y - OldPos.y;

                    if (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CAPTION)
                    {
                        SetWindowPos(hwnd, NULL, 1, 1, 1, 1, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
                    }
                    else
                    {
                        if (xOffset * xOffset > yOffset * yOffset)     //鼠标移动宽度大于高度
                        {
                            MoveWindow(hwnd,
                                rWindow.left,
                                rWindow.top,
                                (rWindow.right + xOffset - rWindow.left),
                                (rWindow.right + xOffset - rWindow.left) / whb,
                                false);
                        }
                        else                                           //鼠标移动宽度不大于高度
                        {
                            MoveWindow(hwnd,
                                rWindow.left,
                                rWindow.top,
                                (rWindow.bottom + yOffset - rWindow.top) * whb,
                                rWindow.bottom + yOffset - rWindow.top,
                                false);
                        }
                        OldPos.x = NewPos.x;
                        OldPos.y = NewPos.y;
                    }
                }
            }
            result = 1;                                     //结果=1
            wasHandled = true;                              //已经处理好了=真
            break;

            //限制窗口移动范围 
            case WM_WINDOWPOSCHANGING:
            {
                WINDOWPOS* pWinPos = (WINDOWPOS*)lParam;
                if (CheckFullscreen())//如果有全屏窗口      
                {
                    if (pWinPos->x > ScrWid - (rWindow.right - rWindow.left))//右
                        pWinPos->x = ScrWid - (rWindow.right - rWindow.left);

                    if (pWinPos->y > ScrHei - (rWindow.bottom - rWindow.top))//下
                        pWinPos->y = ScrHei - (rWindow.bottom - rWindow.top);

                    if (pWinPos->x < 0)     //左
                        pWinPos->x = 0;

                    if (pWinPos->y < 0)     //上
                        pWinPos->y = 0;
                }


                if (pWinPos->cx < 13)  //限制最小尺寸
                    pWinPos->cx = 13;
                if (pWinPos->cy < 5)
                    pWinPos->cy = 5;

                return 0;
            }
            result = 1;                                     //结果=1
            wasHandled = true;                              //已经处理好了=真
            break;

            //双击窗口置顶
            case WM_LBUTTONDBLCLK:
            {
                state = GetMenuState(hMenu, ID_TOP, MF_BYCOMMAND);
                if (state & MF_CHECKED)
                {
                    CheckMenuItem(hMenu, ID_TOP, MF_UNCHECKED | MF_BYCOMMAND);
                    SetWindowPos(hwnd, HWND_NOTOPMOST, 1, 1, 1, 1, SWP_NOMOVE | SWP_NOSIZE);
                }
                else
                {
                    CheckMenuItem(hMenu, ID_TOP, MF_CHECKED | MF_BYCOMMAND);
                    SetWindowPos(hwnd, HWND_TOPMOST, 1, 1, 1, 1, SWP_NOMOVE | SWP_NOSIZE);
                }
            }
            result = 1;                                     //结果=1
            wasHandled = true;                              //已经处理好了=真    
            break;

            //双击右键窗口穿透
            case WM_RBUTTONDBLCLK:
            {
                CheckMenuItem(hMenu, ID_PENETARTE, MF_CHECKED | MF_BYCOMMAND);
                SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
            }
            result = 1;                                     //结果=1
            wasHandled = true;                              //已经处理好了=真    
            break;


            /*
            case WM_CLOSE:                                  //结束窗口并发送 WM_QUIT
            {
                DestroyWindow(hwnd);                        //销毁指定的窗口
            }
            break;
            */

            //托盘菜单消息
            case MYWM_NOTIFYICON:                           //托盘菜单消息
            {
                if (lParam == WM_RBUTTONDOWN)
                {
                    //获取鼠标坐标
                    POINT pt; GetCursorPos(&pt);
                    //右击后点别地可以清除“右击出来的菜单”
                    SetForegroundWindow(hwnd);
                    //弹出菜单,并把用户所选菜单项的标识符返回
                    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, NULL, hwnd, NULL);

                    if (cmd == ID_SHOW)
                    {
                        if (IsWindowVisible(hwnd))
                        {
                            ShowWindowAsync(hwnd, false);
                        }
                        else
                        {
                            ShowWindowAsync(hwnd, true);
                        }
                    }
                    if (cmd == ID_SOUND)
                    {
                        if (GetMenuState(hMenu, ID_SOUND, MF_BYCOMMAND) & MF_CHECKED)
                        {
                            CheckMenuItem(hMenu, ID_SOUND, MF_UNCHECKED | MF_BYCOMMAND);
                            SuspendThread(hThread);
                        }
                        else
                        {
                            CheckMenuRadioItem(hMenu, ID_SOUND, ID_SOUND,ID_SOUND, MF_BYCOMMAND);
                            ResumeThread(hThread);
                        }
                    }
                    if (cmd == ID_TOP)
                    {
                        if (GetMenuState(hMenu, ID_TOP, MF_BYCOMMAND) & MF_CHECKED)
                        {
                            CheckMenuItem(hMenu, ID_TOP, MF_UNCHECKED | MF_BYCOMMAND);
                            SetWindowPos(hwnd, HWND_NOTOPMOST, 1, 1, 1, 1, SWP_NOMOVE | SWP_NOSIZE);
                        }
                        else
                        {
                            CheckMenuItem(hMenu, ID_TOP, MF_CHECKED | MF_BYCOMMAND);
                            SetWindowPos(hwnd, HWND_TOPMOST, 1, 1, 1, 1, SWP_NOMOVE | SWP_NOSIZE);
                        }
                    }
                    if (cmd == ID_PENETARTE)
                    {
                        if (CheckMenuItem(hMenu, ID_PENETARTE, MF_CHECKED) & MF_CHECKED)
                            CheckMenuItem(hMenu, ID_PENETARTE, MF_UNCHECKED);
                        if (SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT) &WS_EX_TRANSPARENT)
                            SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) & (~WS_EX_TRANSPARENT));
                    }
                    if (cmd == ID_ABOUT)
                    {
                        DialogBox(HINST_THISCOMPONENT, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, About);
                        SetWindowPos(hwnd,            //还原窗口大小
                            NULL,
                            0,
                            0,
                            width,
                            height,
                            SWP_NOMOVE | SWP_NOZORDER);
                        if (!IsWindowVisible(hwnd))
                        {
                            ShowWindowAsync(hwnd, true);  //显示窗口
                        }
                    }
                    if (cmd == ID_QUIT)
                    {
                        uElapse2 = 10;
                        SetTimer(hwnd, 2, uElapse2, NULL);
                        Alpha = 254;
                        //SendMessage(hwnd, WM_CLOSE, wParam, lParam);
                    }
                }
                if (lParam == WM_LBUTTONDBLCLK)       //双击左键,显示标题栏
                {
                    if (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CAPTION)
                    {
                        GetClientRect(hwnd, &rWindow);
                        ClientToScreen(hwnd, (POINT*)&rWindow);
                        ClientToScreen(hwnd, (POINT*)&rWindow.right);
                    }
                    else
                    {
                        GetWindowRect(hwnd, &rWindow);
                        AdjustWindowRectEx(&rWindow, GetWindowLongPtr(hwnd, GWL_STYLE) ^ WS_CAPTION, FALSE, GetWindowLongPtr(hwnd, GWL_EXSTYLE));
                    }
                    SetWindowLongPtr(hwnd, GWL_STYLE, GetWindowLongPtr(hwnd, GWL_STYLE) ^ WS_CAPTION);
                    SetWindowPos(hwnd, NULL, 1, 1, 1, 1, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
                    MoveWindow(hwnd, rWindow.left, rWindow.top, rWindow.right - rWindow.left, rWindow.bottom - rWindow.top, TRUE);
                }

                if (lParam == WM_LBUTTONDOWN)         //单击托盘左键
                {
                    SetForegroundWindow(hwnd);        //窗口置焦点
                }
            }
            result = 1;                                     //结果=1
            wasHandled = true;                              //已经处理好了=真
            break;

            //窗口即将被摧毁
            case WM_DESTROY:                                //窗口即将被摧毁
            {
                KillTimer(hwnd, 2);                         //释放时钟
                xianchengguanbi = true;
                CloseHandle(hThread);
                DestroyMenu(hMenu);                         //清除菜单
                Shell_NotifyIcon(NIM_DELETE, &trayIcon);    //销毁托盘图标
                PostQuitMessage(0);                         //销毁窗口
            }
            result = 1;                                     //结果=1
            wasHandled = true;                              //已经处理好了=真
            break;

            default:
                /*
                * 防止当Explorer.exe 崩溃以后，程序在系统系统托盘中的图标就消失
                *
                * 原理：Explorer.exe 重新载入后会重建系统任务栏。当系统任务栏建立的时候会向系统内所有
                * 注册接收TaskbarCreated 消息的顶级窗口发送一条消息，我们只需要捕捉这个消息，并重建系
                * 统托盘的图标即可。
                */
                if (message == WM_TASKBARCREATED)
                {
                    //系统Explorer崩溃重启时，重新加载托盘  
                    Shell_NotifyIcon(NIM_ADD, &trayIcon);
                }
                break;
            }
        }
        if (!wasHandled)                                    //没处理好
        {
            result = DefWindowProc(hwnd, message, wParam, lParam);   //调用默认窗口过程，为应用程序不处理的任何窗口消息提供默认处理。
        }
    }
    return result;
}
//-----------------------------------------------------------------
//运行消息循环
void    DemoApp::Run()
{
    //帧率
    m_timer.Reset();
    MSG msg = { 0 };
    while (msg.message != WM_QUIT)
    {    //用来结束程序运行或当程序调用postquitmessage函数 
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))    //调度传入的非排队消息，检查线程消息队列中是否存在已发布的消息，并检索 (消息（如果存在任何) ）。PM_REMOVE.PeekMessage 处理后，将从队列中删除消息。
        {
            TranslateMessage(&msg);                   // 把按键消息传递给字符消息
            DispatchMessage(&msg);                    // 将消息分派给窗口程序
        }
        else
        {
            m_timer.Tick();
            //UpdateScene(m_timer.DeltaTime());        //实现你想做的每帧执行的操作
            CalculateFrameStats();
            OnRender();
            Sleep(1);
        }
    }
}
//-----------------------------------------------------------------
//渲染
HRESULT    DemoApp::OnRender()
{
    int KeyCode, subscript;
    HRESULT hr;

    if (m_pD2DDeviceContext != nullptr)       //如果成功（hr）&&指示与此呈现目标关联的 HWND 是否被遮挡。窗口被遮挡。
    {
        static RECT  rWindow;
        GetWindowRect(m_hwnd, &rWindow);
        // 开始绘制
        m_pD2DDeviceContext->BeginDraw();    //启动对此呈现目标的绘图。开始画
        m_pD2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
        m_pD2DDeviceContext->Clear();        // 清屏

        float bitmapW = m_pD2DDeviceContext->GetSize().width;
        float bitmapH = m_pD2DDeviceContext->GetSize().height;

        // 绘制
        m_pD2DDeviceContext->DrawBitmap(m_pBitmap[0], D2D1::RectF(0, 0, bitmapW, bitmapH), 1, D2D1_INTERPOLATION_MODE_LINEAR, D2D1::RectF(0, 0, bitmapW, bitmapH), 0);

        for (subscript = 0; subscript <= 85; subscript++)
        {
            KeyCode = key[subscript];
            if (GetAsyncKeyState(KeyCode) & 0x8000)
            {
                m_pD2DDeviceContext->DrawBitmap(m_pBitmap[subscript + 1], { 0, 0, bitmapW, bitmapH }, 1, D2D1_INTERPOLATION_MODE_LINEAR, { 0, 0, bitmapW, bitmapH }, 0);
            }
        }

        // 结束绘制
        hr = m_pD2DDeviceContext->EndDraw();    // 结束绘制
        if (FAILED(hr))                         //没什么用，我的理解是结束失败时候多一层保险
        {
            DiscardDeviceResources();
        }

        // 呈现目标
        m_pSwapChain->Present(0, 0);       // 呈现目标
        ValidateRect(m_hwnd, 0);           // 通过从指定窗口的更新区域中删除矩形来验证矩形中的工作区。

    }
    return hr;
}

//-----------------------------------------------------------------
//创建托盘
void InitTray(HWND hwnd)
{
    memset(&trayIcon, 0, sizeof(NOTIFYICONDATA));
    trayIcon.cbSize           = sizeof(NOTIFYICONDATA);          //结构大小
    trayIcon.hIcon            = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SMALL)); //定义图标，IDI_SMALL-图标资源
    trayIcon.hWnd             = hwnd;                            //关联窗口的句柄
    trayIcon.uCallbackMessage = MYWM_NOTIFYICON;                 //托盘菜单消息
    trayIcon.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;//标志
    trayIcon.uID              = IDR_MAINFRAME;                   //标识符
    lstrcpy(trayIcon.szTip, L"");                                //提示信息
    //生成托盘菜单
    hMenu = CreatePopupMenu();
    //添加菜单,关键在于设置的一个标识符
    AppendMenu(hMenu, MF_STRING,    ID_SHOW,      _T("显示 / 隐藏"));
    AppendMenu(hMenu, MF_STRING,    ID_SOUND,     _T("音效"));
    AppendMenu(hMenu, MF_SEPARATOR, 0,            NULL);         //分割条
    AppendMenu(hMenu, MF_STRING,    ID_TOP,       _T("置顶"));
    AppendMenu(hMenu, MF_STRING,    ID_PENETARTE, _T("穿透"));
    AppendMenu(hMenu, MF_SEPARATOR, 0,            NULL);         //分割条
    AppendMenu(hMenu, MF_STRING,    ID_ABOUT,     _T("关于..."));
    AppendMenu(hMenu, MF_STRING,    ID_QUIT,      _T("退出"));

    Shell_NotifyIcon(NIM_ADD, &trayIcon);//发送消息
}
//-----------------------------------------------------------------
// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

//-----------------------------------------------------------------
// 按键音效线程
// https://github.com/Git1Sunny/TapSound
void keysound()
{
    map<int, bool> keymap;
    for (int i = 8; i < 254; i++) {
        keymap.insert({ i,false });
    }
    while (true)
    {
        for (auto it = keymap.begin(); it != keymap.end(); it++)
        {
            if (GetAsyncKeyState(it->first) && !it->second)
            {
                keymap[it->first] = true;
                switch (it->first)
                {
                case VK_C:
                    if (GetAsyncKeyState(VK_LCONTROL)) {
                        PlaySound(MAKEINTRESOURCE(IDR_WAVE3), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
                        break;
                    }
                case VK_V:
                    if (GetAsyncKeyState(VK_LCONTROL)) {
                        PlaySound(MAKEINTRESOURCE(IDR_WAVE4), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
                        break;
                    }
                case VK_A:
                case VK_B:

                case VK_D:
                case VK_E:
                case VK_F:
                case VK_G:
                case VK_H:
                case VK_I:
                case VK_J:
                case VK_K:
                case VK_L:
                case VK_M:
                case VK_N:
                case VK_O:
                case VK_P:
                case VK_Q:
                case VK_R:
                case VK_S:
                case VK_T:
                case VK_U:

                case VK_W:
                case VK_X:
                case VK_Y:
                case VK_Z:
                case VK_LEFT:
                case VK_UP:
                case VK_RIGHT:
                case VK_DOWN:
                    PlaySound(MAKEINTRESOURCE(IDR_WAVE2), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
                    break;
                case VK_F1:
                case VK_F2:
                case VK_F3:
                case VK_F4:
                case VK_F5:
                case VK_F6:
                case VK_F7:
                case VK_F8:
                case VK_F9:
                case VK_F10:
                case VK_F11:
                case VK_F12:
                    PlaySound(MAKEINTRESOURCE(IDR_WAVE3), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
                    break;
                case VK_RETURN:
                    PlaySound(MAKEINTRESOURCE(IDR_WAVE4), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
                    break;
                case VK_SPACE:
                    PlaySound(MAKEINTRESOURCE(IDR_WAVE5), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
                    break;

                default:
                    PlaySound(MAKEINTRESOURCE(IDR_WAVE1), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
                    break;
                }
            }
            if (!GetAsyncKeyState(it->first) && it->second) {
                keymap[it->first] = false;
            }
        }
        if (xianchengguanbi)
            return;
        Sleep(20);
    }

}

//-----------------------------------------------------------------
// 禁止重复运行
void  ForbiddenRepeatedlyRun()
{
    if (OpenEventA(EVENT_ALL_ACCESS, false, "WR_KEY") != NULL)
    {
        exit(0);
    }
    CreateEvent(NULL, false, false, L"WR_KEY");
}

//-----------------------------------------------------------------
//淡入淡出效果
void SetMouseIn(HWND hwnd, int TouMing)
{
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, 0, TouMing, LWA_ALPHA);
}
//-----------------------------------------------------------------
//前台窗口是否全屏
bool CheckFullscreen()
{
    bool bFullScreen = false;
    HWND hWnd = GetForegroundWindow();
    RECT rcApp, rcDesk;
    GetWindowRect(GetDesktopWindow(), &rcDesk);
    if (hWnd != GetDesktopWindow() && hWnd != GetShellWindow()) {
        GetWindowRect(hWnd, &rcApp);
        if (rcApp.left <= rcDesk.left
            && rcApp.top <= rcDesk.top
            && rcApp.right >= rcDesk.right
            && rcApp.bottom >= rcDesk.bottom)
        {
            bFullScreen = true;
        }
    }
    return bFullScreen;
}
/*
//-----------------------------------------------------------------
//调整大小
void    DemoApp::OnResize(UINT width, UINT height)
{
    if (m_pRT)         //呈现器
    {
        D2D1_SIZE_U size;        //尺寸
        size.width = width;      //宽
        size.height = height;    //高

        // Note: This method can fail, but it's okay to ignore the
        // error here -- it will be repeated on the next call to
        // EndDraw.注意: 这个方法可能会失败，但是可以忽略这里的错误——它将在下次调用 EndDraw 时重复出现
        m_pRT->Resize(size);    //调整大小
    }
}


//安装钩子
BOOL InstallHook() {
    //安装钩子
    hkKey = SetWindowsHookEx(WH_KEYBOARD_LL, &ProcessHook, HINST_THISCOMPONENT, NULL);

    if (hkKey == NULL)
    {
        MessageBox(nullptr, L"安装钩子失败!", L"错误", MB_OK);
        return false;
    }


    return true;
}

//卸载钩子
BOOL UninstallHook() {
    return UnhookWindowsHookEx(hkKey);
}

//钩子函数
LRESULT CALLBACK ProcessHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    bool flag = false;
    KBDLLHOOKSTRUCT* ks = (KBDLLHOOKSTRUCT*)lParam; // 包含低级键盘输入事件信息
    PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)(lParam);//获取回调函数
    if (ks->flags & 0b10000000)
    if (nCode == HC_ACTION)
        switch (wParam)
        {
        case WM_KEYDOWN:case WM_KEYUP:case WM_SYSKEYDOWN:case WM_SYSKEYUP:
            PKBDLLHOOKSTRUCT pk = (PKBDLLHOOKSTRUCT)lParam;
            if ((pk->vkCode == VK_RETURN) && (pk->flags & LLKHF_EXTENDED))
                pk->vkCode = VK_SEPARATOR;
            MessageBox(nullptr, L"安装钩子失败!", L"错误", MB_OK);
            break;
        }
    /*
    PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)(lParam);
    if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)	//检测键盘输入
    {
    flag = true;
    MessageBox(nullptr, L"安装钩子失败!", L"错误", MB_OK);
    //按键被按下
    switch (p->vkCode)
    {
        case VK_BACK: out << "<BK>"<<endl; break;

    return CallNextHookEx(NULL, nCode, wParam, lParam);
    return 0;
}
*/
