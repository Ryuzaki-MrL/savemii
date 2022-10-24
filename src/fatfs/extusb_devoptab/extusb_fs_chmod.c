#include <sys/stat.h>
#include "extusb_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

int
__extusb_fs_chmod(struct _reent *r,
                  const char *path,
                  mode_t mode) {
    return 0;
}

#ifdef __cplusplus
}
#endif