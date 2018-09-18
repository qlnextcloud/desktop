//
// Created by isshe on 2018/9/4.
//

#ifndef CSYNC_PRIORITY_H
#define CSYNC_PRIORITY_H

#include "ocsynclib.h"
#include "csync.h"
#include "std/c_string.h"

enum csync_priority_type_e {
    CSYNC_NOT_PRIORITY   = 0,
    CSYNC_FILE_PRIORITY
};

typedef enum csync_priority_type_e CSYNC_PRIORITY_TYPE;

//void csync_priority_traversal_prepare(CSYNC *ctx);

//CSYNC_PRIORITY_TYPE csync_priority_traversal(CSYNC *ctx, const char *path, int filetype);

int OCSYNC_EXPORT csync_priority_load(const char *fname, c_strlist_t **list);

CSYNC_PRIORITY_TYPE OCSYNC_EXPORT csync_priority_no_ctx(c_strlist_t *excludes, const char *path, int filetype);
#endif //CSYNC_PRIORITY_H
