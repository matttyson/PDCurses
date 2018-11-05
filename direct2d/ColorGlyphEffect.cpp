#include "ColorGlyphEffect.hpp"
#include "ColorGlyphShader.hpp"

#include "ColorGlyphEffectShader.h"

HRESULT ColorGlyphEffect::SetForegroundColor(D2D_VECTOR_3F foreground)
{
    m_colors.fg.r = foreground.x;
    m_colors.fg.g = foreground.y;
    m_colors.fg.b = foreground.z;

    return S_OK;
}

D2D_VECTOR_3F ColorGlyphEffect::GetForegroundColor()const
{
    D2D_VECTOR_3F vector = {m_colors.fg.r, m_colors.fg.g, m_colors.fg.b};
    return vector;
}

HRESULT ColorGlyphEffect::SetBackgroundColor(D2D_VECTOR_3F background)
{
    m_colors.bg.r = background.x;
    m_colors.bg.g = background.y;
    m_colors.bg.b = background.z;
    return S_OK;
}

D2D_VECTOR_3F ColorGlyphEffect::GetBackgroundColor()const
{
    D2D_VECTOR_3F vector = { m_colors.bg.r, m_colors.bg.g, m_colors.bg.b };
    return vector;
}

IFACEMETHODIMP ColorGlyphEffect::Initialize(
    _In_ ID2D1EffectContext *pEffectContext,
    _In_ ID2D1TransformGraph *pTransformGraph
)
{
    m_effectContext = pEffectContext;

    // TODO Load the pixel shader.
    HRESULT hr = pEffectContext->LoadPixelShader(GUID_ColorGlyphShader, g_ColorChangeShader, sizeof(g_ColorChangeShader));

    if(SUCCEEDED(hr)){
        hr = pTransformGraph->SetSingleTransformNode(this);
    }

    return hr;
}

IFACEMETHODIMP ColorGlyphEffect::SetGraph(_In_ ID2D1TransformGraph *pGraph)
{
    return E_NOTIMPL;
}

IFACEMETHODIMP ColorGlyphEffect::PrepareForRender(D2D1_CHANGE_TYPE changeType)
{
    return UpdateConstants();
}

HRESULT ColorGlyphEffect::Register(_In_ ID2D1Factory1* pFactory)
{
    const D2D1_PROPERTY_BINDING bindings[] =
    {
        D2D1_VALUE_TYPE_BINDING(
            TEXT("ForegroundColor"),
            &SetForegroundColor,
            &GetForegroundColor
        ),
        D2D1_VALUE_TYPE_BINDING(
            TEXT("BackgroundColor"),
            &SetBackgroundColor,
            &GetBackgroundColor
        )
    };

    PCWSTR pszXML = TEXT(
        "<?xml version=\"1.0\"?>"
        "<Effect>"
        "<!--System Properties-->"
        "<Property name=\"DisplayName\" type=\"string\" value=\"ColorGlyphEffect\"/>"
        "<Property name=\"Author\" type=\"string\" value=\"Matt Tyson\"/>"
        "<Property name=\"Category\" type=\"string\" value=\"Sample\"/>"
        "<Property name=\"Description\" type=\"string\" value=\"Applies Color settings to a glyph during drawing.\"/>"
        "<Inputs>"
        "<Input name=\"Source\"/>"
        "</Inputs>"
        "<!--Custom Properties go here. -->"
        "<Property name=\"ForegroundColor\" type=\"vector3\">"
        "<Property name=\"DisplayName\" type=\"string\" value=\"foreground color value\"/>"
        "<Property name=\"Min\" type=\"vector3\" value=\"(0.0, 0.0, 0.0)\"/>"
        "<Property name=\"Max\" type=\"vector3\" value=\"(1.0, 1.0, 1.0)\"/>"
        "<Property name=\"Default\" type=\"vector3\" value=\"(0.0, 0.0, 0.0)\"/>"
        "</Property>"
        "<Property name=\"BackgroundColor\" type=\"vector3\">"
        "<Property name=\"DisplayName\" type=\"string\" value=\"background color value\"/>"
        "<Property name=\"Min\" type=\"vector3\" value=\"(0.0, 0.0, 0.0)\"/>"
        "<Property name=\"Max\" type=\"vector3\" value=\"(1.0, 1.0, 1.0)\"/>"
        "<Property name=\"Default\" type=\"vector3\" value=\"(0.0, 0.0, 0.0)\"/>"
        "</Property>"
        "</Effect>");

    HRESULT hr = pFactory->RegisterEffectFromString(
        CLSID_ColorGlyphEffect,
        pszXML,
        bindings,
        ARRAYSIZE(bindings),
        CreateColorGlyphEffect
    );

    return hr;
}

