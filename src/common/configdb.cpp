//
// Created by isshe on 2018/9/6.
//

#include <QCryptographicHash>
#include <QFile>
#include <QLoggingCategory>
#include <QStringList>
#include <QElapsedTimer>
#include <QUrl>
#include <QDir>
#include <QString>

#include "common/configdb.h"
#include "asserts.h"


namespace OCC {
    Q_LOGGING_CATEGORY(gbDb, "globalconfig.database", QtInfoMsg)


    ConfigDb::ConfigDb(const QString &dbFilePath, QObject *parent)
            : QObject(parent)
            , _dbFile(dbFilePath)
            , _mutex(QMutex::Recursive)
            , _transaction(0)
            //, _metadataTableIsEmpty(false)
    {
    }


    ConfigDb::~ConfigDb()
    {
        close();
    }

    void ConfigDb::close()
    {
        QMutexLocker locker(&_mutex);
        qCInfo(gbDb) << "Closing DB" << _dbFile;

        commitTransaction();

        _getPolicyRulesQuery.reset(0);
        _addPolicyRulesQuery.reset(0);
        _addPolicyRulesWithIdQuery.reset(0);
        _delPolicyRuleByIdsQuery.reset(0);
        _delPolicyRuleWithoutIdsQuery.reset(0);
        _delAllPolicyRulesQuery.reset(0);
        _setPolicyRulesQuery.reset(0);
        _setPolicyRulesIsReferencedQuery.reset(0);

        _db.close();
    }

    QString ConfigDb::makeDbName(const QString &localPath)
    {
        QString dbname = QLatin1String("config_");

        QString key = QString::fromUtf8("%1").arg(localPath);

        QByteArray ba = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5);
        dbname.append(ba.left(6).toHex());
        dbname.append(".db");

        // If it exists already, the path is clearly usable
        QFile file(QDir(localPath).filePath(dbname));
        if (file.exists()) {
            return dbname;
        }

        // Try to create a file there
        if (file.open(QIODevice::ReadWrite)) {
            // Ok, all good.
            file.close();
            file.remove();
            return dbname;
        }

