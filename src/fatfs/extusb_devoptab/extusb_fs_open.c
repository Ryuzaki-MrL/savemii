#include "extusb_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

int __extusb_fs_open(struct _reent *r, void *fileStruct, const char *path, int flags, int mode) {
    if (!fileStruct || !path) {
        r->_errno = EINVAL;
        return -1;
    }

    BYTE fsMode;
    if (flags == 0) {
        fsMode = FA_READ;
    } else if (flags == 2) {
        fsMode = FA_READ | FA_WRITE;
    } else if (flags == 0x601) {
        fsMode = FA_CREATE_ALWAYS | FA_WRITE;
    } else if (flags == 0x602) {
        fsMode = FA_CREATE_ALWAYS | FA_WRITE | FA_READ;
    } else if (flags == 0x209) {
        fsMode = FA_OPEN_APPEND | FA_WRITE;
    } else if (flags == 0x20A) {
        fsMode = FA_OPEN_APPEND | FA_WRITE | FA_READ;
    } else {
        r->_errno = EINVAL;
        return -1;
    }

    char *fixedPath = __extusb_fs_fixpath(r, path);
    if (!fixedPath) {
        return -1;
    }

    FIL fp;
    FRESULT fr;

    // Open the file
    fr = f_open(&fp, fixedPath, fsMode);
    if (fr != FR_OK) {
        r->_errno = __extusb_fs_translate_error(fr);
        free(fixedPath);
        return -1;
    }

    __extusb_fs_file_t *file = (__extusb_fs_file_t *) fileStruct;
    file->fp = fp;
    file->flags = (flags & (O_ACCMODE | O_APPEND | O_SYNC));
    file->offset = 0;
    file->path = fixedPath;
    return 0;
}

#ifdef __cplusplus
}
#endif