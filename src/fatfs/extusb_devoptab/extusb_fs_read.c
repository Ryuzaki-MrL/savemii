#include "extusb_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

ssize_t __extusb_fs_read(struct _reent *r, void *fd, char *ptr, size_t len) {
    if (!fd || !ptr) {
        r->_errno = EINVAL;
        return -1;
    }

    // Check that the file was opened with read access
    __extusb_fs_file_t *file = (__extusb_fs_file_t *) fd;
    if ((file->flags & O_ACCMODE) == O_WRONLY) {
        r->_errno = EBADF;
        return -1;
    }

    // cache-aligned, cache-line-sized
    __attribute__((aligned(0x40))) uint8_t alignedBuffer[0x40];

    size_t bytesRead = 0;
    while (bytesRead < len) {
        // only use input buffer if cache-aligned and read size is a multiple of cache line size
        // otherwise read into alignedBuffer
        uint8_t *tmp = (uint8_t *) ptr;
        size_t size = len - bytesRead;

        if ((uintptr_t) ptr & 0x3F) {
            // read partial cache-line front-end
            tmp = alignedBuffer;
            size = MIN(size, 0x40 - ((uintptr_t) ptr & 0x3F));
        } else if (size < 0x40) {
            // read partial cache-line back-end
            tmp = alignedBuffer;
        } else {
            // read whole cache lines
            size &= ~0x3F;
        }

        UINT br;
        FRESULT fr = f_read(&file->fp, tmp, size, &br);

        if (fr != FR_OK) {
            if (bytesRead != 0) {
                return (ssize_t) bytesRead; // error after partial read
            }

            r->_errno = __extusb_fs_translate_error(fr);
            return -1;
        }

        if (tmp == alignedBuffer) {
            memcpy(ptr, alignedBuffer, br);
        }

        bytesRead += br;
        ptr += br;

        if (br != size) {
            return (ssize_t) bytesRead; // partial read
        }
    }

    return (ssize_t) bytesRead;
}

#ifdef __cplusplus
}
#endif