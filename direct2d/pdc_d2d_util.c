#include "pdcd2d.h"

#define BUFFER_SIZE 16

struct ring_buffer {
    int head;
    int tail;
    int size;
    int buffer[BUFFER_SIZE];
};

static struct ring_buffer buffer;

void PDC_EventQueue(void)
{
    MSG msg = { 0 };
    while (PeekMessage(&msg, PDC_d2d_hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static int PDC_d2d_event_full(void)
{
    return ((buffer.head + 1) & (BUFFER_SIZE - 1)) == buffer.tail;
}

void PDC_d2d_init_events(void)
{
    buffer.head = 0;
    buffer.tail = 0;
    buffer.size = 0;
}

/* Adds an event to the queue of keyboard events*/
void PDC_d2d_add_event(int event)
{
    if(PDC_d2d_event_full()){
        return;
    }
    buffer.buffer[buffer.head] = event;
    buffer.head = (buffer.head + 1) & (BUFFER_SIZE - 1);
    buffer.size++;
}

int PDC_d2d_get_event()
{
    int evt = -1;

    if(PDC_d2d_should_resize > 0){
        PDC_d2d_should_resize = 0;
        return KEY_RESIZE;
    }

    if(buffer.size > 0){
        evt = buffer.buffer[buffer.tail];
        buffer.tail = (buffer.tail + 1) & (BUFFER_SIZE - 1);
        buffer.size--;
    }

    return evt;
}

int PDC_d2d_event_count(void)
{
    return buffer.size;
}


static chtype _process_key_event_2(WPARAM vk, LPARAM state)
{
    pdc_key_modifiers = 0L;

    if (SP->save_key_modifiers){
        if(GetKeyState(VK_SHIFT) & 0x8000){
            pdc_key_modifiers |= PDC_KEY_MODIFIER_SHIFT;
        }
        if(GetKeyState(VK_CONTROL) & 0x8000){
            pdc_key_modifiers |= PDC_KEY_MODIFIER_CONTROL;
        }
        if(state & (1 << 29)){
            pdc_key_modifiers |= PDC_KEY_MODIFIER_ALT;
        }
    }
    else if(SP->return_key_modifiers){
        SP->key_code = TRUE;
        if(GetKeyState(VK_RSHIFT) & 0x8000){
            return KEY_SHIFT_R;
        }
        else if(GetKeyState(VK_LSHIFT)){
            return KEY_SHIFT_L;
        }
        else if(GetKeyState(VK_LCONTROL) & 0x8000){
            return KEY_CONTROL_R;
        }
        else if(GetKeyState(VK_RCONTROL) & 0x8000){
            return KEY_CONTROL_L;
        }
        else if(GetKeyState(VK_RMENU) & 0x8000){
            return KEY_ALT_R;
        }
        else if(GetKeyState(VK_LMENU) & 0x8000){
            return KEY_ALT_L;
        }
        SP->key_code = FALSE;
    }

    // 0 - 9
    if(vk >= 0x30 && vk <= 0x39){
        if (state & (1 << 29)) { // ALT
            switch(vk){
            case '0': return ALT_0;
            case '1': return ALT_1;
            case '2': return ALT_2;
            case '3': return ALT_3;
            case '4': return ALT_4;
            case '5': return ALT_5;
            case '6': return ALT_6;
            case '7': return ALT_7;
            case '8': return ALT_8;
            case '9': return ALT_9;
            }
        }
        return vk;
    }
    // A - Z
    if(vk >= 0x41 && vk <= 0x5A){
        if(state & (1 << 29)){ // ALT
            switch(vk){
            case 'A': return ALT_A;
            case 'B': return ALT_B;
            case 'C': return ALT_C;
            case 'D': return ALT_D;
            case 'E': return ALT_E;
            case 'F': return ALT_F;
            case 'G': return ALT_G;
            case 'H': return ALT_H;
            case 'I': return ALT_I;
            case 'J': return ALT_J;
            case 'K': return ALT_K;
            case 'L': return ALT_L;
            case 'M': return ALT_M;
            case 'N': return ALT_N;
            case 'O': return ALT_O;
            case 'P': return ALT_P;
            case 'Q': return ALT_Q;
            case 'R': return ALT_R;
            case 'S': return ALT_S;
            case 'T': return ALT_T;
            case 'U': return ALT_U;
            case 'V': return ALT_V;
            case 'W': return ALT_W;
            case 'X': return ALT_X;
            case 'Y': return ALT_Y;
            case 'Z': return ALT_Z;
            }
        }

        if (GetKeyState(VK_CAPITAL) & 0x0001) {
            return vk;
        }
        if(GetKeyState(VK_SHIFT) & 0x8000){
            return vk;
        }

        // lowercase
        return vk + 0x20;
    }

    // F1 - F24 function keys
    if(vk >= 0x70 && vk <= 0x87){
        return KEY_F((vk - 0x70 +1));
    }

    switch(vk){
    case VK_LEFT: return KEY_LEFT;
    case VK_RIGHT: return KEY_RIGHT;
    case VK_UP: return KEY_UP;
    case VK_DOWN: return KEY_DOWN;
    case VK_INSERT: return KEY_IC;
    case VK_DELETE: return KEY_DC;
    case VK_HOME: return KEY_HOME;
    case VK_END: return KEY_END;
    case VK_PRIOR: return KEY_PPAGE;
    case VK_NEXT: return KEY_NPAGE;
    case VK_ESCAPE: return 0x1B;
    case VK_RETURN: return '\n';
    case VK_TAB: return '\t';
    case VK_BACK: return 0x08;
    case VK_SPACE: return ' ';
    }


    switch(vk){
    case VK_LBUTTON:
    case VK_RBUTTON:
    case VK_MBUTTON:
    case VK_XBUTTON1:
    case VK_XBUTTON2:
        PDC_LOG(("Mouse Event\n"));
        break;
    }

    return -1;
}


LRESULT CALLBACK PDC_d2d_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_SIZING:
    {
        if(wParam == WMSZ_RIGHT || wParam == WMSZ_BOTTOM || wParam == WMSZ_BOTTOMRIGHT){
            RECT windowRect;
            GetWindowRect(hwnd, &windowRect);
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);

            const int borderWidth = abs((windowRect.right - windowRect.left) - (clientRect.right - clientRect.left));
            const int topWidth = abs((windowRect.top - windowRect.bottom) - (clientRect.top - clientRect.bottom));

            // We want to be able to force the window size to a multiple of 8 pixels wide
            // or 16 pixels high so we can fit a complete row / column in.
            // What we do is round down the window sizes so that rect is always a multiple
            // of 8 x 16.
            RECT *newsz = (RECT*) lParam;

            // Work out the size of the client area by removing the border sizes
            const int width = (newsz->right - newsz->left) - borderWidth;
            const int height = (newsz->bottom - newsz->top) - topWidth;

            // Update with our new row and column values
            PDC_d2d_cols = width / 8;
            PDC_d2d_rows = height / 16;

            newsz->right = newsz->left + PDC_d2d_cols * 8 + borderWidth;
            newsz->bottom = newsz->top + PDC_d2d_rows * 16 + topWidth;
            printf("%03d x %03d\n", PDC_d2d_cols, PDC_d2d_rows);
        }
        else
        {
            RECT windowRect;
            GetWindowRect(hwnd, &windowRect);
            *((RECT*)lParam) = windowRect;
        }
    }
        break;
    case WM_SIZE:
        // The window has been resized by the user, notify the client that it should resize itself.
        PDC_d2d_should_resize = 1;
        break;

    case WM_CREATE:
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        const int code = _process_key_event_2(wParam, lParam);
        PDC_d2d_add_event(code);
        return 0;
    }
        break;

    case WM_UNICHAR:
    case WM_CHAR:
        break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
