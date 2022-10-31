#include "extusb_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

int
__extusb_fs_stat(struct _reent *r,
                 const char *path,
                 struct stat *st) {
    if (!path || !st) {
        r->_errno = EINVAL;
        return -1;
    }

    char *fixedPath = __extusb_fs_fixpath(r, path);
    if (!fixedPath) {
        return -1;
    }

    FILINFO fno;
    FRESULT fr = f_stat(fixedPath, &fno);
    if (fr != FR_OK) {
        free(fixedPath);
        r->_errno = __extusb_fs_translate_error(fr);
        return -1;
    }
    free(fixedPath);

    memset(st, 0, sizeof(struct stat));

    st->st_nlink = 1;

    st->st_atime = __extusb_fs_translate_time(fno.fdate, fno.ftime);
    st->st_ctime = __extusb_fs_translate_time(fno.fdate, fno.ftime);
    st->st_mtime = __extusb_fs_translate_time(fno.fdate, fno.ftime);
    st->st_mode = __extusb_fs_translate_mode(fno);

    if (!(fno.fattrib & AM_DIR)) {
        st->st_size = (off_t) fno.fsize;
        st->st_uid = 0;
        st->st_gid = 0;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif