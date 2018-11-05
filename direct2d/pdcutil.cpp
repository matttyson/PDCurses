#include "pdcd2d.h"

void PDC_beep(void)
{
    PDC_LOG((__FUNCTION__ " called\n"));
}

void PDC_napms(int ms)
{
    ::Sleep(ms);
    PDC_LOG((__FUNCTION__ " called\n"));
}

const char *PDC_sysname(void)
{
    return "Direct2D";
}

