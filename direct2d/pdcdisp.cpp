
#include <stdlib.h>
#include <string.h>

#include "pdcd2d.h"

static chtype oldch = -1;
static short foregr = -2;
static short backgr = -2;

static bool pdc_update_fg = false;
static int pdc_color_fg_index = COLOR_WHITE | 8; // bold white
static int pdc_color_bg_index = 0;

void PDC_gotoyx(int row, int col)
{
    PDC_LOG((__FUNCTION__ " called\n"));
}


static void _set_colors(chtype ch)
{
    attr_t sysattrs = SP->termattrs;

    ch &= (A_COLOR | A_BOLD | A_BLINK | A_REVERSE);

    if (oldch != ch)
    {
        short newfg, newbg;

        if (SP->mono)
            return;

        pair_content(PAIR_NUMBER(ch), &newfg, &newbg);

        if ((ch & A_BOLD) && !(sysattrs & A_BOLD))
            newfg |= 8;
        if ((ch & A_BLINK) && !(sysattrs & A_BLINK))
            newbg |= 8;

        if (ch & A_REVERSE)
        {
            short tmp = newfg;
            newfg = newbg;
            newbg = tmp;
        }

        if (newfg != foregr)
        {
#ifndef PDC_WIDE
            const auto col = PDC_d2d_colors[newfg];

            D2D_VECTOR_3F vec = {
                col.r, col.g, col.b
            };

            HRESULT c = PDC_d2d_colorEffect->SetValueByName(
                TEXT("ForegroundColor"),
                D2D1_PROPERTY_TYPE_VECTOR3,
                (const BYTE*)&vec,
                sizeof(vec)
            );
#endif
            foregr = newfg;
        }

        if (newbg != backgr)
        {
            const auto col = PDC_d2d_colors[newbg];

            D2D_VECTOR_3F vec = {
                col.r, col.g, col.b
            };
            HRESULT c = PDC_d2d_colorEffect->SetValueByName(
                TEXT("BackgroundColor"),
                D2D1_PROPERTY_TYPE_VECTOR3,
                (const BYTE*)&vec,
                sizeof(vec)
            );
            backgr = newbg;
        }

        oldch = ch;
    }
}


static void _new_packet(attr_t attr, int lineno, int x, int len, const chtype *srcp)
{
    chtype ch;

    ch = *srcp & A_CHARTEXT;
    // line number can be determined by looking at the pixes per char

    // pdc_font_bitmap

    const int ybegin = PDC_D2D_CHAR_HEIGHT * lineno;
    const int xbegin = PDC_D2D_CHAR_WIDTH * x;
    const float raduis = 0.2f;

    const attr_t sysattrs = SP->termattrs;

    const bool do_blink = (attr & A_BLINK) && (sysattrs & A_BLINK);

    for (int i = 0; i < len; i++) {
        const float left = xbegin + i * PDC_D2D_CHAR_WIDTH;
        const float top = ybegin;
        const float right = left + PDC_D2D_CHAR_WIDTH;
        const float bottom = ybegin + PDC_D2D_CHAR_HEIGHT;

        int color = (srcp[i] & A_COLOR) >> PDC_COLOR_SHIFT;
        const chtype letter = srcp[i] & A_CHARTEXT;

        _set_colors(srcp[i]);

        const auto destination = D2D1::RectF(left, top, right, bottom);
        const auto dpoint = D2D1::Point2F(left, top);

        // Work out the area in the source glyph.

        const int source_row = letter / 32;
        const int source_col = letter % 32;

        const float sleft = source_col * PDC_D2D_CHAR_WIDTH;
        const float stop = source_row * PDC_D2D_CHAR_HEIGHT;

        const auto source = D2D1::RectF(sleft, stop, sleft+ PDC_D2D_CHAR_WIDTH, stop + PDC_D2D_CHAR_HEIGHT);
        
        // https://docs.microsoft.com/en-us/windows/desktop/direct2d/color-matrix
        // https://stackoverflow.com/questions/6347950/programmatically-creating-directx-11-textures-pros-and-cons-of-the-three-differ/6539561#6539561
        // https://stackoverflow.com/questions/9021244/how-to-work-with-pixels-using-direct2d

        //m_d2dContext->SetTransform((ID2D1DrawTransform*)m_colorGlyphEffect);

        PDC_d2d_context->DrawImage(
            PDC_d2d_colorEffect,
            dpoint,
            source,
            D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
            D2D1_COMPOSITE_MODE_SOURCE_OVER
        );

        /*
        m_d2dContext->DrawBitmap(
            pdc_font_bitmap,
            &destination,
            1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
            &source
        );*/
    }


}

#include <vector>

void PDC_transform_line(int lineno, int x, int len, const chtype *srcp)
{
    attr_t old_attr;

    int i = len;
    int j = 1;

    old_attr = *srcp & (A_ATTRIBUTES ^ A_ALTCHARSET);
    PDC_d2d_context->BeginDraw();
    _new_packet(old_attr, lineno, x, i, srcp);

    HRESULT hr = PDC_d2d_context->EndDraw();
    if(FAILED(hr)){
        PDC_LOG(("Failure\n"));
    }

    // https://code.msdn.microsoft.com/windowsapps/DXGI-swap-chain-rotation-21d13d71
    // https://docs.microsoft.com/en-gb/windows/desktop/api/dxgi1_2/nf-dxgi1_2-idxgiswapchain1-present1
    // https://docs.microsoft.com/en-gb/windows/desktop/api/dxgi1_2/ns-dxgi1_2-dxgi_present_parameters
    // https://docs.microsoft.com/en-us/windows/desktop/direct3ddxgi/dxgi-1-2-presentation-improvements
    
    // Calculate the area that we have drawn to
    DXGI_PRESENT_PARAMETERS params = {0};
    RECT dirty;

    dirty.left = x * PDC_D2D_CHAR_WIDTH;
    dirty.top = lineno * PDC_D2D_CHAR_HEIGHT;
    dirty.right = x * PDC_D2D_CHAR_WIDTH + len * PDC_D2D_CHAR_WIDTH;
    dirty.bottom = lineno * PDC_D2D_CHAR_HEIGHT + PDC_D2D_CHAR_HEIGHT;

    params.DirtyRectsCount = 1;
    params.pDirtyRects = &dirty;

    // Submit the buffer for drawing.
    hr = PDC_d2d_swapChain->Present1(1, 0, &params);
    if (FAILED(hr)) {
        PDC_LOG(("Failure\n"));
    }

    PDC_LOG((__FUNCTION__ " called\n"));
}

void PDC_doupdate(void)
{

}

void PDC_blink_text(void)
{
    PDC_LOG((__FUNCTION__ " called\n"));
}
