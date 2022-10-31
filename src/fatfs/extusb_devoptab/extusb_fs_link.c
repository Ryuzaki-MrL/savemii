#include "extusb_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

int
__extusb_fs_link(struct _reent *r,
                 const char *existing,
                 const char *newLink) {
    r->_errno = ENOSYS;
    return -1;
}

#ifdef __cplusplus
}
#endif