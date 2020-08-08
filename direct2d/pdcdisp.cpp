
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

struct line_transform
{
    line_transform(int lineno_, int x_, int len_, const chtype* srcp_)
        :lineno(lineno_), x(x_), len(len_)
    {
        srcp = new chtype[len];
        memcpy(srcp, srcp_, len * sizeof(chtype));
    }
    ~line_transform() {
        delete[] srcp;
    }

    line_transform(const line_transform& rhs) = delete;
    line_transform& operator=(const line_transform& rhs) = delete;

    line_transform(line_transform&& rhs) noexcept
        :lineno(rhs.lineno), x(rhs.x), len(rhs.len), srcp(rhs.srcp)
    {
        rhs.srcp = nullptr;
    }
    line_transform& operator=(line_transform&& rhs) noexcept {
        lineno = rhs.lineno;
        x = rhs.x;
        len = rhs.len;
        srcp = rhs.srcp;

        rhs.srcp = nullptr;

        return *this;
    }

    int lineno;
    int x;
    int len;
    chtype* srcp;
};

static std::vector<line_transform> transforms;

void PDC_transform_line(int lineno, int x, int len, const chtype *srcp)
{
    transforms.emplace_back(lineno, x, len, srcp);
    return;
}

static void D2D_draw_transform(int lineno, int x, int len, const chtype* srcp)
{
    attr_t old_attr = *srcp & (A_ATTRIBUTES & A_ALTCHARSET);

    int i = len;
    int j = 1;

    _new_packet(old_attr, lineno, x, i, srcp);

    // https://code.msdn.microsoft.com/windowsapps/DXGI-swap-chain-rotation-21d13d71
    // https://docs.microsoft.com/en-gb/windows/desktop/api/dxgi1_2/nf-dxgi1_2-idxgiswapchain1-present1
    // https://docs.microsoft.com/en-gb/windows/desktop/api/dxgi1_2/ns-dxgi1_2-dxgi_present_parameters
    // https://docs.microsoft.com/en-us/windows/desktop/direct3ddxgi/dxgi-1-2-presentation-improvements

    PDC_LOG((__FUNCTION__ " called\n"));
}

void PDC_doupdate(void)
{
    HRESULT hr;

    if (transforms.size() == 0) {
        return;
    }

    PDC_d2d_context->BeginDraw();

    RECT* rectlist = new RECT[transforms.size()];

    size_t i = 0;
    for (const auto& ts : transforms) {
        D2D_draw_transform(ts.lineno, ts.x, ts.len, ts.srcp);

        /* Calculate the dirty rects */
        rectlist[i].left = ts.x * PDC_D2D_CHAR_WIDTH;
        rectlist[i].top = ts.lineno * PDC_D2D_CHAR_HEIGHT;
        rectlist[i].right = ts.x * PDC_D2D_CHAR_WIDTH + ts.len * PDC_D2D_CHAR_WIDTH;
        rectlist[i].bottom = ts.lineno * PDC_D2D_CHAR_HEIGHT + PDC_D2D_CHAR_HEIGHT;

        i++;
    }

    hr = PDC_d2d_context->EndDraw();
    if (FAILED(hr)) {
        PDC_LOG(("Failure\n"));
    }

    /*
       TODO: instead of doing 1 rect per line, work out if we can consolodiate the rects.
       For example, if the left and right of the current rect match the previous rect we
       could just adjust the top and bottom of the previous rect and not generate a
       new rect.
    */
    DXGI_PRESENT_PARAMETERS params = { 0 };

    params.DirtyRectsCount = i;
    params.pDirtyRects = rectlist;

    // Submit the buffer for drawing.
    hr = PDC_d2d_swapChain->Present1(1, 0, &params);
    if (FAILED(hr)) {
        PDC_LOG(("Failure\n"));
    }

    delete[] rectlist;
    transforms.clear();
}

void PDC_blink_text(void)
{
    PDC_LOG((__FUNCTION__ " called\n"));
}
