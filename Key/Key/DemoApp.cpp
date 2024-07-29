#include "DemoApp.h"
#define KEY_DOWN(VK) ((GetAsyncKeyState(VK)& 0x8000 ? 1:0))
#define KEYUP(VK) ((GetAsyncKeyState(VK) & 0x8000) ? 0 : 1)
#define KEY_DOWN_FOREGROUND(hWnd,vk) (KEY_DOWN(vk) && GetForegroundWindow() == hWnd) //ǰ�������ж�
#pragma warning(disable:4996)

/***********************************************************************
���ܣ�Direct2D ������ʾ
���ߣ��ֺ���
***********************************************************************/

//-----------------------------------------------------------------
//��������

void InitTray(HWND hWnd);
void  keysound();
void  ForbiddenRepeatedlyRun();
void SetMouseIn(HWND hwnd, int TouMing);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
bool CheckFullscreen();
/*
//˫����ʾ�������õģ�Ӧ����ȡ�ź���
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

//��װ����
extern "C" __declspec(dllexport) BOOL InstallHook();

//ж�ع���
extern "C" __declspec(dllexport) BOOL UninstallHook();

//���Ӵ�����
LRESULT CALLBACK ProcessHook(int nCode, WPARAM wParam, LPARAM lParam);
*/
//-----------------------------------------------------------------
//-----------------------------------------------------------------
//����ȫ�ֱ���
bool xianchengguanbi = false;
double          width = 413, height = 149;             //���ڿ��

double          ScrWid = GetSystemMetrics(SM_CXSCREEN); //��Ļ��
double          ScrHei = GetSystemMetrics(SM_CYSCREEN); //��Ļ��
//double          ScrWid    = GetSystemMetrics(SM_CXFULLSCREEN); //��������������Ļ��,���ǲ�׼ȷ
//double          ScrHei    = GetSystemMetrics(SM_CYFULLSCREEN); //��������������Ļ��
//��ȡ���������С
RECT rect;



int             Alpha;                                     //ʱ�������õ��ĵ���Ч��͸��ֵ
HANDLE          hThread;
UINT            nElapse1, uElapse2;                         //ʱ������
//��������
NOTIFYICONDATA  trayIcon;                                  //��������  
HMENU           hMenu;                                     //���̲˵�


// ����Alphaͨ�����
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
DemoAppʵ��
******************************************************************/

DemoApp::DemoApp() : //��ʾӦ�ó���
    m_hwnd(NULL),
    m_pD2DFactory(NULL),            //d2d����
    m_pWICFactory(NULL)             //WIC����
    //m_pDWriteFactory(NULL)        //DW����
{
    //TFEffect = 0;

    memset(                         //��ĳһ���ڴ��е�����ȫ������Ϊָ����ֵ
        &rc,                        //ָ��Ҫ�����ڴ������ָ�롣
        0,                          //Ҫ���õ�ֵ��ͨ����һ���޷����ַ���
        sizeof(rc));                //Ҫ������Ϊ��ֵ���ַ�����
}

DemoApp::~DemoApp()
{
    this->DiscardDeviceResources();
    SafeRelease(&m_pWICFactory);                 //�ͷ� COM �ӿ�ָ�롣
    SafeRelease(&m_pD2DFactory);
    //SafeRelease(&m_pDWriteFactory);
}

//-----------------------------------------------------------------
// ��Ӧ�ó�����Դ�ļ�����ͼ�񡣴���Դ�ļ�����D2Dλͼ
//-----------------------------------------------------------------
HRESULT DemoApp::LoadResourceBitmap(                    //������Դλͼ HRESULT�Ǻ�������ֵ
    ID2D1RenderTarget* pRenderTarget,                   //��ȾĿ��
    IWICImagingFactory* pIWICFactory,                   //WIC����
    PCWSTR resourceName,                                //��һ��ָ�룬�Ǹ�������   ��Դ��   
    PCWSTR resourceType,                                //��Դ����
    UINT destinationWidth,                              //�����������Σ�����Ŀ��ճ����Ŀ�
    UINT destinationHeight,                             //�����������Σ�����Ŀ��ճ����ĸ�
    ID2D1Bitmap** ppBitmap)                             //��ʾ�Ѱ󶨵� ID2D1RenderTarget ��λͼ

