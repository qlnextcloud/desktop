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

#define DEFAULT_POLICY_RULE_ID 1
#define DEFAULT_POLICY_RULE_DAYS "1111111"
#define DEFAULT_POLICY_RULE_INTERVAL 60         // seconds
#define DEFAULT_POLICY_RULE_REFERENCED 1

namespace OCC {

    class OCSYNC_EXPORT ConfigDb : public QObject {

    Q_OBJECT
    public:
        explicit ConfigDb(const QString &dbFilePath, QObject *parent = nullptr);
        virtual ~ConfigDb();
        static ConfigDb *instance();
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
        bool insertDefaultPolicyRules();
        bool insertDefaultInfo();



        QVector<PolicyInfo> getPolicyInfo();
        bool addPolicyInfo(PolicyInfo &info);
        bool delAllPolicyInfo();
        bool isExistPolicyRule(int id);
        int getPolicyRulesReferencedById(int id);
        bool updatePolicryRuleReferenced(int id, bool increase);
        QString getPolicyRuleNameById(int id);

    private:
        static ConfigDb *_instance;
        SqlDatabase _db;
        QString _dbFile;
        QMutex _mutex; // Public functions are protected with the mutex.
        int _transaction;

        //----isshe-----
        QScopedPointer<SqlQuery> _getPolicyRulesQuery;
        QScopedPointer<SqlQuery> _addPolicyRulesQuery;
        QScopedPointer<SqlQuery> _addPolicyRulesWithIdQuery;
        QScopedPointer<SqlQuery> _delAllPolicyRulesQuery;
        QScopedPointer<SqlQuery> _getPolicyRuleByIdQuery;
        QScopedPointer<SqlQuery> _getPolicyRulesReferencedByIdQuery;
        QScopedPointer<SqlQuery> _setPolicyRuleReferencedQuery;
        QScopedPointer<SqlQuery> _getPolicyRuleNameByIdQuery;
    };

}

#endif //CONFIGDB_H
