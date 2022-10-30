#include "extusb_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

DIR_ITER *
__extusb_fs_diropen(struct _reent *r,
                    DIR_ITER *dirState,
                    const char *path) {
    if (!dirState || !path) {
        r->_errno = EINVAL;
        return NULL;
    }

    char *fixedPath = __extusb_fs_fixpath(r, path);
    if (!fixedPath) {
        return NULL;
    }

    __extusb_fs_dir_t *dir = (__extusb_fs_dir_t *) (dirState->dirStruct);
    FRESULT fr = f_opendir(&dir->dp, fixedPath);
    free(fixedPath);
    if (fr != FR_OK) {
        r->_errno = __extusb_fs_translate_error(fr);
        return NULL;
    }
    memset(&dir->entry_data, 0, sizeof(dir->entry_data));
    return dirState;
}

#ifdef __cplusplus
}
#endif