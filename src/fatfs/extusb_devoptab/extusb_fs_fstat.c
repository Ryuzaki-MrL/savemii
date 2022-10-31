#include "extusb_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

int
__extusb_fs_fstat(struct _reent *r,
                  void *fd,
                  struct stat *st) {
    if (!fd || !st) {
        r->_errno = EINVAL;
        return -1;
    }

    __extusb_fs_file_t *file = (__extusb_fs_file_t *) fd;
    return __extusb_fs_stat(r, file->path, st);
}

#ifdef __cplusplus
}
#endif