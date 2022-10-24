#include "extusb_devoptab.h"
#include "../devices.h"
#include "stdio.h"

#ifdef __cplusplus
extern "C" {
#endif

char *
__extusb_fs_fixpath(struct _reent *r,
                    const char *path) {
    char *p;
    char *fixedPath;

    if (!path) {
        r->_errno = EINVAL;
        return NULL;
    }

    p = strchr(path, ':') + 1;
    if (!strchr(path, ':')) {
        p = (char *) path;
    }

    if (strlen(p) > FS_MAX_PATH) {
        r->_errno = ENAMETOOLONG;
        return NULL;
    }

    fixedPath = (char *) memalign(0x40, FS_MAX_PATH + 1);
    if (!fixedPath) {
        r->_errno = ENOMEM;
        return NULL;
    }

    sprintf(fixedPath, "%d:%s", DEV_SD, p);
    return fixedPath;
}

int __extusb_fs_translate_error(FRESULT error) {
    switch ((int) error) {
        case FR_DISK_ERR:
        case FR_INT_ERR:
        case FR_NOT_READY:
            return EIO;
        case FR_NO_FILE:
        case FR_NO_PATH:
        case FR_INVALID_NAME:
            return ENOENT;
        case FR_DENIED:
            return EACCES;
        case FR_EXIST:
            return EEXIST;
        case FR_INVALID_OBJECT:
            return EBADF;
        case FR_WRITE_PROTECTED:
            return EROFS;
        case FR_INVALID_DRIVE:
            return ENODEV;
        case FR_NOT_ENABLED:
            return ENOMEM;
        case FR_NO_FILESYSTEM:
            return ENODEV;
        case FR_MKFS_ABORTED:
            return EINVAL;
        case FR_TIMEOUT:
            return EINTR;
        case FR_LOCKED:
            return EAGAIN;
        case FR_NOT_ENOUGH_CORE:
            return ENOMEM;
        case FR_TOO_MANY_OPEN_FILES:
            return EMFILE;
        case FR_INVALID_PARAMETER:
            return EINVAL;
    }
    return (int) error;
}

time_t __extusb_fs_translate_time(WORD fdate, WORD ftime) {
    struct tm time;
    time.tm_year = fdate >> 9;
    time.tm_mon = (fdate >> 5) & 0b1111;
    time.tm_mday = fdate & 0b11111;
    time.tm_hour = ftime >> 11;
    time.tm_min = (ftime >> 5) & 0b111111;
    time.tm_sec = (ftime & 0b11111) * 2;
    return mktime(&time);
}

mode_t __extusb_fs_translate_mode(FILINFO fileStat) {
    mode_t retMode = 0;

    if ((fileStat.fattrib & AM_DIR) == AM_DIR) {
        retMode |= S_IFDIR;
    } else {
        retMode |= S_IFREG;
    }

    mode_t flags = (FS_MODE_READ_OWNER | FS_MODE_READ_GROUP | FS_MODE_READ_OTHER) >> 2;
    if (fileStat.fattrib & AM_RDO) {
        flags |= (FS_MODE_WRITE_OWNER | FS_MODE_WRITE_GROUP | FS_MODE_WRITE_OTHER) >> 1;
    }
    flags |= FS_MODE_EXEC_OWNER | FS_MODE_EXEC_GROUP | FS_MODE_EXEC_OTHER;
    return retMode | flags;
}

#ifdef __cplusplus
}
#endif