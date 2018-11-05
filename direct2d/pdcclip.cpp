#include "pdcd2d.h"

int PDC_getclipboard(char **contents, long *length)
{
    PDC_LOG((__FUNCTION__  " called\n"));
    return 0;
}

int PDC_setclipboard(const char *contents, long length)
{
    PDC_LOG((__FUNCTION__ " called\n"));
    return 0;
}

int PDC_freeclipboard(char *contents)
{
    PDC_LOG((__FUNCTION__ " called\n"));
    return 0;
}

int PDC_clearclipboard(void)
{
    return 0;
}
