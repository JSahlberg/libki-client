/*
* Copyright 2010 Kyle M Hall <kyle.m.hall@gmail.com>
*
* This file is part of Libki.
*
* Libki is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Libki is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Libki.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QApplication>
#include <QProcess>

#include "loginwindow.h"
#include "timerwindow.h"
#include "networkclient.h"

#include <stdlib.h>

//#ifdef Q_WS_WIN
//    #include "qt_windows.h"
//#endif


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

#ifdef Q_WS_WIN
    // If this is an MS Windows platform, use the keylocker programs to limit mischief.
    QProcess::startDetached("windows/on_startup.exe");
#endif

QString os_username;
#ifdef Q_WS_WIN
    os_username = getenv("USERNAME");
#endif
#ifdef Q_WS_X11
    os_username = getenv("USER");
#endif
    qDebug() << "OS Username: " << os_username;
    /* Apply the stylesheet */
    QFile qss("libki.qss");
    qss.open(QFile::ReadOnly);
    app.setStyleSheet(qss.readAll());
    qss.close();

    QCoreApplication::setOrganizationName("Libki");
    QCoreApplication::setOrganizationDomain("libki.org");
    QCoreApplication::setApplicationName("Libki Kiosk Management System");
    QSettings::setDefaultFormat(QSettings::IniFormat);

    QSettings settings;

    QString onlyRunFor;
    onlyRunFor = settings.value("node/onlyRunFor").toString();
    qDebug() << "onlyRunFor: " << onlyRunFor;
    if ( !onlyRunFor.isEmpty() ) {
        if ( onlyRunFor != os_username ) {
            qDebug() << "onlyRunFor does not match OS username, exiting.";
            exit(1);
        }
    }

    QString onlyStopFor;
    onlyStopFor = settings.value("node/onlyStopFor").toString();
    qDebug() << "onlyStopFor: " << onlyStopFor;
    if ( !onlyStopFor.isEmpty() ) {
        if ( onlyStopFor == os_username ) {
            qDebug() << "onlyStopFor matches OS username, exiting.";
            exit(1);
        }
    }

    settings.setValue("session/ClientBehavior", "");
    settings.setValue("session/ReservationShowUsername", "");
    settings.sync();

    LoginWindow* loginWindow = new LoginWindow();
    TimerWindow* timerWindow = new TimerWindow();
    NetworkClient* networkClient = new NetworkClient();

    QObject::connect(
                loginWindow,
                SIGNAL( loginSucceeded( const QString& , const QString& , int  ) ),
                timerWindow,
                SLOT( startTimer( const QString& , const QString& , int  ) )
                );

    QObject::connect( timerWindow, SIGNAL(requestLogout()), networkClient, SLOT(attemptLogout()));
    QObject::connect( networkClient, SIGNAL(logoutSucceeded()), timerWindow, SLOT(stopTimer()));

    QObject::connect( timerWindow, SIGNAL( timerStopped() ), loginWindow, SLOT( displayLoginWindow() ) );

    QObject::connect(
                loginWindow,
                SIGNAL( attemptLogin( const QString&, const QString& ) ),
                networkClient,
                SLOT( attemptLogin( const QString&, const QString& ) )
                );

    QObject::connect(
                networkClient,
                SIGNAL(loginSucceeded(QString,QString,int)),
                loginWindow,
                SLOT(attemptLoginSuccess(QString,QString,int))
                );

    QObject::connect(
                networkClient,
                SIGNAL(loginFailed(QString)),
                loginWindow,
                SLOT(attemptLoginFailure(QString))
                );

    QObject::connect(
                networkClient,
                SIGNAL(setReservationStatus(QString)),
                loginWindow,
                SLOT(handleReservationStatus(QString))
                );

    QObject::connect(
                loginWindow,
                SIGNAL(displayingReservationMessage(QString)),
                networkClient,
                SLOT(acknowledgeReservation(QString))
                );

    QObject::connect( networkClient, SIGNAL(timeUpdatedFromServer(int)), timerWindow, SLOT(updateTimeLeft(int)));

    QObject::connect( networkClient, SIGNAL(messageRecieved(QString)), timerWindow, SLOT(showMessage(QString)));

    QObject::connect( networkClient, SIGNAL(allowClose(bool)), loginWindow, SLOT(setAllowClose(bool)));
    QObject::connect( networkClient, SIGNAL(allowClose(bool)), timerWindow, SLOT(setAllowClose(bool)));

    loginWindow->show();

    return app.exec();
}
