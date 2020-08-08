#include "pdcd2d.h"

void PDC_beep(void)
{
    PDC_LOG((__FUNCTION__ " called\n"));
}

// The SDL ports seem to do their event loop processing in here.
// Might need to make sure that event processing doesn't take longer
// than desired naptime.
void PDC_napms(int ms)
{
    PDC_EventQueue();
    Sleep(ms);
    PDC_LOG((__FUNCTION__ " called\n"));
}

const char *PDC_sysname(void)
{
    return "Direct2D";
}