HRESULT __stdcall ColorGlyphEffect::CreateColorGlyphEffect(_Outptr_ IUnknown** ppEffectImpl)
{
    *ppEffectImpl = static_cast<ID2D1EffectImpl*>(new ColorGlyphEffect());

    if(*ppEffectImpl == nullptr){
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

IFACEMETHODIMP ColorGlyphEffect::QueryInterface(REFIID riid, void** ppOutput)
{
    if(!ppOutput){
        return E_INVALIDARG;
    }

    *ppOutput = nullptr;
    HRESULT hr = S_OK;

    if (riid == __uuidof(ID2D1EffectImpl))
    {
        *ppOutput = reinterpret_cast<ID2D1EffectImpl*>(this);
    }
    else if (riid == __uuidof(ID2D1DrawTransform))
    {
        *ppOutput = static_cast<ID2D1DrawTransform*>(this);
    }
    else if (riid == __uuidof(ID2D1Transform))
    {
        *ppOutput = static_cast<ID2D1Transform*>(this);
    }
    else if (riid == __uuidof(ID2D1TransformNode))
    {
        *ppOutput = static_cast<ID2D1TransformNode*>(this);
    }
    else if (riid == __uuidof(IUnknown))
    {
        *ppOutput = this;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    if (*ppOutput != nullptr)
    {
        AddRef();
    }

    return hr;
}

IFACEMETHODIMP_(ULONG) ColorGlyphEffect::AddRef()
{
    InterlockedIncrement(&m_refCount);
    return m_refCount;
}

IFACEMETHODIMP_(ULONG) ColorGlyphEffect::Release()
{
    InterlockedDecrement(&m_refCount);

    if(m_refCount == 0){
        delete this;
        return 0;
    }
    
    return m_refCount;
}

HRESULT ColorGlyphEffect::UpdateConstants()
{
    // Update the DPI if it has changed. This allows the effect to scale across different DPIs automatically.
    //m_effectContext->GetDpi(&m_dpi, &m_dpi);
    //m_constants.dpi = m_dpi;

    return m_drawInfo->SetPixelShaderConstantBuffer(
        reinterpret_cast<BYTE*>(&m_colors),
        sizeof(m_colors)
    );
}

IFACEMETHODIMP ColorGlyphEffect::SetDrawInfo(_In_ ID2D1DrawInfo* pDrawInfo)
{
    m_drawInfo = pDrawInfo;

    return m_drawInfo->SetPixelShader(GUID_ColorGlyphShader);
}

// Map the input to the output rect.
// We do not change the size of the rect, so we can just assign input to output.
IFACEMETHODIMP ColorGlyphEffect::MapOutputRectToInputRects(
    _In_ const D2D1_RECT_L* pOutputRect,
    _Out_writes_(inputRectCount) D2D1_RECT_L* pInputRects,
    UINT32 inputRectCount
) const
{
    if (inputRectCount != 1) {
        return E_INVALIDARG;
    }

    pInputRects[0] = *pOutputRect;

    return S_OK;
}


IFACEMETHODIMP ColorGlyphEffect::MapInputRectsToOutputRect(
    _In_reads_(inputRectCount) CONST D2D1_RECT_L* pInputRects,
    _In_reads_(inputRectCount) CONST D2D1_RECT_L* pInputOpaqueSubRects,
    UINT32 inputRectCount,
    _Out_ D2D1_RECT_L* pOutputRect,
    _Out_ D2D1_RECT_L* pOutputOpaqueSubRect
)
{
    if(inputRectCount != 1){
        return E_INVALIDARG;
    }

    *pOutputRect = pInputRects[0];
    m_inputRect = pInputRects[0];

    return S_OK;
}

IFACEMETHODIMP ColorGlyphEffect::MapInvalidRect(
    UINT32 inputIndex,
    D2D1_RECT_L invalidInputRect,
    _Out_ D2D1_RECT_L* pInvalidOutputRect
)const
{
    HRESULT hr = S_OK;

    // Indicate that the entire output may be invalid.
    *pInvalidOutputRect = m_inputRect;

    return hr;
}

IFACEMETHODIMP_(UINT32) ColorGlyphEffect::GetInputCount() const
{
    return 1;
}

ColorGlyphEffect::ColorGlyphEffect()
    :m_refCount(1)
{
    ZeroMemory(&m_colors, sizeof(m_colors));
}
