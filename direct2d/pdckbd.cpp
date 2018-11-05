#include "pdcd2d.h"

extern "C"
unsigned long pdc_key_modifiers = 0L;

bool PDC_check_key(void)
{
    return FALSE;
}

void PDC_flushinp(void)
{
    PDC_LOG((__FUNCTION__ " called\n"));
}

int PDC_get_key(void)
{
    PDC_LOG((__FUNCTION__ " called\n"));
    return 'A';
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

unsigned long PDC_get_input_fd(void)
{
    PDC_LOG((__FUNCTION__ " called\n"));

    return 0L;  /* test this */
}
