//
// Created by isshe on 2018/9/4.
//

#ifndef PRIORITYFILES_H
#define PRIORITYFILES_H

#pragma once

#include "owncloudlib.h"

#include <QObject>
#include <QSet>
#include <QString>

#include "std/c_string.h"
#include "csync.h"
#include "csync_priority.h"

namespace OCC {
    /**
     * Manages the global system and user exclude lists.
     */
    class OWNCLOUDSYNC_EXPORT PriorityFiles : public QObject {
        Q_OBJECT
    public:
        static PriorityFiles &instance();

        PriorityFiles(c_strlist_t **_prioritysPtr);

        ~PriorityFiles();

        void addPriorityFilePath(const QString &path);

        bool isPriority(
                const QString &filePath,
                const QString &basePath) const;

    public slots:
        /**
         * Reloads the exclude patterns from the registered paths.
         */
        bool reloadPrioritys();

    private:
        // This is a pointer to the csync exclude list, its is owned by this class
        // but the pointer can be in a csync_context so that it can itself also query the list.
        c_strlist_t **_prioritysPtr;
        QSet<QString> _priorityFiles;        // 存配置文件

    };

}
#endif //PRIORITYFILES_H
