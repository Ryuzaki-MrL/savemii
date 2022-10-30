#include "extusb_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

int
__extusb_fs_dirnext(struct _reent *r,
                    DIR_ITER *dirState,
                    char *filename,
                    struct stat *filestat) {
    if (!dirState || !filename || !filestat) {
        r->_errno = EINVAL;
        return -1;
    }

    __extusb_fs_dir_t *dir = (__extusb_fs_dir_t *) (dirState->dirStruct);
    memset(&dir->entry_data, 0, sizeof(dir->entry_data));
    FRESULT fr = f_readdir(&dir->dp, &dir->entry_data);
    if (fr != FR_OK) {
        r->_errno = __extusb_fs_translate_error(fr);
        return -1;
    }
    if (dir->entry_data.fname[0] == 0) {
        r->_errno = ENOENT;
        return -1;
    }

    // Fill in the stat info
    memset(filestat, 0, sizeof(struct stat));
    filestat->st_ino = 0;

    if (dir->entry_data.fattrib & AM_DIR) {
        filestat->st_mode = S_IFDIR;
    } else {
        filestat->st_mode = S_IFREG;
    }

    filestat->st_uid = 0;
    filestat->st_gid = 0;
    filestat->st_size = dir->entry_data.fsize;

    memset(filename, 0, NAME_MAX);
    strcpy(filename, dir->entry_data.fname);
    return 0;
}

#ifdef __cplusplus
}
#endif