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

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#ifdef __cplusplus

#include <QtCore>
#include <QtGui>
#include <QtNetwork>
#include <QtSql>
#include <QtXml>
#include <QtScript>

#ifdef QT_NO_DEBUG
#define NO_DBGINFO      1
#else
#define NO_DBGINFO      0
#endif

// Uncomment these to disable webview even in debug
// #undef NO_DBGINFO
// #define NO_DBGINFO 1

#define SKYPE_CLIENT_NAME "QGVDial"

#define UA_N900   "Mozilla/5.0 (X11; U; Linux armv7l; en-GB; rv:1.9.2a1pre) Gecko/20090928 Firefox/3.5 Maemo Browser 1.4.1.21 RX-51 N900"
#define UA_IPHONE "Mozilla/5.0 (iPhone; U; CPU iPhone OS 3_0 like Mac OS X; en-us) AppleWebKit/528.18 (KHTML, like Gecko) Version/4.0 Mobile/7A341 Safari/528.16"

#define GV_URL          "http://www.google.com/voice"
#define GV_HTTPS        "https://www.google.com/voice"
#define GV_HTTPS_M      "https://www.google.com/voice/m"

#define GV_CLIENTLOGIN "https://www.google.com/accounts/ClientLogin"

struct GVRegisteredNumber
{
    QString     strName;
    QString     strDescription;
    char        chType;

    GVRegisteredNumber () {chType = 0;}
};
typedef QVector<GVRegisteredNumber> GVRegisteredNumberArray;

enum GVI_Entry_Type {
    GVIE_Unknown = 0,
    GVIE_Placed,
    GVIE_Received,
    GVIE_Missed,
    GVIE_Voicemail,
    GVIE_TextMessage,
};

struct GVInboxEntry
{
    QString         id;
    GVI_Entry_Type  Type;
    QDateTime       startTime;

    QString         strDisplayNumber;
    QString         strPhoneNumber;
    QString         strText;

    QString         strNote;

    bool            bRead;
    bool            bSpam;
    bool            bTrash;
    bool            bStar;

    GVInboxEntry () {
        init ();
    }

    void init () {
        id.clear ();
        Type = GVIE_Unknown;
        startTime = QDateTime();
        strDisplayNumber.clear ();
        strPhoneNumber.clear ();
        strText.clear ();
        strNote.clear ();

        bRead = bSpam = bTrash = bStar = false;
    }
};
Q_DECLARE_METATYPE (GVInboxEntry)

template <class T> class VConv
{
public:
    static T* toPtr(QVariant v)
    {
        return  (T *) v.value<void *>();
    }

    static QVariant toQVariant(T* ptr)
    {
        return qVariantFromValue((void *) ptr);
    }
};

typedef QPair<QString,QString> QStringPair;
typedef QList<QStringPair> QStringPairList;

///////////////////////// Google contacts API related //////////////////////////
enum PhoneType {
    PType_Unknown = 0,
    PType_Mobile,
    PType_Home,
    PType_Other,
};

struct PhoneInfo
{
    PhoneType   Type;
    QString     strNumber;

    PhoneInfo() { init(); }

    void init () {
        Type = PType_Unknown;
        strNumber.clear ();
    }

    static PhoneType charToType(const char ch) {
        switch (ch) {
        case 'M':
            return PType_Mobile;
        case 'H':
            return PType_Home;
        case 'O':
            return PType_Other;
        default:
            return PType_Unknown;
        }
    }
    static char typeToChar (PhoneType type) {
        switch(type) {
        case PType_Mobile:
            return 'M';
        case PType_Home:
            return 'H';
        case PType_Other:
            return 'O';
        default:
            return '?';
        }
    }
    static const char * typeToString (PhoneType type) {
        switch(type) {
        case PType_Mobile:
            return "Mobile";
        case PType_Home:
            return "Home";
        case PType_Other:
            return "Other";
        default:
            return "Unknown";
        }
    }
};
typedef QVector<PhoneInfo> PhoneInfoArray;

struct ContactInfo
{
    //! Unique ID for this contact
    QString         strId;
    //! Contact title a.k.a. the contact name
    QString         strTitle;

    //! Array of phones for this contact
    PhoneInfoArray  arrPhones;
    // Which number is selected
    int             selected;

    //! The notes that the user may have added to the contact
    QString         strNotes;

    //! Link to this contact's photo
    QString         hrefPhoto;

    //! Is this contact deleted?
    bool            bDeleted;

    ContactInfo() { init(); }

    void init () {
        strId.clear ();
        strTitle.clear ();
        arrPhones.clear ();
        strNotes.clear ();
        hrefPhoto.clear ();
        bDeleted = false;
        selected = 0;
    }
};
Q_DECLARE_METATYPE(ContactInfo)
////////////////////////////////////////////////////////////////////////////////

#if defined(Q_WS_HILDON) && !defined(Q_WS_MAEMO_5)
#define DIABLO_OS 1
#else
#define DIABLO_OS 0
#endif

#if defined(Q_WS_X11) && !defined(Q_WS_MAEMO_5)
#define LINUX_DESKTOP 1
#else
#define LINUX_DESKTOP 0
#endif

#if LINUX_DESKTOP || defined(Q_WS_WIN)
#define DESKTOP_OS 1
#else
#define DESKTOP_OS 0
#endif

#if defined(Q_OS_SYMBIAN) || defined(Q_WS_MAEMO_5) || DIABLO_OS
#define MOBILE_OS 1
#else
#define MOBILE_OS 0
#endif

#if defined(Q_WS_X11)
#define TELEPATHY_CAPABLE 1
#else
#define TELEPATHY_CAPABLE 0
#endif

#if DESKTOP_OS || defined(Q_WS_MAEMO_5) || defined(Q_OS_SYMBIAN)
#define MOSQUITTO_CAPABLE 1
#else
#define MOSQUITTO_CAPABLE 0
#endif

#if DIABLO_OS
#define MOBILITY_PRESENT 0
#else
#define MOBILITY_PRESENT 1
#endif

#endif //__cplusplus
#endif //__GLOBAL_H__