{
    IWICBitmapDecoder* pDecoder = NULL;                 //������ʾ�������ķ�����
    IWICBitmapFrameDecode* pSource = NULL;              //��֡���ӿڣ����ڷ���ʵ��ͼ��λ
    IWICStream* pStream = NULL;                         //��ʾ��������ӳ���Ԫ�������ݵ� WINDOWS ӳ����� (WIC) ��
    IWICFormatConverter* pConverter = NULL;             //��ʾһ�� IWICBitmapSource(�������ô��м������ص��޷�д�ص�Դ�ķ�����) ������ͼ�����ݴ�һ�����ظ�ʽת��Ϊ��һ�����ظ�ʽ������������ʽ�Ķ����Ͱ������ɫ��ת���� alpha ��ֵ��
    IWICBitmapScaler* pScaler = NULL;                   //��ʾʹ���ز�����ɸѡ�㷨��������λͼ�Ĵ�С�汾��

    HRSRC imageResHandle = NULL;                        //��Դ�����ͼ�������
    HGLOBAL imageResDataHandle = NULL;                  //�ڴ���,ͼ�����ݴ������
    void* pImageFile = NULL;                            //������ָ�룬ͼƬ�ļ�
    DWORD imageFileSize = 0;                            //32λ�޷�������

    // Locate the resource.ȷ����Դ
    imageResHandle = FindResourceW(                     //ȷ��ָ��ģ���о���ָ�����ͺ����Ƶ���Դ��λ�á�
        HINST_THISCOMPONENT,                            //ģ��ľ���������ֲ��ִ���ļ����渽�� MUI �ļ�������Դ��
        resourceName,                                   //��Դ�����ơ�
        resourceType);                                  //��Դ����
    HRESULT hr = imageResHandle ? S_OK : E_FAIL;        //HRESULT�Ǻ�������ֵ,��Դ���,S_OK��COM������������ȷ,E_FAIL�Ǳ��봦��Ĵ��󡣱��ʽ?(���ʽΪ��ʱ��ֵ):(���ʽΪ��ʱ��ֵ)
    if (SUCCEEDED(hr))         //����ɹ���hr��
    {
        // Load the resource.������Դ
        imageResDataHandle = LoadResource(              //�ڴ�����������Դ�����������ڻ�ȡָ���ڴ���ָ����Դ��һ���ֽڵ�ָ��ľ����
            HINST_THISCOMPONENT,                        //���ִ���ļ�������Դ��ģ��ľ����
            imageResHandle);                            //��Դ���

        hr = imageResDataHandle ? S_OK : E_FAIL;        //�ڴ���,���ʽ?(���ʽΪ��ʱ��ֵ):(���ʽΪ��ʱ��ֵ)
    }



    //3.������Դ������ͼ��Ĵ�С��
    if (SUCCEEDED(hr))        //����ɹ���hr��
    {
        // Lock it to get a system memory pointer.�������Ի��һ��ϵͳ�ڴ�ָ��
        pImageFile = LockResource(imageResDataHandle); //ͼƬ�ļ�,����ָ���ڴ���ָ����Դ��ָ��(�ڴ���)

        hr = pImageFile ? S_OK : E_FAIL;               //�ڴ���
    }
    if (SUCCEEDED(hr))                                 //����ɹ���hr��
    {
        // Calculate the size.����ߴ�
        imageFileSize = SizeofResource(                //ͼ���ļ���С
            HINST_THISCOMPONENT,                       //���ִ���ļ�������Դ��ģ��ľ����
            imageResHandle);                           //��Դ���

        hr = imageFileSize ? S_OK : E_FAIL;            //��Դ��� ,���ʽ?(���ʽΪ��ʱ��ֵ):(���ʽΪ��ʱ��ֵ)
    }





    // 4.ʹ�� IWICImagingFactory::CreateStream �������� IWICStream ����
    if (SUCCEEDED(hr))                 //����ɹ���hr��
    {
        // Create a WIC stream to map onto the memory.����                һ�� WIC ��ӳ�䵽�ڴ�
        hr = pIWICFactory->CreateStream(&pStream);    // ���� IWICStream �����ʵ����
    }
    if (SUCCEEDED(hr))               //����ɹ���hr��
    {
        // Initialize the stream with the memory pointer and size.ʹ���ڴ�ָ��ʹ�С��ʼ����
        hr = pStream->InitializeFromMemory(          //��ʼ�����Խ��ڴ����Ϊ���� ������������������������С��
            reinterpret_cast<BYTE*>(pImageFile),     //�����κ�ָ��ת��Ϊ�κ�����ָ�����͡� Ҳ�����κ���������ת��Ϊ�κ�ָ�������Լ�����ת����//ָ�����ڳ�ʼ�����Ļ�������ָ�롣
            imageFileSize                            //�������Ĵ�С��
        );
    }




    //5.ʹ�� IWICImagingFactory::CreateDecoderFromStream �������� IWICBitmapDecoder��
    if (SUCCEEDED(hr))                              //����ɹ���hr��
    {
        // Create a decoder for the stream.Ϊ������������
        hr = pIWICFactory->CreateDecoderFromStream( //���ڸ����� IStream ���� IWICBitmapDecoder �����ʵ����
            pStream,                                //Ҫ���д�������������
            NULL,                                   //��ѡ��������Ӧ�̵� GUID�� ���û����ѡ��Ӧ�̣���ʹ�� NULL ��
            WICDecodeMetadataCacheOnLoad,           //����������ʱҪʹ�õ� WICDecodeOptions (ָ������ѡ���
            &pDecoder                               //����ָ���� IWICBitmapDecoder(������ʾ�������ķ���) ��ָ���ָ�롣
        );
    }



    //6.��ͼ���м���ĳһ֡������֡�洢�� IWICBitmapFrameDecode(��֡���ӿڣ����ڷ���ʵ��ͼ��λ) �����С�
    if (SUCCEEDED(hr))                             //����ɹ���hr��
    {
        // Create the initial frame.������ʼ֡
        hr = pDecoder->GetFrame(                   //����ͼ���ָ��֡��
            0,                                     //Ҫ�������ض�֡��
            &pSource);                             //һ��ָ�룬���ڽ���ָ�� IWICBitmapFrameDecode(����ʵ��ͼ��λ) ��ָ�롣
    }




    /*7.�����Ƚ�ͼ��ת��Ϊ 32bppPBGRA ���ظ�ʽ��Ȼ�� Direct2D ����ʹ�ø�ͼ��
        ��Ҫת��ͼ���ʽ����ʹ��IWICImagingFactory::CreateFormatConverter ������IWICFormatConverter ����
        Ȼ��ʹ�� IWICFormatConverter �����Initialize ����ִ��ת��*/
    if (SUCCEEDED(hr))                            //����ɹ���hr��
    {
        // Convert the image format to 32bppPBGRA.��ͼ���ʽת��Ϊ32bppPBGRA
        // (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).ֵ�� 88 һ���ķ��� 32 λ�޷��Ź淶��������ʽ��ÿ����ɫͨ��֧�� 8 λ��8 λδʹ�á�+ֵ��1alpha ֵ��Ԥ�ˡ� ÿ����ɫ���Ȱ� alpha ֵ���š� alpha ֵ������ֱ�˺�Ԥ�� alpha ������ͬ�ġ� ͨ����û����ɫͨ��ֵ���� alpha ͨ��ֵ�� �������Ԥ�˸�ʽ����ɫͨ��ֵ���� alpha ͨ�������׼Դ������ѧ�������ӷ���ϡ�
        hr = pIWICFactory->CreateFormatConverter(&pConverter);  //���� IWICFormatConverter �����ʵ����
    }
    if (SUCCEEDED(hr))                            //����ɹ���hr��
    {

        hr = pConverter->Initialize(              //��ʼ����ʽת����
            pSource,                              //Ҫת��������λͼ
            GUID_WICPixelFormat32bppPBGRA,        //Ŀ�����ظ�ʽ GUID��
            WICBitmapDitherTypeNone,              //����ת���� WICBitmapDitherType(ָ����ͼ���ʽ֮��ת��ʱҪӦ�õ� dither �㷨������) ��
            NULL,                                 //����ת���ĵ�ɫ�塣
            0.f,                                  //����ת���� alpha ��ֵ��
            WICBitmapPaletteTypeMedianCut         //����ת���ĵ�ɫ��ת�����͡�
        );
    }




    /*8.���ʹ�� CreateBitmapFromWicBitmap �������� ID2D1Bitmap ����
        �ö����ͨ��������Ŀ����Ʋ������� Direct2D ����һ��ʹ�á�*/
    if (SUCCEEDED(hr))                           //����ɹ���hr��
    {
        //create a Direct2D bitmap from the WIC bitmap.�� WIC λͼ���� Direct2D λͼ
        hr = pRenderTarget->CreateBitmapFromWicBitmap(   //ͨ������ָ���� Microsoft Windows ӳ����� (WIC) λͼ������ ID2D1Bitmap��
            pConverter,                          //Ҫ���Ƶ� WIC λͼ��
            NULL,                                //Ҫ������λͼ�����ظ�ʽ�� DPI��
            ppBitmap                             //�˷�������ʱ������ָ����λͼ��ָ��ĵ�ַ�� �˲���δ����ʼ���������ݡ�
        );

    }

    SafeRelease(&pDecoder);                     //�ͷ� COM �ӿ�ָ�롣
    SafeRelease(&pSource);
    SafeRelease(&pStream);
    SafeRelease(&pConverter);
    SafeRelease(&pScaler);

    return hr;
}
//-----------------------------------------------------------------
// ��ʼ����������
HRESULT DemoApp::Initialize()
{
    ForbiddenRepeatedlyRun();
    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)keysound, NULL, 0, NULL);//�����߳�
    SuspendThread(hThread);  //�̹߳���
    //thread t2(keysound);   //������Ч�߳�
    //t2.detach();           //������Ч�߳�
    HRESULT hr;              //����һ������
    //��ȡ���������С
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
    double          ScrWidZ = rect.right - rect.left;
    double          ScrHeiZ = rect.bottom - rect.top;
    //register window class��ע�ᴰ����
    WNDCLASSEX wcex    = { sizeof(WNDCLASSEX) };
    wcex.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;   //������ʽ
    wcex.lpfnWndProc   = DemoApp::WndProc;                       //��Ϣ������
    wcex.cbClsExtra    = 0;                                      //������ṹ��ĸ����ֽ���
    wcex.cbWndExtra    = sizeof(LONG_PTR);
    wcex.hInstance     = HINST_THISCOMPONENT;                    //�������ڵ�ģ����
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);            //�����ʽ
    wcex.hbrBackground = NULL;                                   // (HBRUSH)CreateSolidBrush(0x000000);     //������ˢ���
    wcex.lpszMenuName  = NULL;                                   //�˵�
    wcex.lpszClassName = L"Key";                                 //����������
    wcex.hIcon         = LoadIconW(0, IDI_APPLICATION);          //����ͼ��
    wcex.hIconSm       = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));//����Сͼ��

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(nullptr, L"ע�ᴰ����ʧ��!", L"����", MB_OK);
        return 0;
    }

    // ����Ӧ�ó��򴰿�

    hr = CreateDeviceIndependentResources(); //�������豸�޹ص���Դ
    if (SUCCEEDED(hr))
    {
        CreateDeviceResources();            //�����豸��Դ
        // ��Ϊ CreateWindow �����Ĵ�С������Ϊ��λ
        // ���ǵõ�ϵͳ�� DPI �����������Ŵ��ڴ�С
        FLOAT dpiX, dpiY;    //������
        //������ǰ����ÿӢ�����(DPI) �� ��Ҫˢ�´�ֵ������� ReloadSystemMetrics��
        m_pD2DFactory->GetDesktopDpi(&dpiX, &dpiY);
        double x = ScrWidZ - width;                               //���ڳ�ʼλ��
        double y = ScrHeiZ - height;
        m_hwnd = CreateWindowExA(
            WS_EX_NOACTIVATE,                                     //������չ��ʽ
            "Key",                                                //��������
            m_wndCaption.c_str(),                                 //���ڱ���
            WS_VISIBLE | WS_POPUP,                                //������ʽ
            x,                                                    //���ڳ�ʼ��ĻX����
            y,                                                    //���ڳ�ʼ��ĻX����
            static_cast<UINT>(ceil(413.f * dpiX / 96.f)),         //���ڳ�ʼ���
            static_cast<UINT>(ceil(149.f * dpiX / 96.f)),         //���ڳ�ʼ�߶�
            NULL,                                                 //��������
            NULL,                                                 //�˵��ľ�������Ӵ��ڵı�ʶ��
            HINST_THISCOMPONENT,                                  //Ӧ�ó���ʵ���ľ��
            this                                                  //ָ�򴰿ڵĴ�������
        );

        if (m_hwnd == nullptr)
        {
            MessageBox(nullptr, L"��������ʧ��!", L"����", MB_OK);
            return 0;
        }

        // ����Alphaͨ�����
        EnableAlphaCompositing(m_hwnd);

        nElapse1 = 10;                          //ʱ������
        SetTimer(m_hwnd, 1, nElapse1, NULL);    //����ʱ��


        hr = m_hwnd ? S_OK : E_FAIL;            //���ھ��
        if (SUCCEEDED(hr))                      //����ɹ���hr��
        {
            CreateWindowSizeDependentResources();
            ShowWindow(m_hwnd, SW_SHOWNORMAL);  //��ʾ����
            UpdateWindow(m_hwnd);               //������ڵĸ�������Ϊ�գ� UpdateWindow ����ͨ���򴰿ڷ��� WM_PAINT ��Ϣ������ָ�����ڵĹ�������
        }
    }
    InitTray(m_hwnd);                       //ʵ��������  
    return hr;                              //����
}
//-----------------------------------------------------------------
//�������豸�޹ص���Դ(����D2D������WIC������
HRESULT DemoApp::CreateDeviceIndependentResources()
{
    HRESULT hr;                                  //��������hr

    // ����D2D����
    hr = D2D1CreateFactory(                      //���������ڴ��� Direct2D ��Դ�Ĺ�������
        D2D1_FACTORY_TYPE_SINGLE_THREADED,       //�������߳�ģ�ͼ��䴴������Դ��
        &m_pD2DFactory);                         //��ʹ�� ��ȡ�� ID2D1Factory �� IID �� __uuidof(ID2D1Factory)���á�

    /*if (m_pDWriteFactory == NULL && SUCCEEDED(hr))//���DW����û�ˣ��ҳɹ���hr��
    {
        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pDWriteFactory),
            reinterpret_cast<IUnknown**>(&m_pDWriteFactory));
    }*/

    if (m_pWICFactory == NULL && SUCCEEDED(hr))  //���WIC����û�ˣ��ҳɹ���hr��
    {
        if (SUCCEEDED(                           //����ɹ�
            CoCreateInstance(                    //������Ĭ�ϳ�ʼ����ָ�� CLSID ��������ĵ�������
                CLSID_WICImagingFactory2,        //�뽫���ڴ�����������ݺʹ�������� CLSID��
                nullptr,                         //��� Ϊ NULL����ָʾ��������Ϊ�ۺϵ�һ���ִ����ġ� ������� NULL����ָ��ۺ϶���� IUnknown �ӿڵ�ָ��
                CLSCTX_INPROC_SERVER,            //�����´�������Ĵ��뽫���������е������ġ� ��Щֵȡ��ö�� CLSCTX��
                IID_PPV_ARGS(&m_pWICFactory)     //��Ҫ���������ͨ�ŵĽӿڱ�ʶ�������á�              
            )
        ))
            return true;                         //����z
    }

    return hr;                                   //������Դ���
}
//-----------------------------------------------------------------
//�����豸��Դ�� ���� D2D�豸��DXGI�豸��D2D�豸������ ��
HRESULT DemoApp::CreateDeviceResources()
{

    IDXGIFactory2* pDxgiFactory = nullptr;    // DXGI ����

    IDXGIDevice1* pDxgiDevice   = nullptr;    // DXGI �豸

    HRESULT            hr = S_OK;


    // ���� D3D11�豸���豸������ 
    if (SUCCEEDED(hr))
    {
        // D3D11 ����flag 
        // һ��Ҫ��D3D11_CREATE_DEVICE_BGRA_SUPPORT�����򴴽�D2D�豸�����Ļ�ʧ��
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
        // Debug״̬ ��D3D DebugLayer�Ϳ���ȡ��ע��
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
        // �����豸
        hr = D3D11CreateDevice(
            nullptr,					// ��Ϊ��ָ��ѡ��Ĭ���豸
            D3D_DRIVER_TYPE_HARDWARE,   // ǿ��ָ��Ӳ����Ⱦ
            nullptr,					// ǿ��ָ��WARP��Ⱦ D3D_DRIVER_TYPE_WARP û������ӿ�
            creationFlags,				// ����flag
            featureLevels,				// ��ʹ�õ����Եȼ��б�
            ARRAYSIZE(featureLevels),   // ���Եȼ��б���
            D3D11_SDK_VERSION,		    // SDK �汾
            &m_pD3DDevice,				// ���ص�D3D11�豸ָ��
            &m_featureLevel,			// ���ص����Եȼ�
            &m_pD3DDeviceContext);      // ���ص�D3D11�豸������ָ��
    }

    // ���� IDXGIDevice
    if (SUCCEEDED(hr))
        hr = m_pD3DDevice->QueryInterface(IID_PPV_ARGS(&pDxgiDevice));
    // ����D2D�豸
    if (SUCCEEDED(hr))
        hr = m_pD2DFactory->CreateDevice(pDxgiDevice, &m_pD2DDevice);
    // ����D2D�豸������
    if (SUCCEEDED(hr))
        hr = m_pD2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_pD2DDeviceContext);

    SafeRelease(&pDxgiDevice);
    SafeRelease(&pDxgiFactory);

    return hr;
}
//-----------------------------------------------------------------
//�����봰�ڴ�С��ص���Դ
HRESULT    DemoApp::CreateWindowSizeDependentResources()
{
    // ���ǵõ�ϵͳ�� DPI �����������Ŵ��ڴ�С
    FLOAT dpiX, dpiY;    //������

    IDXGIAdapter* pDxgiAdapter    = nullptr;      // DXGI ������

    IDXGIFactory2* pDxgiFactory   = nullptr;      // DXGI ����

    IDXGISurface* pDxgiBackBuffer = nullptr;      // DXGI Surface ��̨����

    IDXGIDevice1* pDxgiDevice     = nullptr;      // DXGI �豸

    HRESULT hr = S_OK;

    // ���֮ǰ���ڵĳ���������豸
    m_pD2DDeviceContext->SetTarget(nullptr);

    m_pD3DDeviceContext->Flush();

    RECT rect = { 0 }; GetClientRect(m_hwnd, &rect);

    if (m_pSwapChain != nullptr)
    {
        // ����������Ѿ������������軺����
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
        // �������Ѵ��ڵ�D3D�豸����һ���µĽ�����
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
        swapChainDesc.Width              = lround(rect.right - rect.left);         //���
        swapChainDesc.Height             = lround(rect.bottom - rect.top);         //�߶�
        swapChainDesc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM;             //��ʽ
        swapChainDesc.Stereo             = false;                                  //ȫ��������
        swapChainDesc.SampleDesc.Count   = 1;                                      //����_����
        swapChainDesc.SampleDesc.Quality = 0;                                      //����_����
        swapChainDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;        //ʹ�÷���(DXGI_�÷�_��ȾĿ�����)
        swapChainDesc.BufferCount        = 2;                                      //��������
        swapChainDesc.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;               //Ч��(DXGI_����Ч��_����)
        swapChainDesc.Flags              = 0;                                      //��־
        swapChainDesc.Scaling            = DXGI_SCALING_STRETCH;                   //����
        swapChainDesc.AlphaMode          = DXGI_ALPHA_MODE_IGNORE;                 //͸��ģʽ

        // ��ȡ IDXGIDevice
        if (SUCCEEDED(hr))
        {
            hr = m_pD3DDevice->QueryInterface(IID_PPV_ARGS(&pDxgiDevice));
        }
        // ��ȡDxgi������ ���Ի�ȡ����������Ϣ
        if (SUCCEEDED(hr))
        {
            hr = pDxgiDevice->GetAdapter(&pDxgiAdapter);
        }
        // ��ȡDxgi����
        if (SUCCEEDED(hr))
        {
            hr = pDxgiAdapter->GetParent(IID_PPV_ARGS(&pDxgiFactory));
        }
        // ����������
        if (SUCCEEDED(hr))
        {
            hr = pDxgiFactory->CreateSwapChainForHwnd(      //�����������Դ���
                m_pD3DDevice,                               //����_DXGI�豸
                m_hwnd,                                     //����_���ھ��
                &swapChainDesc,                             //�������ṹ
                nullptr,                                    //
                nullptr,                                    //
                &m_pSwapChain);                             //����_DXGI������
        }
        // ȷ��DXGI������߲��ᳬ��һ֡
        if (SUCCEEDED(hr))
        {
            hr = pDxgiDevice->SetMaximumFrameLatency(1);
        }
    }

    // ������Ļ����
    /*if (SUCCEEDED(hr))
    {
        hr = m_pSwapChain->SetRotation(DXGI_MODE_ROTATION_IDENTITY);
    }*/
    // ���ý�������ȡDxgi����
    if (SUCCEEDED(hr))
    {
        hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pDxgiBackBuffer));
    }

    //������ǰ����ÿӢ�����(DPI) �� ��Ҫˢ�´�ֵ������� ReloadSystemMetrics��
    m_pD2DFactory->GetDesktopDpi(&dpiX, &dpiY);
    // ����Dxgi���洴��λͼ
    if (SUCCEEDED(hr))
    {
        D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,    //λͼѡ��
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,                    //���ظ�ʽ
                D2D1_ALPHA_MODE_PREMULTIPLIED),                              //͸��ģʽ
            dpiX,
            dpiY);
        hr = m_pD2DDeviceContext->CreateBitmapFromDxgiSurface(
            pDxgiBackBuffer,
            &bitmapProperties,
            &m_pD2DTargetBimtap);
    }
    // ����
    if (SUCCEEDED(hr))
    {
        // ���� Direct2D ��ȾĿ��
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
// �����豸��Դ
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
// ����ÿ��ƽ��֡���Ĵ��룬�������˻���һ֡��ƽ��ʱ��
unsigned int hour = 0, minute = 0, second = 0;
void    DemoApp::CalculateFrameStats()
{
    // ����ÿ��ƽ��֡���Ĵ��룬�������˻���һ֡��ƽ��ʱ��
    // ��Щͳ����Ϣ����ʾ�ڴ��ڱ�������
    static int frameCnt = 0;
    static float timeElapsed = 0.0f;

    frameCnt++;

    unsigned int timer = clock() / CLOCKS_PER_SEC;
    second = (timer % 60);
    minute = (timer / 60) % 60;
    hour   = (timer / 60) / 60;

    // ����һ��ʱ���ڵ�ƽ��ֵ
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

        // Ϊ�˼�����һ��ƽ��ֵ����һЩֵ
        frameCnt = 0;
        timeElapsed += 1.0f;
    }
}
//-----------------------------------------------------------------
//������Ϣ����
LRESULT CALLBACK DemoApp::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;                                     //���=0
    DemoApp* pDemoApp;

    bool wasHandled = false;                            //�Ѿ��������=�� 

    UINT state;                                         //�жϴ����ö���͸��״̬

    static POINT OldPos, NewPos;                        //������λ�ã�����ȡ���ƫ�������ƶ����ڣ�����ȡ���ƫ���������Ŵ���
    static double   xOffset, yOffset;                   //����ƫ�������������Ŵ���
    static RECT  rWindow;                               //���ھ��Σ��������Ŵ��ڣ������ƶ����ں���ʾ������
    double whb = width / height;                        //���ڵĿ�߱ȣ�����̶����ڱ���
    double          ScrWidZ = rect.right - rect.left;
    double          ScrHeiZ = rect.bottom - rect.top;
    // ��Ҫ�޸�TaskbarCreated������ϵͳ�������Զ������Ϣ
    UINT WM_TASKBARCREATED = RegisterWindowMessage(TEXT("TaskbarCreated"));

    if (message == WM_CREATE)                               //���ô��ڳ�ʼ�����룬�罨�������Ӵ��ڣ�״̬���͹������ȣ�
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;        //���崫�ݸ�Ӧ�ó���Ĵ��ڹ��̵ĳ�ʼ�������� ��Щ��Ա�� CreateWindowEx �����Ĳ�����ͬ��
        DemoApp* pDemoApp = (DemoApp*)pcs->lpCreateParams;  //��������

        ::SetWindowLongPtrW(                                //����ָ�����ڵ����ԡ�
            hwnd,                                           //���ڵľ��
            GWLP_USERDATA,                                  //Ҫ���õ�ֵ�Ĵ��㿪ʼ��ƫ������ ��Чֵ�����㵽���ⴰ���ڴ���ֽ���֮�䣬��ȥ LONG_PTR�Ĵ�С�� ��Ҫ�����κ�����ֵ����ָ������ֵ֮һ��
            PtrToUlong(pDemoApp)                            //�滻ֵ��
        );

        result = 1;                                         //���=1
    }
    else
    {
        DemoApp* pDemoApp = reinterpret_cast<DemoApp*>(static_cast<LONG_PTR>(     // //�����κ�ָ��ת��Ϊ�κ�����ָ�����͡� Ҳ�����κ���������ת��Ϊ�κ�ָ�������Լ�����ת����//ָ�����ڳ�ʼ�����Ļ�������ָ�롣
            ::GetWindowLongPtrW(                            //�����й�ָ�����ڵ���Ϣ�� �������Ὣָ��ƫ������ֵ����������Ĵ����ڴ��С�
                hwnd,                                       //���ڵľ��
                GWLP_USERDATA                               //Ҫ������ֵ�Ĵ��㿪ʼ��ƫ������ ��Чֵ�����㵽���ⴰ���ڴ���ֽ���֮�䣬��ȥ LONG_PTR�Ĵ�С�� ��Ҫ�����κ�����ֵ����ָ������ֵ֮һ��
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
            case WM_SIZE:                                   //���ڴ�С���ı�
            {
                UINT width = LOWORD(lParam);                //������������
                UINT height = HIWORD(lParam);
                pDemoApp->OnResize(width, height);          //������С
            }

            case WM_DISPLAYCHANGE:                          //��ʾ���ķֱ��ʸı���ʹ���Ϣ�����еĴ���
            {
                InvalidateRect(hwnd, 0, 0);//���´���
            }
            result = 0;                                     //���=0
            wasHandled = true;                              //�Ѿ��������=��
            break;
            */

            case WM_TIMER:                     //��ʱ������
            {

                if (wParam == 1)

                {
                    if (Alpha > 250)
                    {
                        nElapse1 = 0;
                        KillTimer(hwnd, 1);  //�ͷ�ʱ��
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
            result = 1;                                     //���=1
            wasHandled = true;                              //�Ѿ��������=��
            break;
            case WM_LBUTTONDOWN:                            //�������
            {
                SetCapture(hwnd);
                OldPos.x = LOWORD(lParam);
                OldPos.y = HIWORD(lParam);
            }
            result = 1;                                     //���=1
            wasHandled = true;                              //�Ѿ��������=��
            break;

            case WM_LBUTTONUP:                              //̧�����
            {
                ReleaseCapture();
                if (!CheckFullscreen())//���û��ȫ������  
                {
                    if (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CAPTION)
                    {
                        if (rWindow.bottom > ScrHeiZ)//��
                            SetWindowPos(hwnd, NULL, rWindow.left, ScrHeiZ + rWindow.top - rWindow.bottom + 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0)//��
                            SetWindowPos(hwnd, NULL, -8, rWindow.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.top < 0)//��
                            SetWindowPos(hwnd, NULL, rWindow.left, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.right > ScrWidZ)//��
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right + 8, rWindow.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

                        if (rWindow.right > ScrWidZ && rWindow.bottom > ScrHeiZ)//����
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right + 8, ScrHeiZ + rWindow.top - rWindow.bottom + 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0 && rWindow.bottom > ScrHeiZ)//����
                            SetWindowPos(hwnd, NULL, -8, ScrHeiZ + rWindow.top - rWindow.bottom + 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.top < 0 && rWindow.right > ScrWidZ)//����
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right + 8, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0 && rWindow.top < 0)//����
                            SetWindowPos(hwnd, NULL, -8, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

                        if (rWindow.right - rWindow.left > ScrWidZ && rWindow.bottom - rWindow.top > ScrHeiZ)
                            SetWindowPos(hwnd, NULL, -8, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                    }
                    else
                    {
                        if (rWindow.bottom > ScrHeiZ)//��
                            SetWindowPos(hwnd, NULL, rWindow.left, ScrHeiZ + rWindow.top - rWindow.bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0)//��
                            SetWindowPos(hwnd, NULL, 0, rWindow.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.top < 0)//��
                            SetWindowPos(hwnd, NULL, rWindow.left, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.right > ScrWidZ)//��
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right, rWindow.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

                        if (rWindow.right > ScrWidZ && rWindow.bottom > ScrHeiZ)//����
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right, ScrHeiZ + rWindow.top - rWindow.bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0 && rWindow.bottom > ScrHeiZ)//����
                            SetWindowPos(hwnd, NULL, 0, ScrHeiZ + rWindow.top - rWindow.bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.top < 0 && rWindow.right > ScrWidZ)//����
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0 && rWindow.top < 0)//����
                            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

                        if (rWindow.right - rWindow.left > ScrWidZ && rWindow.bottom - rWindow.top > ScrHeiZ)//���ڿ������Ļ��򴰿ڸߴ�����Ļ��
                            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                    }
                    
                }


            }
            result = 1;                                     //���=1
            wasHandled = true;                              //�Ѿ��������=��
            break;

            case WM_RBUTTONDOWN:                            //�����Ҽ�
            {
                SetCapture(hwnd);
                GetCursorPos(&OldPos);

            }
            result = 1;                                     //���=1
            wasHandled = true;                              //�Ѿ��������=��
            break;

            case WM_RBUTTONUP:                              //̧���Ҽ�
            {
                ReleaseCapture();

                if (!CheckFullscreen())//���û��ȫ������  
                {
                    if (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CAPTION)
                    {
                        if (rWindow.bottom > ScrHeiZ)//��
                            SetWindowPos(hwnd, NULL, rWindow.left, ScrHeiZ + rWindow.top - rWindow.bottom + 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0)//��
                            SetWindowPos(hwnd, NULL, -8, rWindow.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.top < 0)//��
                            SetWindowPos(hwnd, NULL, rWindow.left, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.right > ScrWidZ)//��
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right + 8, rWindow.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

                        if (rWindow.right > ScrWidZ && rWindow.bottom > ScrHeiZ)//����
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right + 8, ScrHeiZ + rWindow.top - rWindow.bottom + 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0 && rWindow.bottom > ScrHeiZ)//����
                            SetWindowPos(hwnd, NULL, -8, ScrHeiZ + rWindow.top - rWindow.bottom + 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.top < 0 && rWindow.right > ScrWidZ)//����
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right + 8, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0 && rWindow.top < 0)//����
                            SetWindowPos(hwnd, NULL, -8, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

                        if (rWindow.right - rWindow.left > ScrWidZ && rWindow.bottom - rWindow.top > ScrHeiZ)
                            SetWindowPos(hwnd, NULL, -8, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                    }
                    else
                    {
                        if (rWindow.bottom > ScrHeiZ)//��
                            SetWindowPos(hwnd, NULL, rWindow.left, ScrHeiZ + rWindow.top - rWindow.bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0)//��
                            SetWindowPos(hwnd, NULL, 0, rWindow.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.top < 0)//��
                            SetWindowPos(hwnd, NULL, rWindow.left, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.right > ScrWidZ)//��
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right, rWindow.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

                        if (rWindow.right > ScrWidZ && rWindow.bottom > ScrHeiZ)//����
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right, ScrHeiZ + rWindow.top - rWindow.bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0 && rWindow.bottom > ScrHeiZ)//����
                            SetWindowPos(hwnd, NULL, 0, ScrHeiZ + rWindow.top - rWindow.bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.top < 0 && rWindow.right > ScrWidZ)//����
                            SetWindowPos(hwnd, NULL, ScrWidZ + rWindow.left - rWindow.right, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        if (rWindow.left < 0 && rWindow.top < 0)//����
                            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

                        if (rWindow.right - rWindow.left > ScrWidZ && rWindow.bottom - rWindow.top > ScrHeiZ)//���ڿ������Ļ��򴰿ڸߴ�����Ļ��
                            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                    }
                }
            }
            result = 1;                                     //���=1
            wasHandled = true;                              //�Ѿ��������=��
            break;

            //����ƶ�
            case WM_MOUSEMOVE:
            {
                GetWindowRect(hwnd, &rWindow);
                if (GetCapture() == hwnd && wParam == MK_LBUTTON)   //��������ƶ�����
                {
                    SetWindowPos(hwnd,
                        NULL, rWindow.left + (short)LOWORD(lParam) - OldPos.x,
                        rWindow.top + (short)HIWORD(lParam) - OldPos.y,
                        0,
                        0,
                        SWP_NOSIZE | SWP_NOZORDER);
                }
                if (GetCapture() == hwnd && wParam == MK_RBUTTON)   //�Ҽ��������Ŵ���
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
                        if (xOffset * xOffset > yOffset * yOffset)     //����ƶ���ȴ��ڸ߶�
                        {
                            MoveWindow(hwnd,
                                rWindow.left,
                                rWindow.top,
                                (rWindow.right + xOffset - rWindow.left),
                                (rWindow.right + xOffset - rWindow.left) / whb,
                                false);
                        }
                        else                                           //����ƶ���Ȳ����ڸ߶�
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
            result = 1;                                     //���=1
            wasHandled = true;                              //�Ѿ��������=��
            break;

            //���ƴ����ƶ���Χ 
            case WM_WINDOWPOSCHANGING:
            {
                WINDOWPOS* pWinPos = (WINDOWPOS*)lParam;
                if (CheckFullscreen())//�����ȫ������      
                {
                    if (pWinPos->x > ScrWid - (rWindow.right - rWindow.left))//��
                        pWinPos->x = ScrWid - (rWindow.right - rWindow.left);

                    if (pWinPos->y > ScrHei - (rWindow.bottom - rWindow.top))//��
                        pWinPos->y = ScrHei - (rWindow.bottom - rWindow.top);

                    if (pWinPos->x < 0)     //��
                        pWinPos->x = 0;

                    if (pWinPos->y < 0)     //��
                        pWinPos->y = 0;
                }


                if (pWinPos->cx < 13)  //������С�ߴ�
                    pWinPos->cx = 13;
                if (pWinPos->cy < 5)
                    pWinPos->cy = 5;

                return 0;
            }
            result = 1;                                     //���=1
            wasHandled = true;                              //�Ѿ��������=��
            break;

            //˫�������ö�
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
            result = 1;                                     //���=1
            wasHandled = true;                              //�Ѿ��������=��    
            break;

            //˫���Ҽ����ڴ�͸
            case WM_RBUTTONDBLCLK:
            {
                CheckMenuItem(hMenu, ID_PENETARTE, MF_CHECKED | MF_BYCOMMAND);
                SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
            }
            result = 1;                                     //���=1
            wasHandled = true;                              //�Ѿ��������=��    
            break;


            /*
            case WM_CLOSE:                                  //�������ڲ����� WM_QUIT
            {
                DestroyWindow(hwnd);                        //����ָ���Ĵ���
            }
            break;
            */

            //���̲˵���Ϣ
            case MYWM_NOTIFYICON:                           //���̲˵���Ϣ
            {
                if (lParam == WM_RBUTTONDOWN)
                {
                    //��ȡ�������
                    POINT pt; GetCursorPos(&pt);
                    //�һ�����ؿ���������һ������Ĳ˵���
                    SetForegroundWindow(hwnd);
                    //�����˵�,�����û���ѡ�˵���ı�ʶ������
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
                        SetWindowPos(hwnd,            //��ԭ���ڴ�С
                            NULL,
                            0,
                            0,
                            width,
                            height,
                            SWP_NOMOVE | SWP_NOZORDER);
                        if (!IsWindowVisible(hwnd))
                        {
                            ShowWindowAsync(hwnd, true);  //��ʾ����
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
                if (lParam == WM_LBUTTONDBLCLK)       //˫�����,��ʾ������
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

                if (lParam == WM_LBUTTONDOWN)         //�����������
                {
                    SetForegroundWindow(hwnd);        //�����ý���
                }
            }
            result = 1;                                     //���=1
            wasHandled = true;                              //�Ѿ��������=��
            break;

            //���ڼ������ݻ�
            case WM_DESTROY:                                //���ڼ������ݻ�
            {
                KillTimer(hwnd, 2);                         //�ͷ�ʱ��
                xianchengguanbi = true;
                CloseHandle(hThread);
                DestroyMenu(hMenu);                         //����˵�
                Shell_NotifyIcon(NIM_DELETE, &trayIcon);    //��������ͼ��
                PostQuitMessage(0);                         //���ٴ���
            }
            result = 1;                                     //���=1
            wasHandled = true;                              //�Ѿ��������=��
            break;

            default:
                /*
                * ��ֹ��Explorer.exe �����Ժ󣬳�����ϵͳϵͳ�����е�ͼ�����ʧ
                *
                * ԭ��Explorer.exe �����������ؽ�ϵͳ����������ϵͳ������������ʱ�����ϵͳ������
                * ע�����TaskbarCreated ��Ϣ�Ķ������ڷ���һ����Ϣ������ֻ��Ҫ��׽�����Ϣ�����ؽ�ϵ
                * ͳ���̵�ͼ�꼴�ɡ�
                */
                if (message == WM_TASKBARCREATED)
                {
                    //ϵͳExplorer��������ʱ�����¼�������  
                    Shell_NotifyIcon(NIM_ADD, &trayIcon);
                }
                break;
            }
        }
        if (!wasHandled)                                    //û�����
        {
            result = DefWindowProc(hwnd, message, wParam, lParam);   //����Ĭ�ϴ��ڹ��̣�ΪӦ�ó��򲻴�����κδ�����Ϣ�ṩĬ�ϴ���
        }
    }
    return result;
}
//-----------------------------------------------------------------
//������Ϣѭ��
void    DemoApp::Run()
{
    //֡��
    m_timer.Reset();
    MSG msg = { 0 };
    while (msg.message != WM_QUIT)
    {    //���������������л򵱳������postquitmessage���� 
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))    //���ȴ���ķ��Ŷ���Ϣ������߳���Ϣ�������Ƿ�����ѷ�������Ϣ�������� (��Ϣ����������κ�) ����PM_REMOVE.PeekMessage ����󣬽��Ӷ�����ɾ����Ϣ��
        {
            TranslateMessage(&msg);                   // �Ѱ�����Ϣ���ݸ��ַ���Ϣ
            DispatchMessage(&msg);                    // ����Ϣ���ɸ����ڳ���
        }
        else
        {
            m_timer.Tick();
            //UpdateScene(m_timer.DeltaTime());        //ʵ����������ÿִ֡�еĲ���
            CalculateFrameStats();
            OnRender();
            Sleep(1);
        }
    }
}
//-----------------------------------------------------------------
//��Ⱦ
HRESULT    DemoApp::OnRender()
{
    int KeyCode, subscript;
    HRESULT hr;

    if (m_pD2DDeviceContext != nullptr)       //����ɹ���hr��&&ָʾ��˳���Ŀ������� HWND �Ƿ��ڵ������ڱ��ڵ���
    {
        static RECT  rWindow;
        GetWindowRect(m_hwnd, &rWindow);
        // ��ʼ����
        m_pD2DDeviceContext->BeginDraw();    //�����Դ˳���Ŀ��Ļ�ͼ����ʼ��
        m_pD2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
        m_pD2DDeviceContext->Clear();        // ����

        float bitmapW = m_pD2DDeviceContext->GetSize().width;
        float bitmapH = m_pD2DDeviceContext->GetSize().height;

        // ����
        m_pD2DDeviceContext->DrawBitmap(m_pBitmap[0], D2D1::RectF(0, 0, bitmapW, bitmapH), 1, D2D1_INTERPOLATION_MODE_LINEAR, D2D1::RectF(0, 0, bitmapW, bitmapH), 0);

        for (subscript = 0; subscript <= 85; subscript++)
        {
            KeyCode = key[subscript];
            if (GetAsyncKeyState(KeyCode) & 0x8000)
            {
                m_pD2DDeviceContext->DrawBitmap(m_pBitmap[subscript + 1], { 0, 0, bitmapW, bitmapH }, 1, D2D1_INTERPOLATION_MODE_LINEAR, { 0, 0, bitmapW, bitmapH }, 0);
            }
        }

        // ��������
        hr = m_pD2DDeviceContext->EndDraw();    // ��������
        if (FAILED(hr))                         //ûʲô�ã��ҵ�����ǽ���ʧ��ʱ���һ�㱣��
        {
            DiscardDeviceResources();
        }

        // ����Ŀ��
        m_pSwapChain->Present(0, 0);       // ����Ŀ��
        ValidateRect(m_hwnd, 0);           // ͨ����ָ�����ڵĸ���������ɾ����������֤�����еĹ�������

    }
    return hr;
}

//-----------------------------------------------------------------
//��������
void InitTray(HWND hwnd)
{
    memset(&trayIcon, 0, sizeof(NOTIFYICONDATA));
    trayIcon.cbSize           = sizeof(NOTIFYICONDATA);          //�ṹ��С
    trayIcon.hIcon            = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SMALL)); //����ͼ�꣬IDI_SMALL-ͼ����Դ
    trayIcon.hWnd             = hwnd;                            //�������ڵľ��
    trayIcon.uCallbackMessage = MYWM_NOTIFYICON;                 //���̲˵���Ϣ
    trayIcon.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;//��־
    trayIcon.uID              = IDR_MAINFRAME;                   //��ʶ��
    lstrcpy(trayIcon.szTip, L"");                                //��ʾ��Ϣ
    //�������̲˵�
    hMenu = CreatePopupMenu();
    //��Ӳ˵�,�ؼ��������õ�һ����ʶ��
    AppendMenu(hMenu, MF_STRING,    ID_SHOW,      _T("��ʾ / ����"));
    AppendMenu(hMenu, MF_STRING,    ID_SOUND,     _T("��Ч"));
    AppendMenu(hMenu, MF_SEPARATOR, 0,            NULL);         //�ָ���
    AppendMenu(hMenu, MF_STRING,    ID_TOP,       _T("�ö�"));
    AppendMenu(hMenu, MF_STRING,    ID_PENETARTE, _T("��͸"));
    AppendMenu(hMenu, MF_SEPARATOR, 0,            NULL);         //�ָ���
    AppendMenu(hMenu, MF_STRING,    ID_ABOUT,     _T("����..."));
    AppendMenu(hMenu, MF_STRING,    ID_QUIT,      _T("�˳�"));

    Shell_NotifyIcon(NIM_ADD, &trayIcon);//������Ϣ
}
//-----------------------------------------------------------------
// �����ڡ������Ϣ�������
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
// ������Ч�߳�
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
// ��ֹ�ظ�����
void  ForbiddenRepeatedlyRun()
{
    if (OpenEventA(EVENT_ALL_ACCESS, false, "WR_KEY") != NULL)
    {
        exit(0);
    }
    CreateEvent(NULL, false, false, L"WR_KEY");
}

