#include "extusb_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

int
__extusb_fs_rename(struct _reent *r,
                   const char *oldName,
                   const char *newName) {
    char *fixedOldPath, *fixedNewPath;

    if (!oldName || !newName) {
        r->_errno = EINVAL;
        return -1;
    }

    fixedOldPath = __extusb_fs_fixpath(r, oldName);
    if (!fixedOldPath) {
        return -1;
    }

    fixedNewPath = __extusb_fs_fixpath(r, newName);
    if (!fixedNewPath) {
        free(fixedOldPath);
        return -1;
    }

    FRESULT fr = f_rename(fixedOldPath, fixedNewPath);
    free(fixedOldPath);
    free(fixedNewPath);

    if (fr != FR_OK) {
        r->_errno = __extusb_fs_translate_error(fr);
        return -1;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif