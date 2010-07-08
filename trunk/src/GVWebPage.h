#ifndef __GVWEBPAGE_H__
#define __GVWEBPAGE_H__

#include "global.h"
#include "MobileWebPage.h"
#include "GVAccess.h"

#include <QtCore>
#include <QtWebKit>

class GVWebPage : public GVAccess
{
    Q_OBJECT

private:
    GVWebPage(QObject *parent = NULL);
    virtual ~GVWebPage(void);

public:
#if !NO_DBGINFO
    void setView (QWidget *view);
#endif

public slots:
    //! Invoked when the user presses cancel
    void userCancel ();

private slots:
    void callDone (QNetworkReply * reply);
    //! Invoked when the about:blank page has finished loading
    void aboutBlankDone (bool bOk);

    //! Invoked when the google voice login page is loaded
    void loginStage1 (bool bOk);
    //! Invoked when the google voice main page (after login) is loaded
    void loginStage2 (bool bOk);

    //! Invoked when the logout page has loaded
    void logoutDone (bool bOk);

    //! Repeatedly invoked when the next page in the contacts list is loaded
    void contactsLoaded (bool bOk);

    //! Invoked when the main page is loaded
    void callStage1 (bool bOk);
    //! Invoked when the actual call page is loaded
    void callStage2 (bool bOk);

    //! Invoked when the contact info link is loaded
    void contactInfoLoaded (bool bOk);

    //! Invoked when the registered phone list page is loaded
    void phonesListLoaded (bool bOk);

    //! Invoked when the select phone page is loaded
    void selectPhoneLoaded (bool bOk);
    //! Invokes when the phone is selected
    void onRegPhoneSelected (bool bOk);

    //! Invoked when the history page is loaded
    void historyPageLoaded (bool bOk);

    //! Invoked when the history entry link  for an unknown number is loaded
    void getContactFromHistoryLinkLoaded (bool bOk);

    //! Invoked when the garbage timer times out.
    void garbageTimerTimeout ();

    //! Invoked when the SMS main page is loaded
    void sendSMSPage1 (bool bOk);

    //! Invoked when the play vmail page is loaded
    void playVmailPageDone (bool bOk);

    //! Invoked when the page requests download of the vmail
    void vmailDataRecv (QNetworkReply *reply);

    //! Invoked when all the data in the vmail mp3 has finished downloading
    void vmailDataDone ();

private:
    QWebElement doc ();
    bool isLoggedIn ();
    bool isNextContactsPageAvailable ();
    void getHostAndQuery (QString &strHost, QString &strQuery);
    void loadUrlString (const QString &strUrl);
    bool isLoadFailed (bool bOk);

    //! Load the about:blank page
    bool aboutBlank ();
    //! Login to Google voice
    bool login ();
    //! Log out of Google voice
    bool logout ();
    //! Retrieve all contacts for the logged in user
    bool retrieveContacts ();
    //! Get the contact info for the link provided
    bool getContactInfoFromLink ();
    //! Make a phone call to an arbitrary number
    bool dialCallback ();
    //! Get registered phones from the settings page
    bool getRegisteredPhones ();
    //! Select the given registered phone
    bool selectRegisteredPhone ();
    //! Begin the process to get history
    bool getHistory ();
    //! Call a number given the history entry's link
    bool getContactFromHistoryLink ();
    //! This sends SMSes
    bool sendSMS ();
    //! Play a voicemail
    bool playVmail ();

    void cancelDialStage1 ();
    void cancelDialStage2 ();
    void cancelDialStage3 ();

private:
    //! The webkit page that does all our work
    MobileWebPage           webPage;

    //! The timer object that optimizes our memory usage
    QTimer                  garbageTimer;

    //! The starting history page
    int                     nFirstPage;

    //! The current page (contacts or history)
    int                     nCurrent;

    friend class SingletonFactory;
};

#endif //__GVWEBPAGE_H__
