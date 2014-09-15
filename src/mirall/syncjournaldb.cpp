/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include <QFile>
#include <QStringList>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

#include <inttypes.h>

#include "syncjournaldb.h"
#include "syncjournalfilerecord.h"
#include "utility.h"
#include "version.h"

#include "../../csync/src/std/c_jhash.h"

#define QSQLITE "QSQLITE"

namespace Mirall {

SyncJournalDb::SyncJournalDb(const QString& path, QObject *parent) :
    QObject(parent), _transaction(0), _possibleUpgradeFromMirall_1_5(false)
{

    _dbFile = path;
    if( !_dbFile.endsWith('/') ) {
        _dbFile.append('/');
    }
    _dbFile.append(".csync_journal.db");


}

bool SyncJournalDb::exists()
{
    QMutexLocker locker(&_mutex);
    return (!_dbFile.isEmpty() && QFile::exists(_dbFile));
}

void SyncJournalDb::startTransaction()
{
    if( _transaction == 0 ) {
        if( !_db.transaction() ) {
            qDebug() << "ERROR starting transaction: " << _db.lastError().text();
            return;
        }
        _transaction = 1;
        // qDebug() << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX Transaction start!";
    } else {
        qDebug() << "Database Transaction is running, do not starting another one!";
    }
}

void SyncJournalDb::commitTransaction()
{
    if( _transaction == 1 ) {
        if( ! _db.commit() ) {
            qDebug() << "ERROR committing to the database: " << _db.lastError().text();
            return;
        }
        _transaction = 0;
        // qDebug() << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX Transaction END!";
    } else {
        qDebug() << "No database Transaction to commit";
    }
}

bool SyncJournalDb::sqlFail( const QString& log, const QSqlQuery& query )
{
    commitTransaction();
#ifdef NDEBUG
    qWarning()
#else
    qFatal()
#endif
        << "SQL Error" << log << query.lastError().text();

    _db.close();
    return false;
}

bool SyncJournalDb::checkConnect()
{
    if( _db.isOpen() ) {
        return true;
    }

    if( _dbFile.isEmpty() || !QFile::exists(_dbFile) ) {
        qDebug() << "Database " + _dbFile + " is empty or does not exist";
        return false;
    }

    QStringList list = QSqlDatabase::drivers();
    if( list.size() == 0 ) {
        qDebug() << "Database Drivers could not be loaded.";
        return false ;
    } else {
        if( list.indexOf( QSQLITE ) == -1 ) {
            qDebug() << "Database Driver QSQLITE could not be loaded!";
            return false;
        }
    }

    // Add the connection
    _db = QSqlDatabase::addDatabase( QSQLITE,  _dbFile);

    // Open the file
    _db.setDatabaseName(_dbFile);

    if (!_db.isOpen()) {
        if( !_db.open() ) {
            QSqlError error = _db.lastError();
            qDebug() << "Error opening the db: " << error.text();
            return false;
        }
    }

    QSqlQuery pragma1(_db);
    pragma1.prepare("PRAGMA synchronous = 1;");
    if (!pragma1.exec()) {
        return sqlFail("Set PRAGMA synchronous", pragma1);
    }
    pragma1.prepare("PRAGMA case_sensitive_like = ON;");
    if (!pragma1.exec()) {
        return sqlFail("Set PRAGMA case_sensitivity", pragma1);
    }

    /* Because insert are so slow, e do everything in a transaction, and one need to call commit */
    startTransaction();

    QSqlQuery createQuery(_db);
    createQuery.prepare("CREATE TABLE IF NOT EXISTS metadata("
                         "phash INTEGER(8),"
                         "pathlen INTEGER,"
                         "path VARCHAR(4096),"
                         "inode INTEGER,"
                         "uid INTEGER,"
                         "gid INTEGER,"
                         "mode INTEGER,"
                         "modtime INTEGER(8),"
                         "type INTEGER,"
                         "md5 VARCHAR(32)," /* This is the etag.  Called md5 for compatibility */
                        // updateDatabaseStructure() will add a fileid column
                        // updateDatabaseStructure() will add a remotePerm column
                         "PRIMARY KEY(phash)"
                         ");");

    if (!createQuery.exec()) {
        return sqlFail("Create table metadata", createQuery);
    }

    createQuery.prepare("CREATE TABLE IF NOT EXISTS downloadinfo("
                         "path VARCHAR(4096),"
                         "tmpfile VARCHAR(4096),"
                         "etag VARCHAR(32),"
                         "errorcount INTEGER,"
                         "PRIMARY KEY(path)"
                         ");");

    if (!createQuery.exec()) {
        return sqlFail("Create table downloadinfo", createQuery);
    }

    createQuery.prepare("CREATE TABLE IF NOT EXISTS uploadinfo("
                           "path VARCHAR(4096),"
                           "chunk INTEGER,"
                           "transferid INTEGER,"
                           "errorcount INTEGER,"
                           "size INTEGER(8),"
                           "modtime INTEGER(8),"
                           "PRIMARY KEY(path)"
                           ");");

    if (!createQuery.exec()) {
        return sqlFail("Create table uploadinfo", createQuery);
    }

    // create the blacklist table.
    createQuery.prepare("CREATE TABLE IF NOT EXISTS blacklist ("
                        "path VARCHAR(4096),"
                        "lastTryEtag VARCHAR[32],"
                        "lastTryModtime INTEGER[8],"
                        "retrycount INTEGER,"
                        "errorstring VARCHAR[4096],"
                        "PRIMARY KEY(path)"
                        ");");

    if (!createQuery.exec()) {
        return sqlFail("Create table blacklist", createQuery);
    }

    createQuery.prepare("CREATE TABLE IF NOT EXISTS version("
                               "major INTEGER(8),"
                               "minor INTEGER(8),"
                               "patch INTEGER(8),"
                               "custom VARCHAR(256)"
                               ");");
    if (!createQuery.exec()) {
        return sqlFail("Create table blacklist", createQuery);
    }

    QSqlQuery versionQuery("SELECT major, minor FROM version;", _db);
    if (!versionQuery.next()) {
        // If there was no entry in the table, it means we are likely upgrading from 1.5
        _possibleUpgradeFromMirall_1_5 = true;
    } else {
        // Delete the existing entry so we can replace it by the new one
        createQuery.prepare("DELETE FROM version;");
        if (!createQuery.exec()) {
            return sqlFail("Remove version", createQuery);
        }
    }
    createQuery.prepare("INSERT INTO version (major, minor, patch) VALUES ( ? , ? , ? );");
    createQuery.bindValue(0, MIRALL_VERSION_MAJOR);
    createQuery.bindValue(1, MIRALL_VERSION_MINOR);
    createQuery.bindValue(2, MIRALL_VERSION_PATCH);
    if (!createQuery.exec()) {
        return sqlFail("Insert Version", createQuery);
    }

    commitInternal("checkConnect");

    bool rc = updateDatabaseStructure();

    _getFileRecordQuery.reset(new QSqlQuery(_db));
    _getFileRecordQuery->prepare("SELECT path, inode, uid, gid, mode, modtime, type, md5, fileid, remotePerm FROM "
                                 "metadata WHERE phash=:ph" );

    _setFileRecordQuery.reset(new QSqlQuery(_db) );
    _setFileRecordQuery->prepare("INSERT OR REPLACE INTO metadata "
                                 "(phash, pathlen, path, inode, uid, gid, mode, modtime, type, md5, fileid, remotePerm) "
                                 "VALUES ( ? , ?, ? , ? , ? , ? , ?,  ? , ? , ?, ?, ? )" );

    _getDownloadInfoQuery.reset(new QSqlQuery(_db) );
    _getDownloadInfoQuery->prepare( "SELECT tmpfile, etag, errorcount FROM "
                                    "downloadinfo WHERE path=:pa" );

    _setDownloadInfoQuery.reset(new QSqlQuery(_db) );
    _setDownloadInfoQuery->prepare( "INSERT OR REPLACE INTO downloadinfo "
                                    "(path, tmpfile, etag, errorcount) "
                                    "VALUES ( ? , ?, ? , ? )" );

    _deleteDownloadInfoQuery.reset(new QSqlQuery(_db) );
    _deleteDownloadInfoQuery->prepare( "DELETE FROM downloadinfo WHERE path=?" );

    _getUploadInfoQuery.reset(new QSqlQuery(_db));
    _getUploadInfoQuery->prepare( "SELECT chunk, transferid, errorcount, size, modtime FROM "
                                  "uploadinfo WHERE path=:pa" );

    _setUploadInfoQuery.reset(new QSqlQuery(_db));
    _setUploadInfoQuery->prepare( "INSERT OR REPLACE INTO uploadinfo "
                                  "(path, chunk, transferid, errorcount, size, modtime) "
                                  "VALUES ( ? , ?, ? , ? ,  ? , ? )");

    _deleteUploadInfoQuery.reset(new QSqlQuery(_db));
    _deleteUploadInfoQuery->prepare("DELETE FROM uploadinfo WHERE path=?" );


    _deleteFileRecordPhash.reset(new QSqlQuery(_db));
    _deleteFileRecordPhash->prepare("DELETE FROM metadata WHERE phash=?");

    _deleteFileRecordRecursively.reset(new QSqlQuery(_db));
    _deleteFileRecordRecursively->prepare("DELETE FROM metadata WHERE path LIKE(?||'/%')");

    _blacklistQuery.reset(new QSqlQuery(_db));
    _blacklistQuery->prepare("SELECT lastTryEtag, lastTryModtime, retrycount, errorstring "
                             "FROM blacklist WHERE path=:path");

    return rc;
}

void SyncJournalDb::close()
{
    QMutexLocker locker(&_mutex);

    commitTransaction();

    _getFileRecordQuery.reset(0);
    _setFileRecordQuery.reset(0);
    _getDownloadInfoQuery.reset(0);
    _setDownloadInfoQuery.reset(0);
    _deleteDownloadInfoQuery.reset(0);
    _getUploadInfoQuery.reset(0);
    _setUploadInfoQuery.reset(0);
    _deleteUploadInfoQuery.reset(0);
    _deleteFileRecordPhash.reset(0);
    _deleteFileRecordRecursively.reset(0);
    _blacklistQuery.reset(0);
    _possibleUpgradeFromMirall_1_5 = false;

    _db.close();
    _db = QSqlDatabase(); // avoid the warning QSqlDatabasePrivate::removeDatabase: connection [...] still in use
    QSqlDatabase::removeDatabase(_dbFile);
    _avoidReadFromDbOnNextSyncFilter.clear();
}


bool SyncJournalDb::updateDatabaseStructure()
{
    QStringList columns = tableColumns("metadata");
    bool re = true;

    // check if the file_id column is there and create it if not
    if( !checkConnect() ) {
        return false;
    }
    if( columns.indexOf(QLatin1String("fileid")) == -1 ) {

        QSqlQuery query(_db);
        query.prepare("ALTER TABLE metadata ADD COLUMN fileid VARCHAR(128);");
        re = query.exec();
        if(!re) {
            qDebug() << Q_FUNC_INFO << "SQL Error " << query.lastError().text();
        }

        query.prepare("CREATE INDEX metadata_file_id ON metadata(fileid);");
        re = re && query.exec();

        if(!re) {
            qDebug() << Q_FUNC_INFO << "SQL Error " << query.lastError().text();
        }
        commitInternal("update database structure: add fileid col");
    }
    if( columns.indexOf(QLatin1String("remotePerm")) == -1 ) {

        QSqlQuery query(_db);
        query.prepare("ALTER TABLE metadata ADD COLUMN remotePerm VARCHAR(128);");
        re = re && query.exec();
        if(!re) {
            qDebug() << Q_FUNC_INFO << "SQL Error " << query.lastError().text();
        }
        commitInternal("update database structure (remotePerm");
    }

    if( 1 ) {
        QSqlQuery query(_db);
        query.prepare("CREATE INDEX IF NOT EXISTS metadata_inode ON metadata(inode);");
        re = re && query.exec();

        if(!re) {
            qDebug() << Q_FUNC_INFO << "SQL Error " << query.lastError().text();
        }
        commitInternal("update database structure: add inode index");

    }

    if( 1 ) {
        QSqlQuery query(_db);
        query.prepare("CREATE INDEX IF NOT EXISTS metadata_pathlen ON metadata(pathlen);");
        re = re && query.exec();

        if(!re) {
            qDebug() << Q_FUNC_INFO << "SQL Error " << query.lastError().text();
        }
        commitInternal("update database structure: add pathlen index");

    }
    return re;
}

QStringList SyncJournalDb::tableColumns( const QString& table )
{
    QStringList columns;
    if( !table.isEmpty() ) {

        if( checkConnect() ) {
            QString q = QString("PRAGMA table_info(%1);").arg(table);
            QSqlQuery query(_db);
            query.prepare(q);

            if(!query.exec()) {
                QString err = query.lastError().text();
                qDebug() << "Error creating prepared statement: " << query.lastQuery() << ", Error:" << err;;
                return columns;
            }

            while( query.next() ) {
                columns.append( query.value(1).toString() );
            }
        }
    }
    qDebug() << "Columns in the current journal: " << columns;

    return columns;
}

qint64 SyncJournalDb::getPHash(const QString& file) const
{
    QByteArray utf8File = file.toUtf8();
    int64_t h;

    if( file.isEmpty() ) {
        return -1;
    }

    int len = utf8File.length();

    h = c_jhash64((uint8_t *) utf8File.data(), len, 0);
    return h;
}

bool SyncJournalDb::setFileRecord( const SyncJournalFileRecord& _record )
{
    SyncJournalFileRecord record = _record;
    QMutexLocker locker(&_mutex);

    if (!_avoidReadFromDbOnNextSyncFilter.isEmpty()) {
        // If we are a directory that should not be read from db next time, don't write the etag
        QString prefix = record._path + "/";
        foreach(const QString &it, _avoidReadFromDbOnNextSyncFilter) {
            if (it.startsWith(prefix)) {
                qDebug() << "Filtered writing the etag of" << prefix << "because it is a prefix of" << it;
                record._etag = "_invalid_";
                break;
            }
        }
    }

    qlonglong phash = getPHash(record._path);
    if( checkConnect() ) {
        QByteArray arr = record._path.toUtf8();
        int plen = arr.length();

        QString etag( record._etag );
        if( etag.isEmpty() ) etag = "";
        QString fileId( record._fileId);
        if( fileId.isEmpty() ) fileId = "";
        QString remotePerm (record._remotePerm);
        if (remotePerm.isEmpty()) remotePerm = QString(); // have NULL in DB (vs empty)

        _setFileRecordQuery->bindValue(0, QString::number(phash));
        _setFileRecordQuery->bindValue(1, plen);
        _setFileRecordQuery->bindValue(2, record._path );
        _setFileRecordQuery->bindValue(3, record._inode );
        _setFileRecordQuery->bindValue(4, 0 ); // uid Not used
        _setFileRecordQuery->bindValue(5, 0 ); // gid Not used
        _setFileRecordQuery->bindValue(6, record._mode );
        _setFileRecordQuery->bindValue(7, QString::number(Utility::qDateTimeToTime_t(record._modtime)));
        _setFileRecordQuery->bindValue(8, QString::number(record._type) );
        _setFileRecordQuery->bindValue(9, etag );
        _setFileRecordQuery->bindValue(10, fileId );
        _setFileRecordQuery->bindValue(11, remotePerm );

        if( !_setFileRecordQuery->exec() ) {
            qWarning() << "Error SQL statement setFileRecord: " << _setFileRecordQuery->lastQuery() <<  " :"
                       << _setFileRecordQuery->lastError().text();
            return false;
        }

        qDebug() <<  _setFileRecordQuery->lastQuery() << phash << plen << record._path << record._inode
                 << record._mode
                 << QString::number(Utility::qDateTimeToTime_t(record._modtime)) << QString::number(record._type)
                 << record._etag << record._fileId << record._remotePerm;
        _setFileRecordQuery->finish();

        return true;
    } else {
        qDebug() << "Failed to connect database.";
        return false; // checkConnect failed.
    }
}

bool SyncJournalDb::deleteFileRecord(const QString& filename, bool recursively)
{
    QMutexLocker locker(&_mutex);

    if( checkConnect() ) {
        // if (!recursively) {
        // always delete the actual file.

        qlonglong phash = getPHash(filename);
        _deleteFileRecordPhash->bindValue( 0, QString::number(phash) );

        if( !_deleteFileRecordPhash->exec() ) {
            qWarning() << "Exec error of SQL statement: "
                       << _deleteFileRecordPhash->lastQuery()
                       <<  " : " << _deleteFileRecordPhash->lastError().text();
            return false;
        }
        qDebug() <<  _deleteFileRecordPhash->executedQuery() << phash << filename;
        _deleteFileRecordPhash->finish();
        if( recursively) {
            _deleteFileRecordRecursively->bindValue(0, filename);
            if( !_deleteFileRecordRecursively->exec() ) {
                qWarning() << "Exec error of SQL statement: "
                           << _deleteFileRecordRecursively->lastQuery()
                           <<  " : " << _deleteFileRecordRecursively->lastError().text();
                return false;
            }
            qDebug() <<  _deleteFileRecordRecursively->executedQuery()  << filename;
            _deleteFileRecordRecursively->finish();
        }
        return true;
    } else {
        qDebug() << "Failed to connect database.";
        return false; // checkConnect failed.
    }
}


SyncJournalFileRecord SyncJournalDb::getFileRecord( const QString& filename )
{
    QMutexLocker locker(&_mutex);

    qlonglong phash = getPHash( filename );
    SyncJournalFileRecord rec;

    if( checkConnect() ) {
        _getFileRecordQuery->bindValue(":ph", QString::number(phash));

        if (!_getFileRecordQuery->exec()) {
            QString err = _getFileRecordQuery->lastError().text();
            qDebug() << "Error creating prepared statement: " << _getFileRecordQuery->lastQuery() << ", Error:" << err;;
            return rec;
        }

        if( _getFileRecordQuery->next() ) {
            bool ok;
            rec._path    = _getFileRecordQuery->value(0).toString();
            rec._inode   = _getFileRecordQuery->value(1).toInt(&ok);
            //rec._uid     = _getFileRecordQuery->value(2).toInt(&ok); Not Used
            //rec._gid     = _getFileRecordQuery->value(3).toInt(&ok); Not Used
            rec._mode    = _getFileRecordQuery->value(4).toInt(&ok);
            rec._modtime = Utility::qDateTimeFromTime_t(_getFileRecordQuery->value(5).toLongLong(&ok));
            rec._type    = _getFileRecordQuery->value(6).toInt(&ok);
            rec._etag    = _getFileRecordQuery->value(7).toByteArray();
            rec._fileId  = _getFileRecordQuery->value(8).toByteArray();
            rec._remotePerm = _getFileRecordQuery->value(9).toByteArray();

            _getFileRecordQuery->finish();
        } else {
            QString err = _getFileRecordQuery->lastError().text();
	    qDebug() << "No journal entry found for " << filename;
        }
    }
    return rec;
}

bool SyncJournalDb::postSyncCleanup(const QSet<QString> &items )
{
    QMutexLocker locker(&_mutex);

    if( !checkConnect() ) {
        return false;
    }

    QSqlQuery query(_db);
    query.prepare("SELECT phash, path FROM metadata order by path");

    if (!query.exec()) {
        QString err = query.lastError().text();
        qDebug() << "Error creating prepared statement: " << query.lastQuery() << ", Error:" << err;;
        return false;
    }

    QStringList superfluousItems;

    while(query.next()) {
        const QString file = query.value(1).toString();
        bool contained = items.contains(file);
        if( !contained ) {
            superfluousItems.append(query.value(0).toString());
        }
    }

    if( superfluousItems.count() )  {
        QString sql = "DELETE FROM metadata WHERE phash in ("+ superfluousItems.join(",")+")";
        qDebug() << "Sync Journal cleanup: " << sql;
        QSqlQuery delQuery(_db);
        delQuery.prepare(sql);
        if( !delQuery.exec() ) {
            QString err = delQuery.lastError().text();
            qDebug() << "Error removing superfluous journal entries: " << delQuery.lastQuery() << ", Error:" << err;;
            return false;
        }
    }
    return true;
}

int SyncJournalDb::getFileRecordCount()
{
    QMutexLocker locker(&_mutex);

    if( !checkConnect() ) {
        return -1;
    }

    QSqlQuery query(_db);
    query.prepare("SELECT COUNT(*) FROM metadata");

    if (!query.exec()) {
        QString err = query.lastError().text();
        qDebug() << "Error creating prepared statement: " << query.lastQuery() << ", Error:" << err;;
        return 0;
    }

    if (query.next()) {
        int count = query.value(0).toInt();
        return count;
    }

    return 0;
}

static void toDownloadInfo(const QSqlQuery & query, SyncJournalDb::DownloadInfo * res)
{
    bool ok = true;
    res->_tmpfile    = query.value(0).toString();
    res->_etag       = query.value(1).toByteArray();
    res->_errorCount = query.value(2).toInt(&ok);
    res->_valid      = ok;
}

static bool deleteBatch(QSqlQuery & query, const QStringList & entries, const QString & name)
{
    if (entries.isEmpty())
        return true;

    qDebug() << "Removing stale " << qPrintable(name) << " entries: " << entries.join(", ");
    query.bindValue(0, entries);
    if (!query.execBatch()) {
        QString err = query.lastError().text();
        qDebug() << "Error removing stale " << qPrintable(name) << " entries: "
                 << query.lastQuery() << ", Error:" << err;
        return false;
    }
    query.finish();
    return true;
}

SyncJournalDb::DownloadInfo SyncJournalDb::getDownloadInfo(const QString& file)
{
    QMutexLocker locker(&_mutex);

    DownloadInfo res;

    if( checkConnect() ) {
        _getDownloadInfoQuery->bindValue(":pa", file);

        if (!_getDownloadInfoQuery->exec()) {
            QString err = _getDownloadInfoQuery->lastError().text();
            qDebug() << "Database error for file " << file << " : " << _getDownloadInfoQuery->lastQuery() << ", Error:" << err;;
            return res;
        }

        if( _getDownloadInfoQuery->next() ) {
            toDownloadInfo(*_getDownloadInfoQuery, &res);
        }
        _getDownloadInfoQuery->finish();
    }
    return res;
}

void SyncJournalDb::setDownloadInfo(const QString& file, const SyncJournalDb::DownloadInfo& i)
{
    QMutexLocker locker(&_mutex);

    if( !checkConnect() ) {
        return;
    }

    if (i._valid) {
        _setDownloadInfoQuery->bindValue(0, file);
        _setDownloadInfoQuery->bindValue(1, i._tmpfile);
        _setDownloadInfoQuery->bindValue(2, i._etag );
        _setDownloadInfoQuery->bindValue(3, i._errorCount );

        if( !_setDownloadInfoQuery->exec() ) {
            qWarning() << "Exec error of SQL statement: " << _setDownloadInfoQuery->lastQuery() <<  " :"   << _setDownloadInfoQuery->lastError().text();
            return;
        }

        qDebug() <<  _setDownloadInfoQuery->lastQuery() << file << i._tmpfile << i._etag << i._errorCount;
        _setDownloadInfoQuery->finish();

    } else {
        _deleteDownloadInfoQuery->bindValue( 0, file );

        if( !_deleteDownloadInfoQuery->exec() ) {
            qWarning() << "Exec error of SQL statement: " << _deleteDownloadInfoQuery->lastQuery() <<  " : " << _deleteDownloadInfoQuery->lastError().text();
            return;
        }
        qDebug() <<  _deleteDownloadInfoQuery->executedQuery()  << file;
        _deleteDownloadInfoQuery->finish();
    }
}

QVector<SyncJournalDb::DownloadInfo> SyncJournalDb::getAndDeleteStaleDownloadInfos(const QSet<QString>& keep)
{
    QVector<SyncJournalDb::DownloadInfo> empty_result;
    QMutexLocker locker(&_mutex);

    if (!checkConnect()) {
        return empty_result;
    }

    QSqlQuery query(_db);
    // The selected values *must* match the ones expected by toDownloadInfo().
    query.prepare("SELECT tmpfile, etag, errorcount, path FROM downloadinfo");

    if (!query.exec()) {
        QString err = query.lastError().text();
        qDebug() << "Error creating prepared statement: " << query.lastQuery() << ", Error:" << err;
        return empty_result;
    }

    QStringList superfluousPaths;
    QVector<SyncJournalDb::DownloadInfo> deleted_entries;

    while (query.next()) {
        const QString file = query.value(3).toString(); // path
        if (!keep.contains(file)) {
            superfluousPaths.append(file);
            DownloadInfo info;
            toDownloadInfo(query, &info);
            deleted_entries.append(info);
        }
    }

    if (!deleteBatch(*_deleteDownloadInfoQuery, superfluousPaths, "downloadinfo"))
        return empty_result;

    return deleted_entries;
}

SyncJournalDb::UploadInfo SyncJournalDb::getUploadInfo(const QString& file)
{
    QMutexLocker locker(&_mutex);

    UploadInfo res;

    if( checkConnect() ) {

        _getUploadInfoQuery->bindValue(":pa", file);

        if (!_getUploadInfoQuery->exec()) {
            QString err = _getUploadInfoQuery->lastError().text();
            qDebug() << "Database error for file " << file << " : " << _getUploadInfoQuery->lastQuery() << ", Error:" << err;
            return res;
        }

        if( _getUploadInfoQuery->next() ) {
            bool ok = true;
            res._chunk      = _getUploadInfoQuery->value(0).toInt(&ok);
            res._transferid = _getUploadInfoQuery->value(1).toInt(&ok);
            res._errorCount = _getUploadInfoQuery->value(2).toInt(&ok);
            res._size       = _getUploadInfoQuery->value(3).toLongLong(&ok);
            res._modtime    = Utility::qDateTimeFromTime_t(_getUploadInfoQuery->value(4).toLongLong(&ok));
            res._valid      = ok;
        }
        _getUploadInfoQuery->finish();
    }
    return res;
}

void SyncJournalDb::setUploadInfo(const QString& file, const SyncJournalDb::UploadInfo& i)
{
    QMutexLocker locker(&_mutex);

    if( !checkConnect() ) {
        return;
    }

    if (i._valid) {
        _setUploadInfoQuery->bindValue(0, file);
        _setUploadInfoQuery->bindValue(1, i._chunk);
        _setUploadInfoQuery->bindValue(2, i._transferid );
        _setUploadInfoQuery->bindValue(3, i._errorCount );
        _setUploadInfoQuery->bindValue(4, i._size );
        _setUploadInfoQuery->bindValue(5, Utility::qDateTimeToTime_t(i._modtime) );

        if( !_setUploadInfoQuery->exec() ) {
            qWarning() << "Exec error of SQL statement: " << _setUploadInfoQuery->lastQuery() <<  " :"   << _setUploadInfoQuery->lastError().text();
            return;
        }

        qDebug() <<  _setUploadInfoQuery->lastQuery() << file << i._chunk << i._transferid << i._errorCount;
        _setUploadInfoQuery->finish();
    } else {
        _deleteUploadInfoQuery->bindValue(0, file);

        if( !_deleteUploadInfoQuery->exec() ) {
            qWarning() << "Exec error of SQL statement: " << _deleteUploadInfoQuery->lastQuery() <<  " : " << _deleteUploadInfoQuery->lastError().text();
            return;
        }
        qDebug() <<  _deleteUploadInfoQuery->executedQuery() << file;
        _deleteUploadInfoQuery->finish();
    }
}

bool SyncJournalDb::deleteStaleUploadInfos(const QSet<QString> &keep)
{
    QMutexLocker locker(&_mutex);

    if (!checkConnect()) {
        return false;
    }

    QSqlQuery query(_db);
    query.prepare("SELECT path FROM uploadinfo");

    if (!query.exec()) {
        QString err = query.lastError().text();
        qDebug() << "Error creating prepared statement: " << query.lastQuery() << ", Error:" << err;
        return false;
    }

    QStringList superfluousPaths;

    while (query.next()) {
        const QString file = query.value(0).toString();
        if (!keep.contains(file)) {
            superfluousPaths.append(file);
        }
    }

    return deleteBatch(*_deleteUploadInfoQuery, superfluousPaths, "uploadinfo");
}

SyncJournalBlacklistRecord SyncJournalDb::blacklistEntry( const QString& file )
{
    QMutexLocker locker(&_mutex);
    SyncJournalBlacklistRecord entry;

    if( file.isEmpty() ) return entry;

    // SELECT lastTryEtag, lastTryModtime, retrycount, errorstring

    if( checkConnect() ) {
        _blacklistQuery->bindValue( ":path", file );
        if( _blacklistQuery->exec() ){
            if( _blacklistQuery->next() ) {
                bool ok;
                entry._lastTryEtag    = _blacklistQuery->value(0).toByteArray();
                entry._lastTryModtime = _blacklistQuery->value(1).toLongLong(&ok);
                entry._retryCount     = _blacklistQuery->value(2).toInt();
                entry._errorString    = _blacklistQuery->value(3).toString();
                entry._file           = file;
            }
        } else {
            qWarning() << "Exec error blacklist: " << _blacklistQuery->lastQuery() <<  " : "
                       << _blacklistQuery->lastError().text();
        }
        _blacklistQuery->finish();
    }

    return entry;
}

bool SyncJournalDb::deleteStaleBlacklistEntries(const QSet<QString> &keep)
{
    QMutexLocker locker(&_mutex);

    if (!checkConnect()) {
        return false;
    }

    QSqlQuery query(_db);
    query.prepare("SELECT path FROM blacklist");

    if (!query.exec()) {
        QString err = query.lastError().text();
        qDebug() << "Error creating prepared statement: " << query.lastQuery() << ", Error:" << err;
        return false;
    }

    QStringList superfluousPaths;

    while (query.next()) {
        const QString file = query.value(0).toString();
        if (!keep.contains(file)) {
            superfluousPaths.append(file);
        }
    }

    QSqlQuery delQuery(_db);
    delQuery.prepare("DELETE FROM blacklist WHERE path = ?");
    return deleteBatch(delQuery, superfluousPaths, "blacklist");
}

int SyncJournalDb::blackListEntryCount()
{
    int re = 0;

    QMutexLocker locker(&_mutex);
    if( checkConnect() ) {
        QSqlQuery query(_db);
        if( ! query.exec("SELECT count(*) FROM blacklist") ) {
            sqlFail("Count number of blacklist entries failed", query);
        }
        if( query.next() ) {
            re = query.value(0).toInt();
        }
    }
    return re;
}

int SyncJournalDb::wipeBlacklist()
{
    QMutexLocker locker(&_mutex);
    if( checkConnect() ) {
        QSqlQuery query(_db);

        query.prepare("DELETE FROM blacklist");

        if( ! query.exec() ) {
            sqlFail("Deletion of whole blacklist failed", query);
            return -1;
        }
        return query.numRowsAffected();
    }
    return -1;
}

void SyncJournalDb::wipeBlacklistEntry( const QString& file )
{
    QMutexLocker locker(&_mutex);
    if( checkConnect() ) {
        QSqlQuery query(_db);

        query.prepare("DELETE FROM blacklist WHERE path=:path");
        query.bindValue(":path", file);
        if( ! query.exec() ) {
            sqlFail("Deletion of blacklist item failed.", query);
        }
    }
}

void SyncJournalDb::updateBlacklistEntry( const SyncJournalBlacklistRecord& item )
{
    QMutexLocker locker(&_mutex);
    QSqlQuery query(_db);

    if( !checkConnect() ) {
        return;
    }

    QString sql("SELECT retrycount FROM blacklist WHERE path=:path");

    if( Utility::fsCasePreserving() ) {
        // if the file system is case preserving we have to check the blacklist
        // case insensitively
        sql += QLatin1String(" COLLATE NOCASE");
    }

    query.prepare(sql);
    query.bindValue(":path", item._file);

    if( !query.exec() ) {
        qDebug() << "SQL exec blacklistitem failed:" << query.lastError().text();
        return;
    }

    QSqlQuery iQuery(_db);
    if( query.next() ) {
        int retries = query.value(0).toInt();
        retries--;
        if( retries < 0 ) retries = 0;

        iQuery.prepare( "UPDATE blacklist SET lastTryEtag = :etag, lastTryModtime = :modtime, "
                        "retrycount = :retries, errorstring = :errStr WHERE path=:path");
        iQuery.bindValue(":etag", item._lastTryEtag);
        iQuery.bindValue(":modtime", QString::number(item._lastTryModtime));
        iQuery.bindValue(":retries", retries);
        iQuery.bindValue(":errStr", item._errorString);
        iQuery.bindValue(":path", item._file);
    } else {
        // there is no entry yet.
        iQuery.prepare("INSERT INTO blacklist (path, lastTryEtag, lastTryModtime, retrycount, errorstring) "
                         "VALUES (:path, :lastEtag, :lastMTime, :retrycount, :errorstring);");

        iQuery.bindValue(":path", item._file );
        iQuery.bindValue(":lastEtag", item._lastTryEtag);
        iQuery.bindValue(":lastMTime", QString::number(item._lastTryModtime));
        iQuery.bindValue(":retrycount", item._retryCount);
        iQuery.bindValue(":errorstring", item._errorString);
    }
    if( !iQuery.exec() ) {
        qDebug() << "SQL exec blacklistitem insert/update failed: "<< iQuery.lastError().text();
    }

}

void SyncJournalDb::avoidRenamesOnNextSync(const QString& path)
{
    QMutexLocker locker(&_mutex);

    if( !checkConnect() ) {
        return;
    }

    QSqlQuery query(_db);
    query.prepare("UPDATE metadata SET fileid = '', inode = '0' WHERE path == ? OR path LIKE(?||'/%')");
    query.bindValue(0, path);
    query.bindValue(1, path);
    if( !query.exec() ) {
        qDebug() << Q_FUNC_INFO << "SQL error in avoidRenamesOnNextSync: "<< query.lastError().text();
    } else {
        qDebug() << Q_FUNC_INFO << query.executedQuery()  << path << "(" << query.numRowsAffected() << " rows)";
    }

    // We also need to remove the ETags so the update phase refreshes the directory paths
    // on the next sync
    locker.unlock();
    avoidReadFromDbOnNextSync(path);
}

void SyncJournalDb::avoidReadFromDbOnNextSync(const QString& fileName)
{
    //Make sure that on the next sync, filName is not read from the DB but use the PROPFIND to
    //get the info from the server
    // We achieve that by clearing the etag of the parents directory recursively

    QMutexLocker locker(&_mutex);

    if( !checkConnect() ) {
        return;
    }

    QSqlQuery query(_db);
    // This query will match entries for whitch the path is a prefix of fileName
    query.prepare("UPDATE metadata SET md5='_invalid_' WHERE ? LIKE(path||'/%') AND type == 2"); // CSYNC_FTW_TYPE_DIR == 2
    query.bindValue(0, fileName);
    if( !query.exec() ) {
        qDebug() << Q_FUNC_INFO << "SQL error in avoidRenamesOnNextSync: "<< query.lastError().text();
    } else {
        qDebug() << Q_FUNC_INFO << query.executedQuery()  << fileName << "(" << query.numRowsAffected() << " rows)";
    }

    // Prevent future overwrite of the etag for this sync
    _avoidReadFromDbOnNextSyncFilter.append(fileName);
}

void SyncJournalDb::commit(const QString& context, bool startTrans)
{
    QMutexLocker lock(&_mutex);
    commitInternal(context, startTrans);
}


void SyncJournalDb::commitInternal(const QString& context, bool startTrans )
{
    qDebug() << "Transaction Start " << context;
    commitTransaction();

    if( startTrans ) {
        startTransaction();
    }
}

SyncJournalDb::~SyncJournalDb()
{
    close();
}

bool SyncJournalDb::isConnected()
{
    QMutexLocker lock(&_mutex);
    return checkConnect();
}

bool SyncJournalDb::isUpdateFrom_1_5()
{
    QMutexLocker lock(&_mutex);
    checkConnect();
    return _possibleUpgradeFromMirall_1_5;
}



} // namespace Mirall
