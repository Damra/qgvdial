/*
qgvdial is a cross platform Google Voice Dialer
Copyright (C) 2010  Yuvraaj Kelkar

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

Contact: yuvraaj@gmail.com
*/

#ifndef CONTACTSPARSEROBJECT_H
#define CONTACTSPARSEROBJECT_H

#include "global.h"

// For some reason the symbian MOC doesn't like it if I don't include QObject
// even though it is present in QtCore which is included in global.h
#include <QObject>

class ContactsParserObject : public QObject
{
    Q_OBJECT

public:
    ContactsParserObject(QByteArray data,
                         const QString strAuth,
                         QObject *parent = 0);
    void setEmitLog (bool enable = true);
    ~ContactsParserObject();

signals:
    //! Status emitter for status bar
    void status(const QString &strText, int timeout = 2000);
    // Emitted when one contact is parsed out of the XML
    void gotOneContact (const ContactInfo &contactInfo);
    // Emitted when work is done
    void done(bool rv);

public slots:
    void doWork ();

private slots:
    void onGotOneContact (const ContactInfo &contactInfo);
    void onGotOnePhoto (const ContactInfo &contactInfo);

private:
    QNetworkRequest createRequest(QString strUrl);
    void decRef (bool rv = true);

private:
    QByteArray  byData;

    bool        bEmitLog;

    QNetworkAccessManager *nwMgr;
    QString               strGoogleAuth;

    QAtomicInt  refCount;
};

class PhotoReplyTracker : public QObject
{
    Q_OBJECT

public:
    PhotoReplyTracker(const ContactInfo &ci,
                            QNetworkReply *r,
                            QObject *parent = NULL);

signals:
    // Emitted when the photo is retrieved from the contact
    void gotOneContact (const ContactInfo &contactInfo);

public slots:
    void onFinished();

private slots:
    void onResponseTimeout();

private:
    QNetworkReply  *reply;
    ContactInfo     contactInfo;
    QTimer          responseTimeout;

    bool            aborted;
};

#endif // CONTACTSPARSEROBJECT_H