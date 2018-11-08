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
extern "C" {
#endif
extern int pdc_cheight;
extern int pdc_cwidth;

/* Functions to buffer events while we wait for the user to collect them. */
void PDC_d2d_add_event(int event);
void PDC_d2d_init_events(void);
int PDC_d2d_get_event(void);
int PDC_d2d_event_count(void);

/* Win32 event loop callback. */
LRESULT CALLBACK PDC_d2d_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/**
 * Runs the Win32 event loop.  Collects key codes and other events
 * Needs to be called periodaclly for the app to function
*/
void PDC_EventQueue(void);

extern HWND hwnd;

#ifdef __cplusplus
}
#endif

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

extern ID2D1DeviceContext *m_d2dContext;
extern ID2D1Effect *pdc_colorEffect;
extern D2D1::ColorF pdc_d2d_colors[256];
extern ID2D1Bitmap *pdc_font_bitmap;

extern IDXGISwapChain1 *m_swapChain;

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

#endif // __cplusplus

#endif
