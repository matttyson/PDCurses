#include "pdcd2d.h"

int PDC_get_cursor_mode(void)
{
    PDC_LOG(("PDC_get_cursor_mode() - called\n"));

    return 0;
}

/* return number of screen rows */

int PDC_get_rows(void)
{
    PDC_LOG(("PDC_get_rows() - called\n"));

    return pdc_d2d_rows;
}

/* return width of screen/viewport */
int PDC_get_columns(void)
{
    PDC_LOG(("PDC_get_columns() - called\n"));

    return pdc_d2d_cols;
}