        // Neither worked, just keep the original and throw errors later
        qCWarning(gbDb) << "Could not find a writable database path" << file.fileName();
        return dbname;
    }


    void ConfigDb::startTransaction()
    {
        if( _transaction == 0 ) {
            if( !_db.transaction() ) {
                qDebug() << "ERROR starting transaction: " << _db.error();
                return;
            }
            _transaction = 1;
            // qDebug() << "XXX Transaction start!";
        } else {
            qDebug() << "Database Transaction is running, not starting another one!";
        }
    }

    void ConfigDb::commitTransaction()
    {
        if( _transaction == 1 ) {
            if( ! _db.commit() ) {
                qDebug() << "ERROR committing to the database: " << _db.error();
                return;
            }
            _transaction = 0;
            // qDebug() << "XXX Transaction END!";
        } else {
            qDebug() << "No database Transaction to commit";
        }
    }

    void ConfigDb::commitInternal(const QString& context, bool startTrans )
    {
        qDebug() << Q_FUNC_INFO << "Transaction commit " << context << (startTrans ? "and starting new transaction" : "");
        commitTransaction();

        if( startTrans ) {
            startTransaction();
        }
    }

    bool ConfigDb::sqlFail( const QString& log, const SqlQuery& query )
    {
        commitTransaction();
        qWarning() << "SQL Error" << log << query.error();
        ASSERT(false);
        _db.close();
        return false;
    }


    QStringList ConfigDb::tableColumns(const QString &table)
    {
        QStringList columns;
        if (!table.isEmpty()) {
            if (checkConnect()) {
                QString q = QString("PRAGMA table_info('%1');").arg(table);
                SqlQuery query(_db);
                query.prepare(q);

                if (!query.exec()) {
                    return columns;
                }

                while (query.next()) {
                    columns.append(query.stringValue(1));
                }
            }
        }
        qCDebug(gbDb) << "Columns in the current journal: " << columns;

        return columns;
    }

    bool ConfigDb::updatePolicyRulesTableStructure()
    {
        //QStringList columns = tableColumns("policyrules");
        bool res = true;

        // check if the file_id column is there and create it if not
        if (!checkConnect()) {
            return false;
        }

        // 以下进行更新，具体参考 syncjournaldb.cpp 中类似的函数

        return res;
    }

    bool ConfigDb::updateDatabaseStructure()
    {
        if (!updatePolicyRulesTableStructure())
        {
            return false;
        }

        return true;
    }

    bool ConfigDb::checkConnect()
    {
        if (_db.isOpen()) {
            return true;
        }

        if (_dbFile.isEmpty()) {
            qCWarning(gbDb) << "Database filename" + _dbFile + " is empty";
            return false;
        }

        // The database file is created by this call (SQLITE_OPEN_CREATE)
        if (!_db.openOrCreateReadWrite(_dbFile)) {
            QString error = _db.error();
            qCWarning(gbDb) << "Error opening the db: " << error;
            return false;
        }

        if (!QFile::exists(_dbFile)) {
            qCWarning(gbDb) << "Database file" + _dbFile + " does not exist";
            return false;
        }

        SqlQuery pragma1(_db);
        pragma1.prepare("SELECT sqlite_version();");
        if (!pragma1.exec()) {
            return sqlFail("SELECT sqlite_version()", pragma1);
        } else {
            pragma1.next();
            qCInfo(gbDb) << "sqlite3 version" << pragma1.stringValue(0);
        }

        /*
        pragma1.prepare(QString("PRAGMA journal_mode=%1;").arg(_journalMode));
        if (!pragma1.exec()) {
            return sqlFail("Set PRAGMA journal_mode", pragma1);
        } else {
            pragma1.next();
            qCInfo(gbDb) << "sqlite3 journal_mode=" << pragma1.stringValue(0);
        }
         */

        // For debugging purposes, allow temp_store to be set
        /*
        static QString env_temp_store = QString::fromLocal8Bit(qgetenv("OWNCLOUD_SQLITE_TEMP_STORE"));
        if (!env_temp_store.isEmpty()) {
            pragma1.prepare(QString("PRAGMA temp_store = %1;").arg(env_temp_store));
            if (!pragma1.exec()) {
                return sqlFail("Set PRAGMA temp_store", pragma1);
            }
            qCInfo(gbDb) << "sqlite3 with temp_store =" << env_temp_store;
        }
         */

        pragma1.prepare("PRAGMA synchronous = 1;");
        if (!pragma1.exec()) {
            return sqlFail("Set PRAGMA synchronous", pragma1);
        }

        // 开大小写敏感
        pragma1.prepare("PRAGMA case_sensitive_like = ON;");
        if (!pragma1.exec()) {
            return sqlFail("Set PRAGMA case_sensitivity", pragma1);
        }

        /* Because insert is so slow, we do everything in a transaction, and only need one call to commit */
        startTransaction();
        SqlQuery createQuery(_db);

        //----isshe----
        // days: [0,1,2,3,4,5,6], json类型
        // days: "0 1 1 1 1 1 0", "1" 表示启用，不用整数，不知道会不会有大小端的问题
        createQuery.prepare("CREATE TABLE IF NOT EXISTS policyrules("
                            "id INTEGER PRIMARY KEY,"
                            "name TEXT UNIQUE,"
                            "days VARCHAR(8) DEFAULT '0111110',"
                            "starttime INTEGER,"
                            "endtime INTEGER,"
                            "interval INTEGER,"
                            "referenced INTEGER DEFAULT 0"
                            ");");
        if (!createQuery.exec()) {
            return sqlFail("Create table policyrules", createQuery);
        }

        // 先提交一次
        commitInternal("checkConnect");

        // 创建表格放在这个之前
        // 更新表格
        bool rc = updateDatabaseStructure();
        if (!rc) {
            qCWarning(gbDb) << "Failed to update the database structure!";
        }

        // 以下进行数据库操作的准备工作
        // ----isshe----
        _getPolicyRulesQuery.reset(new SqlQuery(_db));
        if (_getPolicyRulesQuery->prepare("SELECT id, name, days, interval, referenced FROM policyrules;")) {
            return sqlFail("prepare _getPolicyRulesQuery", *_getPolicyRulesQuery);
        }

        _addPolicyRulesQuery.reset(new SqlQuery(_db));
        if (_addPolicyRulesQuery->prepare("INSERT OR IGNORE INTO policyrules "
                                          "(name, days, interval) VALUES (?1, ?2, ?3);")) {
            return sqlFail("prepare _addPolicyRulesQuery", *_addPolicyRulesQuery);
        }

        _addPolicyRulesWithIdQuery.reset(new SqlQuery(_db));
        if (_addPolicyRulesWithIdQuery->prepare("INSERT OR IGNORE INTO policyrules "
                                          "(id, name, days, interval, referenced) "
                                          "VALUES (?1, ?2, ?3, ?4, ?5);")) {
            return sqlFail("prepare _addPolicyRulesWithIdQuery", *_addPolicyRulesWithIdQuery);
        }

        _setPolicyRulesQuery.reset(new SqlQuery(_db));
        if (_setPolicyRulesQuery->prepare("UPDATE policyrules SET "
                                          "name='?2', days='?3', interval='?4', "
                                          "referenced='?5' WHERE id=?1;")) {
            return sqlFail("prepare _setPolicyRulesQuery", *_setPolicyRulesQuery);
        }

        _setPolicyRulesIsReferencedQuery.reset(new SqlQuery(_db));
        if (_setPolicyRulesIsReferencedQuery->prepare("UPDATE policyrules SET "
                                          "referenced='?2' WHERE id=?1;")) {
            return sqlFail("prepare _setPolicyRulesIsReferencedQuery", *_setPolicyRulesIsReferencedQuery);
        }
        _delPolicyRuleByIdsQuery.reset(new SqlQuery(_db));
        if (_delPolicyRuleByIdsQuery->prepare("DELETE FROM policyrules WHERE id IN ( ?1 );")) {
            return sqlFail("prepare _delPolicyRuleByIdQuery", *_delPolicyRuleByIdsQuery);
        }

        _delPolicyRuleWithoutIdsQuery.reset(new SqlQuery(_db));
        if (_delPolicyRuleWithoutIdsQuery->prepare("DELETE FROM policyrules WHERE id NOT IN ( ?1 );")) {
            return sqlFail("prepare _delPolicyRuleWithoutIdsQuery", *_delPolicyRuleWithoutIdsQuery);
        }

        _delAllPolicyRulesQuery.reset(new SqlQuery(_db));
        if (_delAllPolicyRulesQuery->prepare("DELETE FROM policyrules;")) {
            return sqlFail("prepare _delAllPolicyRulesQuery", *_delAllPolicyRulesQuery);
        }


        commitInternal(QString("checkConnect End"), false);

        return rc;
    }


    bool ConfigDb::isConnected()
    {
        QMutexLocker lock(&_mutex);
        return checkConnect();
    }

    QVector<ConfigDb::PolicyInfo> ConfigDb::getPolicyInfo(){
        QMutexLocker locker(&_mutex);

        QVector<ConfigDb::PolicyInfo> res;

        if (!checkConnect()) {
            return res;
        }

        _getPolicyRulesQuery->reset_and_clear_bindings();

        if (!_getPolicyRulesQuery->exec()) {
            return res;
        }

        while (_getPolicyRulesQuery->next()) {
            PolicyInfo info;
            info._id = _getPolicyRulesQuery->int64Value(0);
            info._name = _getPolicyRulesQuery->stringValue(1);
            info._days = _getPolicyRulesQuery->baValue(2);
            info._interval = _getPolicyRulesQuery->intValue(3);
            info._referenced = _getPolicyRulesQuery->intValue(4);
            res.append(info);
        }

        //_getPolicyRulesQuery->finish();
        return res;
    }

    /**
     *
     * @param info
     * @return
     */
    bool ConfigDb::addPolicyInfo(PolicyInfo &info)
    {
        QMutexLocker locker(&_mutex);
        if (!checkConnect()) {
            return false;
        }

        SqlQuery *tempQuery = nullptr;

        // 修改
        if (info._id > 0) {
            _addPolicyRulesWithIdQuery->reset_and_clear_bindings();

            _addPolicyRulesWithIdQuery->bindValue(1, info._id);
            _addPolicyRulesWithIdQuery->bindValue(2, info._name);
            _addPolicyRulesWithIdQuery->bindValue(3, info._days);
            _addPolicyRulesWithIdQuery->bindValue(4, info._interval);
            _addPolicyRulesWithIdQuery->bindValue(5, info._referenced);

            tempQuery = _addPolicyRulesWithIdQuery.data();

        } else {        // 新增
            _addPolicyRulesQuery->reset_and_clear_bindings();
            _addPolicyRulesQuery->bindValue(1, info._name);
            _addPolicyRulesQuery->bindValue(2, info._days);
            _addPolicyRulesQuery->bindValue(3, info._interval);

            tempQuery = _addPolicyRulesQuery.data();
        }

        return tempQuery->exec();
    }

    /**
     *
     * @param ids
     * @return
     */
    bool ConfigDb::delPolicyInfoByIds(const QString &ids)
    {
        if (ids.isEmpty()) {
            return true;            // nothing delete, return true
        }

        QMutexLocker locker(&_mutex);
        if (!checkConnect()) {
            return false;
        }

        _delPolicyRuleByIdsQuery->reset_and_clear_bindings();
        _delPolicyRuleByIdsQuery->bindValue(1, ids);

        /*
        if (!_delPolicyRuleByIdsQuery->exec()) {
            return false;
        }

        _delPolicyRuleByIdsQuery->finish();
        return true;
        */
        return _delPolicyRuleByIdsQuery->exec();
    }

    /**
     *
     * @param ids
     * @return
     */
    bool ConfigDb::delPolicyInfoWithoutIds(const QString &ids)
    {
        if (ids.isEmpty()) {
            return true;            // nothing delete, return true
        }

        QMutexLocker locker(&_mutex);
        if (!checkConnect()) {
            return false;
        }

        _delPolicyRuleWithoutIdsQuery->reset_and_clear_bindings();
        _delPolicyRuleWithoutIdsQuery->bindValue(1, ids);

        /*
        if (!_delPolicyRuleWithoutIdsQuery->exec()) {
            return false;
        }

        _delPolicyRuleWithoutIdsQuery->finish();
        return true;
        */
        return _delPolicyRuleWithoutIdsQuery->exec();
    }

    /**
     *
     * @return
     */
    bool ConfigDb::delAllPolicyInfo()
    {
        QMutexLocker locker(&_mutex);
        if (!checkConnect()) {
            return false;
        }
        _delAllPolicyRulesQuery->reset_and_clear_bindings();

        /*
        if (!_delAllPolicyRulesQuery->exec()) {
            return false;
        }

        _delAllPolicyRulesQuery->finish();
        return true;
        */

        return _delAllPolicyRulesQuery->exec();
    }

    /**
     *
     * @param info
     * @return
     */
    bool ConfigDb::setPolicyInfo(PolicyInfo &info)
    {
        QMutexLocker locker(&_mutex);
        if (!checkConnect()) {
            return false;
        }

        _setPolicyRulesQuery->reset_and_clear_bindings();
        _setPolicyRulesQuery->bindValue(1, info._id);
        _setPolicyRulesQuery->bindValue(2, info._name);
        _setPolicyRulesQuery->bindValue(3, info._days);
        _setPolicyRulesQuery->bindValue(4, info._interval);
        _setPolicyRulesQuery->bindValue(5, info._referenced);

        /*
        if (!_setPolicyRulesQuery->exec()) {
            return false;
        }

        _setPolicyRulesQuery->finish();
        return true;
        */
        return _setPolicyRulesQuery->exec();
    }

    /**
     *
     * @param info
     * @return
     */
    bool ConfigDb::setIsReferencedField(PolicyInfo &info)
    {
        QMutexLocker locker(&_mutex);
        if (!checkConnect()) {
            return false;
        }
        _setPolicyRulesQuery->reset_and_clear_bindings();
        _setPolicyRulesQuery->bindValue(1, info._id);
        _setPolicyRulesQuery->bindValue(2, info._referenced);

        /*
        if (!_setPolicyRulesQuery->exec()) {
            return false;
        }

        _setPolicyRulesQuery->finish();
        return true;
        */
        return _setPolicyRulesQuery->exec();
    }

}