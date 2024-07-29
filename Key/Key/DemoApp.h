#pragma once
/***********************************************************************
���ܣ�Direct2D 1.1Ӧ�ó����࣬ʹ��ʱ���Լ̳������
���ߣ�Ray1024
��ַ��http://www.cnblogs.com/Ray1024/
***********************************************************************/
#include "framework.h"
#include "D2D1Timer.h"

/******************************************************************
��Ҫ�õ��ĺ궨��
******************************************************************/
//ģ�壬���ͷ�ָ�� ppT ����������Ϊ���� NULL��
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
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)//��__ImageBase�ĵ�ַǿ������ת��ΪHINSTANCE����
#endif


#include <math.h>

/******************************************************************
DemoApp
******************************************************************/
class DemoApp   //��
{
public:                                                       // �����������������߷��ʼ���
    DemoApp();                                                // ���캯��
    ~DemoApp();                                               // ��������

    HRESULT Initialize();                                     //��ʼ����������

    void Run();                                    //������Ϣѭ��

private:
    HRESULT CreateDeviceIndependentResources();               //�������豸�޹ص���Դ(����D2D������WIC������

    HRESULT CreateDeviceResources();                          //�����豸��Դ�� ���� D2D�豸��DXGI�豸��D2D�豸������ ��

    HRESULT CreateWindowSizeDependentResources();                //�����봰�ڴ�С��ص���Դ

    void DiscardDeviceResources();                            // �����豸�����Դ

    void CalculateFrameStats();                               // ����֡����Ϣ

    void UpdateScene(float dt) {};                            // ������д�˺�����ʵ����������ÿִ֡�еĲ���

    HRESULT OnRender();                                          //����Ⱦ��

    //void OnResize(UINT width, UINT height);                   //������С

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message,  //���ڹ���
        WPARAM wParam, LPARAM lParam);

    HRESULT LoadResourceBitmap(                               //������Դλͼ HRESULT�Ǻ�������ֵ
        ID2D1RenderTarget*   pRenderTarget,                   //��ȾĿ��
        IWICImagingFactory*  pIWICFactory,                    //WIC����
        PCWSTR               resourceName,                    //��һ��ָ�룬�Ǹ�������   ��Դ��   
        PCWSTR               resourceType,                    //��Դ����
        UINT                 destinationWidth,                //�����������Σ�����Ŀ��ճ����Ŀ�
        UINT                 destinationHeight,               //�����������Σ�����Ŀ��ճ����ĸ�
        ID2D1Bitmap**        ppBitmap);                       //��ʾ�Ѱ󶨵� ID2D1RenderTarget ��λͼ

protected:
    // �õ�����Դ-----------------------------------------------------

    D2D1Timer                m_timer;                       // ���ڼ�¼deltatime����Ϸʱ��

    BOOL                     m_fRunning = TRUE;             // �Ƿ�����

    HWND                     m_hwnd;                        // ���ھ��

   

    std::string				 m_wndCaption = "";          // ���ڱ���

    ID2D1Bitmap*             m_pBitmap[87] = {};		    // λͼ

    ID3D11Device*            m_pD3DDevice;                  // D3D �豸

    ID3D11DeviceContext*     m_pD3DDeviceContext;           // D3D �豸������

    ID2D1Factory1*           m_pD2DFactory = nullptr;       // D2D ����

    ID2D1Device*             m_pD2DDevice = nullptr;        // D2D �豸

    ID2D1DeviceContext*      m_pD2DDeviceContext = nullptr; // D2D �豸������

    ID2D1HwndRenderTarget*   m_pRT;                         // ������

    IDWriteFactory*          m_pDWriteFactory;	            // DWrite����

    IWICImagingFactory2*     m_pWICFactory = nullptr;       // WIC ����

    IDXGISwapChain1*         m_pSwapChain = nullptr;        // DXGI ������

    ID2D1Bitmap1*            m_pD2DTargetBimtap = nullptr;  // D2D λͼ ���浱ǰ��ʾ��λͼ

    D3D_FEATURE_LEVEL        m_featureLevel;                //  �����豸���Եȼ�

    DXGI_PRESENT_PARAMETERS  m_parameters;                  // �ֶ�������

    RECT                     rc;                            // ��������rc   
    int key[86] = { 27, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 192, 49, 50, 51, 52, 53, 54,
        55, 56, 57, 48, 189, 187, 8, 9, 81, 87, 69, 82, 84, 89, 85, 73, 79, 80, 219, 221,
        220, 20, 65, 83, 68, 70, 71, 72, 74, 75, 76, 186, 222, 13, 160, 90, 88, 67, 86, 66,
        78, 77, 188, 190, 191, 161, 162, 91, 164, 32, 165, 93, 163, 44, 145, 19, 45, 36, 33, 46, 35, 34, 38, 37, 40, 39 };
};

