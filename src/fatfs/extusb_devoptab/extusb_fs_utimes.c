#include <time.h>
#include "extusb_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

int
__extusb_fs_utimes(struct _reent *r,
                   const char *filename,
                   const struct timeval times[2]) {
    if (!filename) {
        r->_errno = EINVAL;
        return -1;
    }

    FILINFO fno;
    if (times == NULL) {
        DWORD time = get_fattime();
        fno.fdate = (WORD) (time >> 16);
        fno.ftime = (WORD) (time & 0xFFFF);
    } else {
        struct tm time;
        localtime_r(&times[1].tv_sec, &time);
        fno.fdate = (WORD) (((time.tm_year - 1980) * 512U) | time.tm_mon * 32U | time.tm_mday);
        fno.ftime = (WORD) (time.tm_hour * 2048U | time.tm_min * 32U | time.tm_sec / 2U);
    }

    FRESULT fr = f_utime(filename, &fno);
    if (fr != FR_OK) {
        r->_errno = __extusb_fs_translate_error(fr);
        return -1;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif