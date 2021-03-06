/*
 * Copyright (C) by Roeland Jago Douma <roeland@famdouma.nl>
 * Copyright (C) 2015 by Klaas Freitag <freitag@owncloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#ifndef SHARELINKWIDGET_H
#define SHARELINKWIDGET_H

#include "accountfwd.h"
#include "sharepermissions.h"
#include "QProgressIndicator.h"
#include <QDialog>
#include <QSharedPointer>
#include <QList>

class QMenu;
class QTableWidgetItem;

namespace OCC {

namespace Ui {
    class ShareLinkWidget;
}

class AbstractCredentials;
class QuotaInfo;
class SyncResult;
class LinkShare;
class Share;
class ShareManager;

/**
 * @brief The ShareDialog class
 * @ingroup gui
 */
class ShareLinkWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ShareLinkWidget(AccountPtr account,
        const QString &sharePath,
        const QString &localPath,
        SharePermissions maxSharingPermissions,
        QWidget *parent = 0);
    ~ShareLinkWidget();
    void getShares();
    void toggleButton(bool show);

private slots:
    void slotSharesFetched(const QList<QSharedPointer<Share>> &shares);
    //void slotShareSelectionChanged();

    void slotCreateorDeleteShareLink(bool checked);
    void slotCreatePassword();

    void slotExpireDateChanged(const QDate &date);

    void slotContextMenuButtonClicked();
    void slotLinkContextMenuActionTriggered(QAction *action);

    void slotDeleteShareFetched();
    void slotCreateShareFetched();
    void slotCreateShareRequiresPassword(const QString &message);
    void slotPasswordSet();
    //void slotExpireSet();

    void slotServerError(int code, const QString &message);
    void slotPasswordSetError(int code, const QString &message);

private:
    void displayError(const QString &errMsg);

    void togglePasswordOptions(bool enable);
    void setPassword(const QString &password);

    void toggleExpireDateOptions(bool enable);
    void setExpireDate(const QDate &date);

    void copyShareLink(const QUrl &url);

    /** Confirm with the user and then delete the share */
    void confirmAndDeleteShare();

    /** Retrieve a share's name, accounting for _namesSupported */
    QString shareName() const;

    /**
     * Retrieve the selected share, returning 0 if none.
     */
    //QSharedPointer<LinkShare> selectedShare() const;

    Ui::ShareLinkWidget *_ui;
    AccountPtr _account;
    QString _sharePath;
    QString _localPath;
    QString _shareUrl;

    QProgressIndicator *_pi_create;
    QProgressIndicator *_pi_password;
    QProgressIndicator *_pi_date;
    QProgressIndicator *_pi_editing;

    ShareManager *_manager;
    QSharedPointer<LinkShare> _linkShare;

    bool _isFile;
    bool _passwordRequired;
    bool _expiryRequired;
    bool _namesSupported;

    QMenu *_linkContextMenu;
    QAction *_copyLinkAction;
    QAction *_readOnlyLinkAction;
    QAction *_allowEditingLinkAction;
    QAction *_allowUploadEditingLinkAction;
    QAction *_allowUploadLinkAction;
    QAction *_passwordProtectLinkAction;
    QAction *_expirationDateLinkAction;
    QAction *_unshareLinkAction;
};
}

#endif // SHARELINKWIDGET_H
