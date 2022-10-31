#include "extusb_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

int
__extusb_fs_unlink(struct _reent *r,
                   const char *name) {
    char *fixedPath;

    if (!name) {
        r->_errno = EINVAL;
        return -1;
    }

    fixedPath = __extusb_fs_fixpath(r, name);
    if (!fixedPath) {
        return -1;
    }

    FRESULT fr = f_unlink(fixedPath);
    free(fixedPath);
    if (fr != FR_OK) {
        r->_errno = __extusb_fs_translate_error(fr);
        return -1;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif