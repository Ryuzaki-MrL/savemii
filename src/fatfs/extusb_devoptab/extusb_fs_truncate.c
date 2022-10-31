#include "extusb_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

int
__extusb_fs_ftruncate(struct _reent *r,
                      void *fd,
                      off_t len) {
    // Make sure length is non-negative
    if (!fd || len < 0) {
        r->_errno = EINVAL;
        return -1;
    }

    // Set the new file size
    __extusb_fs_file_t *file = (__extusb_fs_file_t *) fd;
    FRESULT fr = f_lseek(&file->fp, len);
    if (fr != FR_OK) {
        r->_errno = __extusb_fs_translate_error(fr);
        return -1;
    }

    fr = f_truncate(&file->fp);
    if (fr != FR_OK) {
        r->_errno = __extusb_fs_translate_error(fr);
        return -1;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif