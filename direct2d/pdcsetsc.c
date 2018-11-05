#include "pdcd2d.h"

/*
    I *think* that the way PDCurses does its #define bool stuff causes C++ to think
    that we are creating an overloaded function that accepts an unsigned char
    which causes some strange linker errors.

    To work around this we'll compile these functions as c files, so we don't
    have to mess with conflicting linker definitions.
*/

int PDC_set_bold(bool boldon)
{
    PDC_LOG((__FUNCTION__ " called\n"));
    return 0;
}

int PDC_set_blink(bool blinkon)
{
    PDC_LOG((__FUNCTION__ " called\n"));
    return 0;
}
