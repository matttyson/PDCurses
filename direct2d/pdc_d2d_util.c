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
    while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) {
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

        //return -1;
    }

    SP->key_code = TRUE; // ?

    // 0 - 9
    if(vk >= 0x30 && vk <= 0x39){
        return vk;
    }
    // A - Z
    if(vk >= 0x41 && vk <= 0x5A){
        return vk;
    }

    switch(vk){
    case VK_BACK: return KEY_BACKSPACE;
//    case VK_TAB: return KEY_TAB
    case VK_LEFT: return KEY_LEFT;
    case VK_RIGHT: return KEY_RIGHT;
    case VK_UP: return KEY_UP;
    case VK_DOWN: return KEY_DOWN;
    case VK_INSERT: return KEY_IC;
    case VK_ESCAPE: return 0x1B;
    case VK_RETURN: return '\n';
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
