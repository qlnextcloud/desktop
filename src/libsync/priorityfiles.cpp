//
// Created by isshe on 2018/9/4.
//

#include "priorityfiles.h"
#include "config.h"
#include "common/utility.h"

#include "std/c_string.h"
#include "csync.h"
#include "csync_priority.h"

#include <QFileInfo>

using namespace OCC;

PriorityFiles::PriorityFiles(c_strlist_t **prioritysPtr)
        : _prioritysPtr(prioritysPtr)
{
}

PriorityFiles::~PriorityFiles()
{
    c_strlist_destroy(*_prioritysPtr);
}

PriorityFiles &PriorityFiles::instance()
{
    static c_strlist_t *globalPrioritys;
    static PriorityFiles inst(&globalPrioritys);
    return inst;
}

void PriorityFiles::addPriorityFilePath(const QString &path)
{
    _priorityFiles.insert(path);
}

bool PriorityFiles::reloadPrioritys(){
    c_strlist_destroy(*_prioritysPtr);
    *_prioritysPtr = nullptr;

    bool success = true;
    foreach (const QString &file, _priorityFiles) {
        if (csync_priority_load(file.toUtf8(), _prioritysPtr) < 0)
        {
            success = false;
        }
    }

    // The csync_exclude_traversal_prepare is called implicitely at sync start.
    return success;
}

bool PriorityFiles::isPriority(
        const QString &filePath,
        const QString &basePath) const
{
    if (!filePath.startsWith(basePath, Utility::fsCasePreserving() ? Qt::CaseInsensitive : Qt::CaseSensitive)) {
        // Mark paths we're not responsible for as excluded...
        return true;
    }

    QFileInfo fi(filePath);
    csync_ftw_type_e type = CSYNC_FTW_TYPE_FILE;
    if (fi.isDir()) {
        type = CSYNC_FTW_TYPE_DIR;
    }

    QString relativePath = filePath.mid(basePath.size());
    if (relativePath.endsWith(QLatin1Char('/'))) {
        relativePath.chop(1);
    }

    return csync_priority_no_ctx(*_prioritysPtr, relativePath.toUtf8(), type) != CSYNC_NOT_PRIORITY;
}
