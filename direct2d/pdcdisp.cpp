
#include <stdlib.h>
#include <string.h>

#include "pdcd2d.h"

#ifdef CHTYPE_LONG

# define A(x) ((chtype)x | A_ALTCHARSET)

chtype acs_map[128] =
{
    A(0), A(1), A(2), A(3), A(4), A(5), A(6), A(7), A(8), A(9),
    A(10), A(11), A(12), A(13), A(14), A(15), A(16), A(17), A(18),
    A(19), A(20), A(21), A(22), A(23), A(24), A(25), A(26), A(27),
    A(28), A(29), A(30), A(31), ' ', '!', '"', '#', '$', '%', '&',
    '\'', '(', ')', '*',

# ifdef PDC_WIDE
    0x2192, 0x2190, 0x2191, 0x2193,
# else
    A(0x1a), A(0x1b), A(0x18), A(0x19),
# endif

    '/',

# ifdef PDC_WIDE
    0x2588,
# else
    0xdb,
# endif

    '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=',
    '>', '?', '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
    'X', 'Y', 'Z', '[', '\\', ']', '^', '_',

# ifdef PDC_WIDE
    0x2666, 0x2592,
# else
    A(0x04), 0xb1,
# endif

    'b', 'c', 'd', 'e',

# ifdef PDC_WIDE
    0x00b0, 0x00b1, 0x2591, 0x00a4, 0x2518, 0x2510, 0x250c, 0x2514,
    0x253c, 0x23ba, 0x23bb, 0x2500, 0x23bc, 0x23bd, 0x251c, 0x2524,
    0x2534, 0x252c, 0x2502, 0x2264, 0x2265, 0x03c0, 0x2260, 0x00a3,
    0x00b7,
# else
    0xf8, 0xf1, 0xb0, A(0x0f), 0xd9, 0xbf, 0xda, 0xc0, 0xc5, 0x2d,
    0x2d, 0xc4, 0x2d, 0x5f, 0xc3, 0xb4, 0xc1, 0xc2, 0xb3, 0xf3,
    0xf2, 0xe3, 0xd8, 0x9c, 0xf9,
# endif

    A(127)
};

# undef A

#endif

int pdc_cheight = 16;
int pdc_cwidth = 8;
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

        PDC_pair_content(PAIR_NUMBER(ch), &newfg, &newbg);

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
            const auto col = pdc_d2d_colors[newfg];

            D2D_VECTOR_3F vec = {
                col.r, col.g, col.b
            };

            HRESULT c = pdc_colorEffect->SetValueByName(
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

    const int ybegin = pdc_cheight * lineno;
    const int xbegin = pdc_cwidth * x;
    const float raduis = 0.2f;

    const attr_t sysattrs = SP->termattrs;

    const bool do_blink = (attr & A_BLINK) && (sysattrs & A_BLINK);

    for (int i = 0; i < len; i++) {
        const float left = xbegin + i * pdc_cwidth;
        const float top = ybegin;
        const float right = left + pdc_cwidth;
        const float bottom = ybegin + pdc_cheight;

        int color = (srcp[i] & A_COLOR) >> PDC_COLOR_SHIFT;
        const chtype letter = srcp[i] & A_CHARTEXT;

        _set_colors(srcp[i]);

        const auto destination = D2D1::RectF(left, top, right, bottom);
        const auto dpoint = D2D1::Point2F(left, top);

        // Work out the area in the source glyph.

        const int source_row = letter / 32;
        const int source_col = letter % 32;

        const float sleft = source_col * 8;
        const float stop = source_row * 16;

        const auto source = D2D1::RectF(sleft, stop, sleft+8.0f, stop+16.0f);
        
        // https://docs.microsoft.com/en-us/windows/desktop/direct2d/color-matrix
        // https://stackoverflow.com/questions/6347950/programmatically-creating-directx-11-textures-pros-and-cons-of-the-three-differ/6539561#6539561
        // https://stackoverflow.com/questions/9021244/how-to-work-with-pixels-using-direct2d

        //m_d2dContext->SetTransform((ID2D1DrawTransform*)m_colorGlyphEffect);

        m_d2dContext->DrawImage(
            pdc_colorEffect,
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
    attr_t attr;

    int i = 1;
    int j = 1;

    old_attr = *srcp & (A_ATTRIBUTES ^ A_ALTCHARSET);

    static int cptr = 0;

    m_d2dContext->BeginDraw();

    while(j < len){
        attr = srcp[i] & (A_ATTRIBUTES ^ A_ALTCHARSET);
        
        if(attr != old_attr){
            _new_packet(old_attr, lineno, x, i, srcp);
            old_attr = attr;
            srcp += i;
            x += i;
            i = 0;
        }

        i++;
        j++;
    }
    
    _new_packet(old_attr, lineno, x, i, srcp);

    HRESULT hr = m_d2dContext->EndDraw();
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

    dirty.left = x * 8;
    dirty.top = lineno * 16;
    dirty.right = x * 8 + len * 8;
    dirty.bottom = lineno * 16 + 16;

    params.DirtyRectsCount = 1;
    params.pDirtyRects = &dirty;

    // Submit the buffer for drawing.
    hr = m_swapChain->Present1(1, 0, &params);
    if (FAILED(hr)) {
        PDC_LOG(("Failure\n"));
    }

    PDC_LOG((__FUNCTION__ " called\n"));
}

void PDC_blink_text(void)
{
    PDC_LOG((__FUNCTION__ " called\n"));
}
