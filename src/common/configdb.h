//
// Created by isshe on 2018/9/6.
//

#ifndef CONFIGDB_H
#define CONFIGDB_H

#include <QObject>
#include <qmutex.h>
#include <QDateTime>
#include <QHash>
#include <functional>

#include "common/ownsql.h"

namespace OCC {

    class OCSYNC_EXPORT ConfigDb : public QObject {

    Q_OBJECT
    public:
        explicit ConfigDb(const QString &dbFilePath, QObject *parent = 0);
        virtual ~ConfigDb();
        void close();

        static QString makeDbName(const QString &localPath);

        void startTransaction();
        void commitTransaction();
        void commitInternal(const QString &context, bool startTrans = true);
        bool sqlFail( const QString& log, const SqlQuery& query );
        bool updateDatabaseStructure();
        bool checkConnect();
        QStringList tableColumns(const QString &table);
        bool updatePolicyRulesTableStructure();
        bool isConnected();

        // ----isshe----
        struct PolicyInfo
        {
            qint64 _id;
            QString _name;
            QByteArray _days;
            int _interval;
            int _referenced;
        };

        QVector<PolicyInfo> getPolicyInfo();
        bool addPolicyInfo(PolicyInfo &info);
        bool delPolicyInfoByIds(const QString &ids);
        bool delPolicyInfoWithoutIds(const QString &ids);
        bool delAllPolicyInfo();
        bool setPolicyInfo(PolicyInfo &info);
        bool setIsReferencedField(PolicyInfo &info);


    private:
        SqlDatabase _db;
        QString _dbFile;
        QMutex _mutex; // Public functions are protected with the mutex.
        int _transaction;
        //bool _metadataTableIsEmpty;

        //----isshe-----
        QScopedPointer<SqlQuery> _getPolicyRulesQuery;
        QScopedPointer<SqlQuery> _addPolicyRulesQuery;
        QScopedPointer<SqlQuery> _addPolicyRulesWithIdQuery;
        QScopedPointer<SqlQuery> _delPolicyRuleByIdsQuery;
        QScopedPointer<SqlQuery> _delPolicyRuleWithoutIdsQuery;
        QScopedPointer<SqlQuery> _delAllPolicyRulesQuery;
        QScopedPointer<SqlQuery> _setPolicyRulesQuery;
        QScopedPointer<SqlQuery> _setPolicyRulesIsReferencedQuery;


    };

}

#endif //CONFIGDB_H