//-----------------------------------------------------------------
//���뵭��Ч��
void SetMouseIn(HWND hwnd, int TouMing)
{
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, 0, TouMing, LWA_ALPHA);
}
//-----------------------------------------------------------------
//ǰ̨�����Ƿ�ȫ��
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
//������С
void    DemoApp::OnResize(UINT width, UINT height)
{
    if (m_pRT)         //������
    {
        D2D1_SIZE_U size;        //�ߴ�
        size.width = width;      //��
        size.height = height;    //��

        // Note: This method can fail, but it's okay to ignore the
        // error here -- it will be repeated on the next call to
        // EndDraw.ע��: ����������ܻ�ʧ�ܣ����ǿ��Ժ�������Ĵ��󡪡��������´ε��� EndDraw ʱ�ظ�����
        m_pRT->Resize(size);    //������С
    }
}


//��װ����
BOOL InstallHook() {
    //��װ����
    hkKey = SetWindowsHookEx(WH_KEYBOARD_LL, &ProcessHook, HINST_THISCOMPONENT, NULL);

    if (hkKey == NULL)
    {
        MessageBox(nullptr, L"��װ����ʧ��!", L"����", MB_OK);
        return false;
    }


    return true;
}

//ж�ع���
BOOL UninstallHook() {
    return UnhookWindowsHookEx(hkKey);
}

