#include "extusb_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

int
__extusb_fs_dirclose(struct _reent *r,
                     DIR_ITER *dirState) {

    if (!dirState) {
        r->_errno = EINVAL;
        return -1;
    }

    __extusb_fs_dir_t *dir = (__extusb_fs_dir_t *) (dirState->dirStruct);
    FRESULT fr = f_closedir(&dir->dp);
    if (fr != FR_OK) {
        r->_errno = __extusb_fs_translate_error(fr);
        return -1;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif