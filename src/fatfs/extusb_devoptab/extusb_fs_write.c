#include "extusb_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

ssize_t __extusb_fs_write(struct _reent *r, void *fd, const char *ptr,
                          size_t len) {
    if (!fd || !ptr) {
        r->_errno = EINVAL;
        return -1;
    }

    __extusb_fs_file_t *file = (__extusb_fs_file_t *) fd;
    if ((file->flags & O_ACCMODE) == O_RDONLY) {
        r->_errno = EBADF;
        return -1;
    }

    // cache-aligned, cache-line-sized
    __attribute__((aligned(0x40))) uint8_t alignedBuffer[0x40];

    size_t bytesWritten = 0;
    while (bytesWritten < len) {
        // only use input buffer if cache-aligned and write size is a multiple of cache line size
        // otherwise write from alignedBuffer
        uint8_t *tmp = (uint8_t *) ptr;
        size_t size = len - bytesWritten;

        if ((uintptr_t) ptr & 0x3F) {
            // write partial cache-line front-end
            tmp = alignedBuffer;
            size = MIN(size, 0x40 - ((uintptr_t) ptr & 0x3F));
        } else if (size < 0x40) {
            // write partial cache-line back-end
            tmp = alignedBuffer;
        } else {
            // write whole cache lines
            size &= ~0x3F;
        }

        if (tmp == alignedBuffer) {
            memcpy(tmp, ptr, size);
        }

        UINT bw;
        FRESULT fr = f_write(&file->fp, tmp, size, &bw);
        if (fr != FR_OK) {
            if (bytesWritten != 0) {
                return (ssize_t) bytesWritten; // error after partial write
            }

            r->_errno = __extusb_fs_translate_error(fr);
            return -1;
        }

        bytesWritten += bw;
        ptr += bw;

        if (bw != size) {
            return (ssize_t) bytesWritten; // partial write
        }
    }

    return (ssize_t) bytesWritten;
}

#ifdef __cplusplus
}
#endif