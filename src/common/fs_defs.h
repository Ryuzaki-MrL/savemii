#ifndef FS_DEFS_H
#define	FS_DEFS_H

#include <gctypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FS_MAX_LOCALPATH_SIZE           511
#define FS_MAX_MOUNTPATH_SIZE           128
#define FS_MAX_FULLPATH_SIZE            (FS_MAX_LOCALPATH_SIZE + FS_MAX_MOUNTPATH_SIZE)

#define BUS_SPEED                       248625000

#define FSA_STATUS_END_OF_DIRECTORY     (-4)
#define FSA_STATUS_NOT_FOUND            (-23)

#ifdef __cplusplus
}
#endif

#endif	/* FS_DEFS_H */

