#include "extusb_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

off_t
__extusb_fs_seek(struct _reent *r,
                 void *fd,
                 off_t pos,
                 int whence) {
    uint64_t offset;;

    if (!fd) {
        r->_errno = EINVAL;
        return -1;
    }

    __extusb_fs_file_t *file = (__extusb_fs_file_t *) fd;
    FILINFO fno;
    FRESULT fr;

    // Find the offset to see from
    switch (whence) {
        // Set absolute position; start offset is 0
        case SEEK_SET:
            offset = 0;
            break;

            // Set position relative to the current position
        case SEEK_CUR:
            offset = file->offset;
            break;

            // Set position relative to the end of the file
        case SEEK_END:
            fr = f_stat(file->path, &fno);
            if (fr != FR_OK) {
                r->_errno = __extusb_fs_translate_error(fr);
                return -1;
            }
            offset = fno.fsize;
            break;

            // An invalid option was provided
        default:
            r->_errno = EINVAL;
            return -1;
    }

    // TODO: A better check that prevents overflow.
    if (pos < 0 && offset < -pos) {
        // Don't allow seek to before the beginning of the file
        r->_errno = EINVAL;
        return -1;
    }

    uint32_t old_offset = file->offset;
    file->offset = offset + pos;

    fr = f_lseek(&file->fp, file->offset);
    if (fr != FR_OK) {
        // revert offset update on error.
        file->offset = old_offset;
        r->_errno = __extusb_fs_translate_error(fr);
        return -1;
    }

    return file->offset;
}

#ifdef __cplusplus
}
#endif