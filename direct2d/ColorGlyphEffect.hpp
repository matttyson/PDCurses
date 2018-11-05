// eb02bf10-0cca-41c9-bc0a-611b2e21f416
#pragma once

/*
    This is a pixel shader that will set the foreground and background
    Color of what we are rendering to the display.

    The input is expected to be a black and white image with the
    foreground being white, and the background being black.

    the white pixes will have the foreground Color applied, and the
    black pixels will have the background Color enabled.

    Requires DirectX 11 HLSL V5

    https://docs.microsoft.com/en-us/windows/desktop/direct2d/custom-effects
*/

#include <d2d1_1.h>
#include <d2d1effectauthor.h>  
#include <d2d1effecthelpers.h>

#include <wrl/client.h>

DEFINE_GUID(CLSID_ColorGlyphEffect,
    0xeb02bf10, 0x0cca, 0x41c9, 0xbc, 0x0a, 0x61, 0x1b, 0x2e, 0x21, 0xf4, 0x16);
DEFINE_GUID(GUID_ColorGlyphShader,
    0x3ca87c9f, 0xe668, 0x4eef, 0xee, 0x86, 0x57, 0xe7, 0xd2, 0x99, 0x1f, 0xe3);


class ColorGlyphEffect :
    public ID2D1EffectImpl,
    public ID2D1DrawTransform {
public:
    // 2.1 Declare ID2D1EffectImpl implementation methods.
    IFACEMETHODIMP Initialize(
        _In_ ID2D1EffectContext *pContextInternal,
        _In_ ID2D1TransformGraph *pTransformGraph
    );

    IFACEMETHODIMP PrepareForRender(D2D1_CHANGE_TYPE changeType);
    IFACEMETHODIMP SetGraph(_In_ ID2D1TransformGraph *pGraph);

    // 2.2 Declare effect registration methods.
    static HRESULT Register(_In_ ID2D1Factory1* pFactory);
    static HRESULT __stdcall CreateColorGlyphEffect(_Outptr_ IUnknown** ppEffectImpl);

    HRESULT SetForegroundColor(D2D_VECTOR_3F Colors); // RGB values
    D2D_VECTOR_3F GetForegroundColor()const;

    HRESULT SetBackgroundColor(D2D_VECTOR_3F Colors); // RGB values
    D2D_VECTOR_3F GetBackgroundColor()const;

    // Drawinfo method from ID2D1DrawTransform
    IFACEMETHODIMP SetDrawInfo(_In_ ID2D1DrawInfo* pRenderInfo);
    IFACEMETHODIMP MapOutputRectToInputRects(
        _In_ const D2D1_RECT_L* pOutputRect,
        _Out_writes_(inputRectCount) D2D1_RECT_L* pInputRects,
        UINT32 inputRectCount
    ) const;

    IFACEMETHODIMP MapInputRectsToOutputRect(
        _In_reads_(inputRectCount) CONST D2D1_RECT_L* pInputRects,
        _In_reads_(inputRectCount) CONST D2D1_RECT_L* pInputOpaqueSubRects,
        UINT32 inputRectCount,
        _Out_ D2D1_RECT_L* pOutputRect,
        _Out_ D2D1_RECT_L* pOutputOpaqueSubRect
    );

    IFACEMETHODIMP MapInvalidRect(
        UINT32 inputIndex,
        D2D1_RECT_L invalidInputRect,
        _Out_ D2D1_RECT_L* pInvalidOutputRect
    )const;
    IFACEMETHODIMP_(UINT32) GetInputCount() const;

    // 2.3 Declare IUnknown implementation methods.
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppOutput);

private:
    ColorGlyphEffect();
    HRESULT UpdateConstants();


    struct {
        struct {
            float r;
            float g;
            float b;
        } fg;
        struct {
            float r;
            float g;
            float b;
        } bg;
    } m_colors;

    ULONG m_refCount;

    D2D1_RECT_L m_inputRect;

    Microsoft::WRL::ComPtr<ID2D1DrawInfo> m_drawInfo;
    Microsoft::WRL::ComPtr<ID2D1EffectContext> m_effectContext;
};