//���Ӻ���
LRESULT CALLBACK ProcessHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    bool flag = false;
    KBDLLHOOKSTRUCT* ks = (KBDLLHOOKSTRUCT*)lParam; // �����ͼ����������¼���Ϣ
    PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)(lParam);//��ȡ�ص�����
    if (ks->flags & 0b10000000)
    if (nCode == HC_ACTION)
        switch (wParam)
        {
        case WM_KEYDOWN:case WM_KEYUP:case WM_SYSKEYDOWN:case WM_SYSKEYUP:
            PKBDLLHOOKSTRUCT pk = (PKBDLLHOOKSTRUCT)lParam;
            if ((pk->vkCode == VK_RETURN) && (pk->flags & LLKHF_EXTENDED))
                pk->vkCode = VK_SEPARATOR;
            MessageBox(nullptr, L"��װ����ʧ��!", L"����", MB_OK);
            break;
        }
    /*
    PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)(lParam);
    if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)	//����������
    {
    flag = true;
    MessageBox(nullptr, L"��װ����ʧ��!", L"����", MB_OK);
    //����������
    switch (p->vkCode)
    {
        case VK_BACK: out << "<BK>"<<endl; break;

    return CallNextHookEx(NULL, nCode, wParam, lParam);
    return 0;
}
*/
