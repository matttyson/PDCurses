#include "pdcd2d.h"

#include <Windows.h>

unsigned long pdc_key_modifiers = 0L;

bool PDC_check_key(void)
{
    PDC_EventQueue();

    if(PDC_d2d_should_resize > 0){
        return TRUE;
    }

    return PDC_d2d_event_count() > 0;
}

void PDC_flushinp(void)
{
    PDC_d2d_init_events();
    PDC_LOG((__FUNCTION__ " called\n"));
}

int PDC_get_key(void)
{
    const int key = PDC_d2d_get_event();    

    if(key == KEY_RESIZE) {
        SP->resized = TRUE;
        SP->key_code = TRUE;
    }

    PDC_EventQueue();
    PDC_LOG((__FUNCTION__ " called\n"));
    return key;
}

int PDC_modifiers_set(void)
{
    PDC_LOG((__FUNCTION__ " called\n"));
    return OK;
}

int PDC_mouse_set(void)
{
    PDC_LOG((__FUNCTION__ " called\n"));
    return OK;
}

void PDC_set_keyboard_binary(bool on)
{
    PDC_LOG((__FUNCTION__ " called\n"));
}
