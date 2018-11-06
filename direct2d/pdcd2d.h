#ifndef PDCD2D_H
#define PDCD2D_H


#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <math.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#undef NOMINMAX
#undef MOUSE_MOVED

#include <curses.h>
#include <curspriv.h>

#ifdef __cplusplus

#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1_1helper.h>
#include <d2d1helper.h>
#include <d2d1effects.h>
#include <dxgi1_2.h>
#include <wincodec.h>
#include <dwrite.h>

#include "ColorGlyphEffect.hpp"

extern HWND hwnd;
extern ID2D1DeviceContext *m_d2dContext;
extern ID2D1Effect *pdc_colorEffect;
extern D2D1::ColorF pdc_d2d_colors[256];
extern ID2D1Bitmap *pdc_font_bitmap;

extern IDXGISwapChain1 *m_swapChain;

extern int pdc_cheight;
extern int pdc_cwidth;

template<class Interface>
inline void SafeRelease(
    Interface **ppInterfaceToRelease
)
{
    if (*ppInterfaceToRelease != NULL)
    {
        (*ppInterfaceToRelease)->Release();

        (*ppInterfaceToRelease) = NULL;
    }
}

#endif

#endif
