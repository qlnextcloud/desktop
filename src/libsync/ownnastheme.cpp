/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
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

#include "ownnastheme.h"

#include <QString>
#include <QVariant>
#ifndef TOKEN_AUTH_ONLY
#include <QPixmap>
#include <QIcon>
#include <QStyle>
#include <QApplication>
#endif
#include <QCoreApplication>

#include "config.h"
#include "common/utility.h"
#include "version.h"

namespace OCC {

ownNasTheme::ownNasTheme()
    : Theme()
{
}

QString ownNasTheme::configFileName() const
{
    return QLatin1String("ownnas.cfg");
}

QString ownNasTheme::about() const
{
    QString devString;
    /*
    devString = trUtf8("<p>Version %2. For more information visit <a href=\"%3\">https://%4</a></p>"
                       "<p>For known issues and help, please visit: <a href=\"https://central.owncloud.org/c/desktop-client\">https://central.owncloud.org</a></p>"
                       "<p><small>By Klaas Freitag, Daniel Molkentin, Olivier Goffart, Markus Götz, "
                       " Jan-Christoph Borchardt, and others.</small></p>"
                       "<p>Copyright ownCloud GmbH</p>"
                       "<p>Licensed under the GNU General Public License (GPL) Version 2.0<br/>"
                       "ownCloud and the ownCloud Logo are registered trademarks of ownCloud GmbH "
                       "in the United States, other countries, or both.</p>")
                    .arg(Utility::escape(MIRALL_VERSION_STRING),
                        Utility::escape("https://" MIRALL_STRINGIFY(APPLICATION_DOMAIN)),
                        Utility::escape(MIRALL_STRINGIFY(APPLICATION_DOMAIN)));

    devString += gitSHA1();
    */
    devString = tr("<p>Version %1.</p>").arg(MIRALL_VERSION_STRING);
    devString += tr("<p>©2018 TB Co.,Ltd.</p>");

    return devString;
}

#ifndef TOKEN_AUTH_ONLY
QIcon ownNasTheme::trayFolderIcon(const QString &) const
{
    QPixmap fallback = qApp->style()->standardPixmap(QStyle::SP_FileDialogNewFolder);
    return QIcon::fromTheme("folder", fallback);
}

QIcon ownNasTheme::applicationIcon() const
{
    return themeIcon(QLatin1String("Ownnas-icon"));
}


QVariant ownNasTheme::customMedia(Theme::CustomMediaType type)
{
    if (type == Theme::oCSetupTop) {
        // return QCoreApplication::translate("ownNasTheme",
        //                                   "If you don't have an ownCloud server yet, "
        //                                   "see <a href=\"https://owncloud.com\">owncloud.com</a> for more info.",
        //                                   "Top text in setup wizard. Keep short!");
        return QVariant();
    } else {
        return QVariant();
    }
}

#endif

QString ownNasTheme::helpUrl() const
{
    return QString::fromLatin1("https://doc.ownnas.org/desktop/%1.%2/").arg(MIRALL_VERSION_MAJOR).arg(MIRALL_VERSION_MINOR);
}

#ifndef TOKEN_AUTH_ONLY
QColor ownNasTheme::wizardHeaderBackgroundColor() const
{
    return QColor("#1d2d42");
}

QColor ownNasTheme::wizardHeaderTitleColor() const
{
    return QColor("#ffffff");
}

QPixmap ownNasTheme::wizardHeaderLogo() const
{
    return QPixmap(hidpiFileName(":/client/theme/colored/wizard_logo.png"));
}

#endif
/*
QString ownNasTheme::appName() const
{
    return QLatin1String("Ownnas");
}

QString ownNasTheme::appNameGUI() const
{
    return QLatin1String("Ownnas");
}
 */

}
