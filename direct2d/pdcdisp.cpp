
#include <stdlib.h>
#include <string.h>

#include "pdcd2d.h"

#ifdef PDC_WIDE
# include "../common/acsgr.h"
#else
# include "../common/acs437.h"
#endif

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


static void _new_packet(int lineno, int x, int len, const chtype *srcp)
{
    const int ybegin = PDC_D2D_CHAR_HEIGHT * lineno;
    const int xbegin = PDC_D2D_CHAR_WIDTH * x;

    const attr_t sysattrs = SP->termattrs;

    for (int i = 0; i < len; i++) {
        const float left = xbegin + i * PDC_D2D_CHAR_WIDTH;
        const float top = ybegin;
        const float right = left + PDC_D2D_CHAR_WIDTH;
        const float bottom = ybegin + PDC_D2D_CHAR_HEIGHT;
        const auto destination = D2D1::RectF(left, top, right, bottom);

        chtype ch = srcp[i];

        _set_colors(ch);

        if (ch & A_ALTCHARSET && !(ch & 0xff80)) {
            ch = acs_map[ch & 0x7f];
        }

        const auto dpoint = D2D1::Point2F(left, top);

        // Work out the area in the source glyph.
        const int source_row = (ch & 0xff) / 32;
        const int source_col = (ch & 0xff) % 32;

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
    }
}

#include <array>

struct line_transform
{
    line_transform() :lineno(0), x(0), len(0), srcp(nullptr) {}
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
        :lineno(rhs.lineno), x(rhs.x), len(rhs.len)
    {
        delete[] srcp;
        srcp = rhs.srcp;
        rhs.srcp = nullptr;
    }
    line_transform& operator=(line_transform&& rhs) noexcept {
        lineno = rhs.lineno;
        x = rhs.x;
        len = rhs.len;
        if (srcp != nullptr) {
            delete[] srcp;
        }
        srcp = rhs.srcp;

        rhs.srcp = nullptr;

        return *this;
    }

    int lineno;
    int x;
    int len;
    chtype* srcp;
};

constexpr size_t max_transforms = 25;
static size_t current_transforms = 0;
static std::array<line_transform, max_transforms> transforms;
static std::array<RECT, max_transforms> ts_rects;

void PDC_transform_line(int lineno, int x, int len, const chtype *srcp)
{
    if (current_transforms == max_transforms) {
        PDC_doupdate();
    }
    transforms[current_transforms++] = line_transform(lineno, x, len, srcp);
}

static void D2D_draw_transform(int lineno, int x, int len, const chtype* srcp)
{
    _new_packet(lineno, x, len, srcp);

    // https://code.msdn.microsoft.com/windowsapps/DXGI-swap-chain-rotation-21d13d71
    // https://docs.microsoft.com/en-gb/windows/desktop/api/dxgi1_2/nf-dxgi1_2-idxgiswapchain1-present1
    // https://docs.microsoft.com/en-gb/windows/desktop/api/dxgi1_2/ns-dxgi1_2-dxgi_present_parameters
    // https://docs.microsoft.com/en-us/windows/desktop/direct3ddxgi/dxgi-1-2-presentation-improvements

    PDC_LOG((__FUNCTION__ " called\n"));
}

void PDC_doupdate(void)
{
    HRESULT hr;
    int prev_lineno = INT_MIN;
    int prev_x = INT_MIN;
    int prev_len = INT_MIN;
    size_t r = 0;

    if (current_transforms == 0) {
        return;
    }

    PDC_d2d_context->BeginDraw();

    for(size_t i = 0; i < current_transforms; i++) {
        D2D_draw_transform(
            transforms[i].lineno,
            transforms[i].x,
            transforms[i].len,
            transforms[i].srcp);

        if ((prev_lineno + 1 == transforms[i].lineno) &&
            (prev_x == transforms[i].x) &&
            (prev_len == transforms[i].len))
        {
            /*
             * This line is the same size as the line above it. Instead of creating a new rect
             * just adjust the bottom of the previous rect
             */
            ts_rects[r-1].bottom = transforms[i].lineno * PDC_D2D_CHAR_HEIGHT + PDC_D2D_CHAR_HEIGHT;

            prev_lineno++;
        }
        else {
        /* Calculate the dirty rects */
            ts_rects[r].left = transforms[i].x * PDC_D2D_CHAR_WIDTH;
            ts_rects[r].top = transforms[i].lineno * PDC_D2D_CHAR_HEIGHT;
            ts_rects[r].right = transforms[i].x * PDC_D2D_CHAR_WIDTH + transforms[i].len * PDC_D2D_CHAR_WIDTH;
            ts_rects[r].bottom = transforms[i].lineno * PDC_D2D_CHAR_HEIGHT + PDC_D2D_CHAR_HEIGHT;
            r++;
            prev_lineno = transforms[i].lineno;
            prev_x = transforms[i].x;
            prev_len = transforms[i].len;
        }
    }

    hr = PDC_d2d_context->EndDraw();
    if (FAILED(hr)) {
        PDC_LOG(("Failure\n"));
        /* What do we do here?*/
        current_transforms = 0;
        return;
    }

    DXGI_PRESENT_PARAMETERS params = { 0 };

    params.DirtyRectsCount = static_cast<UINT>(r);
    params.pDirtyRects = ts_rects.data();

    // Submit the buffer for drawing.
    hr = PDC_d2d_swapChain->Present1(1, 0, &params);
    if (FAILED(hr)) {
        PDC_LOG(("Failure\n"));
    }

    current_transforms = 0;
}

void PDC_blink_text(void)
{
    PDC_LOG((__FUNCTION__ " called\n"));
}
