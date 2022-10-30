#include "extusb_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

int __extusb_fs_close(struct _reent *r,
                      void *fd) {
    if (!fd) {
        r->_errno = EINVAL;
        return -1;
    }

    __extusb_fs_file_t *file = (__extusb_fs_file_t *) fd;
    FRESULT fr = f_close(&file->fp);
    if (fr != FR_OK) {
        r->_errno = __extusb_fs_translate_error(fr);
        return -1;
    }

    free(file->path);

    return 0;
}

#ifdef __cplusplus
}
#endif