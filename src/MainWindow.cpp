#include "MainWindow.h"
#include "PhoneNumberValidator.h"

#include <QDesktopServices>

#include <iostream>
using namespace std;

MainWindow::MainWindow (QWidget *parent)
: QDeclarativeView (parent)
, fLogfile (this)
, icoGoogle (":/Google.png")
, pSystray (NULL)
, oContacts (this)
, oInbox (this)
, vmailPlayer (this)
, statusTimer (this)
#ifdef Q_WS_MAEMO_5
, infoBox (this)
#endif
, menuFile ("&File", this)
, actLogin ("Login...", this)
, actDismiss ("Dismiss", this)
, actRefresh ("Refresh", this)
, actExit ("Exit", this)
, bLoggedIn (false)
, modelRegNumber (this)
, indRegPhone (0)
, mtxDial (QMutex::Recursive)
, bCallInProgress (false)
, bDialCancelled (false)
#if MOSQUITTO_CAPABLE
, mqThread (QString("qgvdial:%1").arg(QHostInfo::localHostName())
            .toLatin1().constData (), this)
#endif
{
    initLogging ();

    qRegisterMetaType<ContactInfo>("ContactInfo");

    OsDependent &osd = Singletons::getRef().getOSD ();
    osd.setDefaultWindowAttributes (this);

    initQML ();

    // A systray icon if the OS supports it
    if (QSystemTrayIcon::isSystemTrayAvailable ())
    {
        pSystray = new QSystemTrayIcon (this);
        pSystray->setIcon (icoGoogle);
        pSystray->setToolTip ("Google Voice dialer");
        pSystray->setContextMenu (&menuFile);
        QObject::connect (
            pSystray,
            SIGNAL (activated (QSystemTrayIcon::ActivationReason)),
            this,
            SLOT (systray_activated (QSystemTrayIcon::ActivationReason)));
        pSystray->show ();
    }

    QObject::connect (qApp, SIGNAL (messageReceived (const QString &)),
                      this, SLOT   (messageReceived (const QString &)));

    QObject::connect (&statusTimer, SIGNAL (timeout()),
                       this       , SLOT   (onStatusTimerTick ()));

    // Schedule the init a bit later so that the app.exec() can begin executing
    QTimer::singleShot (100, this, SLOT (init()));
}//MainWindow::MainWindow

MainWindow::~MainWindow ()
{
#if MOSQUITTO_CAPABLE
    mqThread.terminate ();
#endif
}//MainWindow::~MainWindow

void
MainWindow::initLogging ()
{
    // Initialize logging
    OsDependent &osd = Singletons::getRef().getOSD ();
    QString strLogfile = osd.getStoreDirectory ();
    strLogfile += QDir::separator ();
    strLogfile += "qgvdial.log";
    fLogfile.setFileName (strLogfile);
    fLogfile.open (QIODevice::WriteOnly | QIODevice::Append);
}//MainWindow::initLogging

/** Log information to console and to log file
 * This function is invoked from the qDebug handler that is installed in main.
 * @param strText Text to be logged
 * @param level Log level
 */
void
MainWindow::log (const QString &strText, int level /*= 10*/)
{
    QString strDisp;
    QRegExp regex("^\"(.*)\"\\s*");
    if (strText.indexOf (regex) != -1) {
        strDisp = regex.cap (1);
    } else {
        strDisp = strText;
    }

    QDateTime dt = QDateTime::currentDateTime ();
    QString strLog = QString("%1 : %2 : %3")
                     .arg(dt.toString ("yyyy-MM-dd hh:mm:ss.zzz"))
                     .arg(level)
                     .arg(strDisp);


    // Send to standard output
    cout << strLog.toStdString () << endl;

    // Send to log file
    if (fLogfile.isOpen ()) {
        QTextStream streamLog(&fLogfile);
        streamLog << strLog << endl;
    }

    // Append it to the circular buffer
    arrLogMsgs.prepend (strLog);
    if (arrLogMsgs.size () > 50) {
        arrLogMsgs.removeLast ();
    }
    QDeclarativeContext *ctx = this->rootContext();
    if (NULL != ctx) {
        ctx->setContextProperty ("g_logModel", QVariant::fromValue(arrLogMsgs));
    }
}//MainWindow::log

/** Status update function
 * Use this function to update the status. The status is shown dependent on the
 * platform. On Windows and Linux, this status is shown on the system tray as a
 * notification message from our systray icon. On Maemo, it is shown as the
 * notification banner.
 * @param strText Text to show as the status
 * @param timeout Timeout in milliseconds. 0 indicates a status that remains
 *          until the next status is to be displayed.
 */
void
MainWindow::setStatus(const QString &strText, int timeout /* = 3000*/)
{
    qDebug () << strText;

#ifdef Q_WS_MAEMO_5
    infoBox.hide ();

    // Show the banner only if the window is invisible. Otherwise the QML
    // status bar is more than enough for this job.
    if (!this->isVisible ()) {
        QLabel *theLabel = (QLabel *) infoBox.widget ();
        if (NULL == theLabel) {
            theLabel = new QLabel (strText, &infoBox);
            theLabel->setAlignment (Qt::AlignHCenter);
            infoBox.setWidget (theLabel);
            qDebug("Created the Maemo5 yellow banner label");
        } else {
            qDebug() << "Display the status banner:" << strText;
            theLabel->setText (strText);
        }
        infoBox.setTimeout (0 == timeout ? 3000 : timeout);
        infoBox.show ();
    }
#else
    if (NULL != pSystray) {
        pSystray->showMessage ("Status", strText,
                               QSystemTrayIcon::Information,
                               timeout);
    }
#endif

    statusTimer.stop ();
    QDeclarativeContext *ctx = this->rootContext();
    ctx->setContextProperty ("g_strStatus", strText);

    if (0 != timeout) {
        statusTimer.setSingleShot (true);
        statusTimer.setInterval (timeout);
        statusTimer.start ();
    }
}//MainWindow::setStatus

void
MainWindow::onStatusTimerTick ()
{
    QDeclarativeContext *ctx = this->rootContext();
    ctx->setContextProperty ("g_strStatus", "Ready");
}//MainWindow::onStatusTimerTick

/** Invoked when the QtSingleApplication sends a message
 * We have used a QtSingleApplication to ensure that there is only one instance
 * of our program running in a specific user context. When the user attempts to
 * fire up another instance of our application, the second instance communicates
 * with the first and tells it to show the main window. the 2nd instance then
 * self-terminates. The first instance gets the "show" command as a parameter to
 * this SLOT.
 * It is also possible for the second instance to send a "quit" message. This is
 * almost always done to quit before uninstall/upgrade.
 * @param message The message passed by the other application.
 */
void
MainWindow::messageReceived (const QString &message)
{
    if (message == "show") {
        qDebug ("Second instance asked us to show");
        this->show ();
    } else if (message == "quit") {
        qDebug ("Second instance asked us to quit");
        this->on_actionE_xit_triggered ();
    }
}//MainWindow::messageReceived

/** Invoked when the user clicks the hide button or long presses the top bar
 * This function is supposed to hide the qgvdial main window. On all platforms
 * except Symbian a simple hide() is sufficient. In Symbian, we need to do it
 * differently.
 */
void
MainWindow::onSigHide ()
{
#if defined(Q_OS_SYMBIAN)
    // Symbian qgvdial needs lowering, not hiding.
    this->lower ();
#else
    this->hide ();
#endif
}//MainWindow::onSigHide

/** Deferred initialization function
 * This function does all of the initialization that was originally in the
 * constructor. It was moved out of the constructor because it takes a long time
 * to complete and because it was in the constructor, the GUI would not be shown
 * until the init was done. Since the GUI does not need full init, I shifted it
 * to this function and invoke this function function in a delay timer (100ms).
 * That way the constructor returns quickly, the app begins processing events,
 * the GUI is displayed and all is ready to show before init begins. Then as the
 * init progresses, the init functions can output status messages documenting
 * what the app is currently doing. User and programmer both happy!
 */
void
MainWindow::init ()
{
    GVAccess &webPage = Singletons::getRef().getGVAccess ();
    CacheDatabase &dbMain = Singletons::getRef().getDBMain ();
    OsDependent &osd = Singletons::getRef().getOSD ();

    setStatus ("Initializing...");

    // Initialize the database: This may create OR blowup and then re-create
    // the database.
    dbMain.init ();

    // Pick up proxy settings from the DB and apply to webpage.
    bool bProxyEnable = false, bUseSystemProxy = false;
    bool bProxyAuthRequired = false;
    QString strProxyHost, strProxyUser, strProxyPass;
    int proxy_port = 0;
    dbMain.getProxySettings (bProxyEnable, bUseSystemProxy,
                             strProxyHost, proxy_port,
                             bProxyAuthRequired, strProxyUser, strProxyPass);
    onSigProxyChanges (bProxyEnable, bUseSystemProxy, strProxyHost, proxy_port,
                       bProxyAuthRequired, strProxyUser, strProxyPass);

    // Initialize the DBUS interface to allow other applications (and qgv-tp) to
    // initiate calls and send texts through us.
    osd.initDialServer (this, SLOT (dialNow (const QString &)));
    osd.initTextServer (
        this, SLOT (sendSMS (const QStringList &, const QString &)),
        this, SLOT (onSendTextWithoutData (const QStringList &)));

    // The GV access class signals these during the dialling protocol
    QObject::connect (&webPage    , SIGNAL (dialInProgress (const QString &)),
                       this       , SLOT   (dialInProgress (const QString &)));
    QObject::connect ( this       , SIGNAL (dialCanFinish ()),
                      &webPage    , SLOT   (dialCanFinish ()));
    QObject::connect (
        &webPage, SIGNAL (dialAccessNumber (const QString &,
                                            const QVariant &)),
         this   , SLOT   (dialAccessNumber (const QString &,
                                            const QVariant &)));

    // Skype client factory needs a main widget. Also, it needs a status sink.
    SkypeClientFactory &skypeFactory = Singletons::getRef().getSkypeFactory ();
    skypeFactory.setMainWidget (this);
    QObject::connect (
        &skypeFactory, SIGNAL (status(const QString &, int)),
         this        , SLOT   (setStatus(const QString &, int)));

    // Telepathy Observer factory init and status
    ObserverFactory &obF = Singletons::getRef().getObserverFactory ();
    obF.init ();
    QObject::connect (&obF , SIGNAL (status(const QString &, int)),
                       this, SLOT   (setStatus(const QString &, int)));

    // webPage init and status. This webpage is for debug purposes only
    QObject::connect (&webPage, SIGNAL (status(const QString &, int)),
                       this   , SLOT   (setStatus(const QString &, int)));

    // When the call initiators change, update us
    CallInitiatorFactory& cif = Singletons::getRef().getCIFactory ();
    QObject::connect (&cif, SIGNAL(changed()),
                      this, SLOT(onCallInitiatorsChange()));
    // Call initiator status
    QObject::connect (&cif , SIGNAL (status(const QString &, int)),
                       this, SLOT   (setStatus(const QString &, int)));

    // The dialog box that we use to input text signals us to send an SMS.
    // Connect it to the slot that does the job.
    QObject::connect (
        &dlgSMS, SIGNAL (sendSMS (const QStringList &, const QString &)),
         this  , SLOT   (sendSMS (const QStringList &, const QString &)));

    // Additional UI initializations:
    //@@UV: Need this for later
//    ui->edNumber->setValidator (new PhoneNumberValidator (ui->edNumber));

    // Login/logout = Ctrl+L
    actLogin.setShortcut (QKeySequence(Qt::CTRL + Qt::Key_L));
#ifdef Q_WS_MAEMO_5
    // Dismiss = Esc
    actDismiss.setShortcut (QKeySequence(Qt::CTRL + Qt::Key_W));
#else
    // Dismiss = Esc
    actDismiss.setShortcut (QKeySequence(Qt::Key_Escape));
#endif
    // Refresh = Ctrl+R
    actRefresh.setShortcut (QKeySequence(Qt::CTRL + Qt::Key_R));
    // Quit = Ctrl+Q
    actExit.setShortcut (QKeySequence(Qt::CTRL + Qt::Key_Q));
    // Add these actions to the window
    menuFile.addAction (&actLogin);
    menuFile.addAction (&actDismiss);
    menuFile.addAction (&actRefresh);
    menuFile.addAction (&actExit);
    this->addAction (&actLogin);
    this->addAction (&actDismiss);
    this->addAction (&actRefresh);
    this->addAction (&actExit);
    // When the actions are triggered, do the corresponding work.
    QObject::connect (&actLogin, SIGNAL (triggered()),
                       this    , SLOT   (on_action_Login_triggered()));
    QObject::connect (&actDismiss, SIGNAL (triggered()),
                       this      , SLOT   (hide ()));
    QObject::connect (&actRefresh, SIGNAL (triggered()),
                       this      , SLOT   (onRefresh()));
    QObject::connect (&actExit, SIGNAL (triggered()),
                       this   , SLOT   (on_actionE_xit_triggered()));

    this->setWindowIcon (icoGoogle);

#if MOSQUITTO_CAPABLE
    // Connect the signals from the Mosquitto thread
    QObject::connect (&mqThread , SIGNAL(sigUpdateInbox()),
                      &oInbox   , SLOT  (refresh()));
    QObject::connect (&mqThread , SIGNAL(sigUpdateContacts()),
                      &oContacts, SLOT  (refreshContacts()));
    QObject::connect (&mqThread , SIGNAL(status(QString,int)),
                       this     , SLOT  (setStatus(QString,int)));
#endif

    QObject::connect (
        &vmailPlayer, SIGNAL(stateChanged(QMediaPlayer::State)),
         this       , SLOT(onVmailPlayerStateChanged(QMediaPlayer::State)));

    // If the cache has the username and password, begin login
    if (dbMain.getUserPass (strUser, strPass))
    {
        this->setUsername (strUser);
        this->setPassword (strPass);

        QVariantList l;
        logoutCompleted (true, l);
        // Login without popping up the "enter user/pass" dialog
        doLogin ();
    }
    else
    {
        // Show this status for 60 seconds (or until the next status)
        setStatus ("Please enter email and password", 60 * 1000);

        strUser.clear ();
        strPass.clear ();

        on_action_Login_triggered ();
    }
}//MainWindow::init

void
MainWindow::initQML ()
{
    OsDependent &osd = Singletons::getRef().getOSD ();
    QRect rect = osd.getStartingSize ();

    int webwidget_typeid =
    qmlRegisterType<WebWidget>("org.qgvdial.WebWidget", 1, 0, "MyWebWidget");
    qDebug() << "Init: Webwidget type ID =" << webwidget_typeid;

    bool bTempFalse = false;
    int iTempZero = 0;

    // Prepare the glabally accessible variants for QML.
    QDeclarativeContext *ctx = this->rootContext();
    ctx->setContextProperty ("g_MainWidth", rect.width ());
    ctx->setContextProperty ("g_MainHeight", rect.height ());
    ctx->setContextProperty ("g_bShowMsg", bTempFalse);
    ctx->setContextProperty ("g_registeredPhonesModel", &modelRegNumber);
    ctx->setContextProperty ("g_bIsLoggedIn", bTempFalse);
    ctx->setContextProperty ("g_bShowSettings", bTempFalse);
    ctx->setContextProperty ("g_strStatus", "Getting Ready");
    ctx->setContextProperty ("g_strMsgText", "No message");
    ctx->setContextProperty ("g_CurrentPhoneName", "Not loaded");
    ctx->setContextProperty ("g_vmailPlayerState", iTempZero);
    ctx->setContextProperty ("g_logModel", QVariant::fromValue(arrLogMsgs));

    // Initialize the QML view
    this->setSource (QUrl ("qrc:/Main.qml"));
    this->setResizeMode (QDeclarativeView::SizeRootObjectToView);

    this->setUsername ("example@gmail.com");
    this->setPassword ("hunter2 :p");

    // The root object changes when we reload the source. Pick it up again.
    QGraphicsObject *gObj = this->rootObject();

    // Connect all signals to slots in this class.
    QObject::connect (gObj, SIGNAL (sigCall (QString)),
                      this, SLOT   (dialNow (QString)));
    QObject::connect (gObj, SIGNAL (sigText (QString)),
                      this, SLOT   (onSigText (const QString &)));
    QObject::connect (gObj, SIGNAL (sigVoicemail (QString)),
                      this, SLOT   (retrieveVoicemail (const QString &)));
    QObject::connect (gObj, SIGNAL (sigVmailPlayback (int)),
                      this, SLOT   (onSigVmailPlayback (int)));
    QObject::connect (gObj, SIGNAL (sigSelChanged (int)),
                      this, SLOT   (onRegPhoneSelectionChange (int)));
    QObject::connect (gObj   , SIGNAL (sigInboxSelect (QString)),
                      &oInbox, SLOT   (onInboxSelected (const QString &)));
    QObject::connect (gObj, SIGNAL (sigUserChanged (const QString &)),
                      this, SLOT   (onUserTextChanged (const QString &)));
    QObject::connect (gObj, SIGNAL (sigPassChanged (const QString &)),
                      this, SLOT   (onPassTextChanged (const QString &)));
    QObject::connect (gObj, SIGNAL (sigLogin ()),
                      this, SLOT   (doLogin ()));
    QObject::connect (gObj, SIGNAL (sigLogout ()),
                      this, SLOT   (doLogout ()));
    QObject::connect (gObj, SIGNAL (sigRefresh ()),
                      this, SLOT   (onRefresh ()));
    QObject::connect (gObj, SIGNAL (sigRefreshAll ()),
                      this, SLOT   (onRefreshAll ()));
    QObject::connect (gObj, SIGNAL (sigHide ()),
                      this, SLOT   (onSigHide ()));
    QObject::connect (gObj, SIGNAL (sigQuit ()),
                      this, SLOT   (on_actionE_xit_triggered ()));
    QObject::connect (gObj, SIGNAL (sigLinkActivated (const QString &)),
                      this, SLOT   (onLinkActivated (const QString &)));
    QObject::connect (
        gObj, SIGNAL (sigProxyChanges(bool, bool, const QString &, int,
                                      bool, const QString &, const QString &)),
        this, SLOT (onSigProxyChanges(bool, bool, const QString &, int,
                                      bool, const QString &, const QString &)));
    QObject::connect (
        gObj, SIGNAL (sigMosquittoChanges(bool, const QString &, int,
                                          const QString &)),
        this, SLOT   (onSigMosquittoChanges(bool, const QString &, int,
                                            const QString &)));
    QObject::connect (gObj, SIGNAL (sigMsgBoxDone(bool)),
                      this, SLOT (onSigMsgBoxDone(bool)));

#if DESKTOP_OS
    this->setFixedSize (this->size ());
#endif
}//MainWindow::initQML

/** Invoked to begin the login process.
 * We already have the username and password, so just start the login to the GV
 * website. The async completion routine is loginCompleted.
 */
void
MainWindow::doLogin ()
{
    OsDependent &osd = Singletons::getRef().getOSD ();
    GVAccess &webPage = Singletons::getRef().getGVAccess ();
    QVariantList l;

    bool bOk = false;
    do { // Begin cleanup block (not a loop)
        webPage.setTimeout(60);

        l += strUser;
        l += strPass;

        setStatus ("Logging in...", 0);

        osd.setLongWork (this, true);

        // webPage.workCompleted -> this.loginCompleted
        if (!webPage.enqueueWork (GVAW_login, l, this,
                SLOT (loginCompleted (bool, const QVariantList &))))
        {
            qWarning ("Login returned immediately with failure!");
            osd.setLongWork (this, false);
            break;
        }

        bOk = true;
    } while (0); // End cleanup block (not a loop)

    if (!bOk)
    {
        webPage.setTimeout(20);
        // Cleanup if any
        strUser.clear ();
        strPass.clear ();

        l.clear ();
        logoutCompleted (true, l);
    }
}//MainWindow::doLogin

void
MainWindow::onUserTextChanged (const QString &strUsername)
{
    if (strUser != strUsername) {
        strUser = strUsername;
        this->setUsername (strUser);
    }
}//MainWindow::onUserPassTextChanged

void
MainWindow::onPassTextChanged (const QString &strPassword)
{
    if (strPass != strPassword) {
        strPass = strPassword;

        this->setPassword (strPass);
    }
}//MainWindow::onUserPassTextChanged

/** SLOT: Invoked when user triggers the login/logout action
 * If it is a login action, the Login dialog box is shown.
 */
void
MainWindow::on_action_Login_triggered ()
{
    if (!bLoggedIn) {
        QDeclarativeContext *ctx = this->rootContext();
        ctx->setContextProperty ("g_bShowSettings", true);
    } else {
        doLogout ();
    }
}//MainWindow::on_action_Login_triggered

void
MainWindow::loginCompleted (bool bOk, const QVariantList &varList)
{
    strSelfNumber.clear ();
    GVAccess &webPage = Singletons::getRef().getGVAccess ();
    webPage.setTimeout(20);

    if (!bOk)
    {
        QVariantList l;
        logoutCompleted (true, l);

        OsDependent &osd = Singletons::getRef().getOSD ();
        osd.setLongWork (this, false);

        setStatus ("User login failed", 30*1000);
        this->showMsgBox ("User login failed");
    }
    else
    {
        CacheDatabase &dbMain = Singletons::getRef().getDBMain ();
        setStatus ("User logged in");

        // Save the users GV number returned by the login completion
        strSelfNumber = varList[varList.size()-1].toString ();

        // Prepare then contacts
        initContacts ();
        // Prepare the inbox widget for usage
        initInbox ();

        // Allow access to buttons and widgets
        actLogin.setText ("Logout");
        bLoggedIn = true;

        // Save the user name and password that was used to login
        dbMain.putUserPass (strUser, strPass);

        this->setUsername (strUser);
        this->setPassword (strPass);
        QDeclarativeContext *ctx = this->rootContext();
        ctx->setContextProperty ("g_bIsLoggedIn", bLoggedIn);
        ctx->setContextProperty ("g_bShowSettings", false);

        // Fill up the combobox on the main page
        if ((!dbMain.getRegisteredNumbers (arrNumbers)) ||
            (0 == arrNumbers.size ()))
        {
            refreshRegisteredNumbers ();
        }
        else
        {
            onCallInitiatorsChange (false);
        }

        bool bMqEnabled;
        QString strMqHost, strMqTopic;
        int mqPort;
        if (dbMain.getMqSettings (bMqEnabled, strMqHost, mqPort, strMqTopic)) {
            onSigMosquittoChanges (bMqEnabled, strMqHost, mqPort, strMqTopic);
        }
    }
}//MainWindow::loginCompleted

void
MainWindow::doLogout ()
{
    GVAccess &webPage = Singletons::getRef().getGVAccess ();
    QVariantList l;
    webPage.enqueueWork (GVAW_logout, l, this,
                         SLOT (logoutCompleted (bool, const QVariantList &)));

    OsDependent &osd = Singletons::getRef().getOSD ();
    osd.setLongWork (this, true);

#if MOSQUITTO_CAPABLE
    mqThread.setQuit ();
#endif
}//MainWindow::doLogout

void
MainWindow::logoutCompleted (bool, const QVariantList &)
{
    // This clears out the table and the view as well
    deinitContacts ();
    deinitInbox ();

    arrNumbers.clear ();

    actLogin.setText ("Login...");

    bLoggedIn = false;

    QDeclarativeContext *ctx = this->rootContext();
    ctx->setContextProperty ("g_bIsLoggedIn", bLoggedIn);

    setStatus ("Logout complete");
    OsDependent &osd = Singletons::getRef().getOSD ();
    osd.setLongWork (this, false);
}//MainWindow::logoutCompleted

void
MainWindow::systray_activated (QSystemTrayIcon::ActivationReason reason)
{
    switch (reason)
    {
    case QSystemTrayIcon::Trigger:
        if (this->isVisible ()) {
            this->hide ();
        } else {
            this->show ();
        }
        break;

    default:
        break;
    }
}//MainWindow::systray_activated

void
MainWindow::on_actionE_xit_triggered ()
{
    this->close ();

    for (QMap<QString,QString>::iterator i  = mapVmail.begin ();
                                         i != mapVmail.end ();
                                         i++)
    {
        qDebug() << "Delete vmail cached at" << i.value ();
        QFile::remove (i.value ());
    }
    mapVmail.clear ();

#if MOSQUITTO_CAPABLE
    mqThread.setQuit ();
#endif
    qApp->quit ();
}//MainWindow::on_actionE_xit_triggered

void
MainWindow::getContactsDone (bool bOk)
{
    if (!bOk)
    {
        this->showMsgBox ("Contacts retrieval failed");
        setStatus ("Contacts retrieval failed");
    }
}//MainWindow::getContactsDone

void
MainWindow::initContacts ()
{
    // Status
    QObject::connect (&oContacts, SIGNAL (status   (const QString &, int)),
                       this     , SLOT   (setStatus(const QString &, int)));

    // oContacts.allContacts -> this.getContactsDone
    QObject::connect (&oContacts, SIGNAL (allContacts (bool)),
                      this      , SLOT   (getContactsDone (bool)));

    oContacts.setUserPass (strUser, strPass);
    oContacts.loginSuccess ();
    oContacts.initModel (this);
    oContacts.refreshContacts ();
}//MainWindow::initContacts

void
MainWindow::deinitContacts ()
{
    oContacts.deinitModel ();
    oContacts.loggedOut ();
}//MainWindow::deinitContacts

void
MainWindow::initInbox ()
{
    // Status
    QObject::connect (
            &oInbox, SIGNAL (status   (const QString &, int)),
            this   , SLOT   (setStatus(const QString &, int)));

    oInbox.loginSuccess ();
    oInbox.initModel (this);
    oInbox.refresh ();
}//MainWindow::initInbox

void
MainWindow::deinitInbox ()
{
    oInbox.deinitModel ();
    oInbox.loggedOut ();
}//MainWindow::deinitInbox

/** Convert a number and a key to more info into a structure with all the info.
 * @param strNumber The phone number
 * @param strNameLink The key to the associated information. This parameter is
 *          optional. If it is not present, then a dummy structure is created
 *          that has only the number as valid information.
 */
bool
MainWindow::getInfoFrom (const QString &strNumber,
                         const QString &strNameLink,
                         ContactInfo &info)
{
    info.init ();

    if (0 != strNameLink.size ())
    {
        CacheDatabase &dbMain = Singletons::getRef().getDBMain ();
        info.strId = strNameLink;

        if (!dbMain.getContactFromLink (info))
        {
            this->showMsgBox ("Failed to get contact information");
            return (false);
        }

        for (int i = 0; i < info.arrPhones.size (); i++)
        {
            QString lhs = strNumber;
            QString rhs = info.arrPhones[i].strNumber;

            GVAccess::simplify_number (lhs);
            GVAccess::simplify_number (rhs);
            if (lhs == rhs)
            {
                info.selected = i;
                break;
            }
        }
    }
    else
    {
        info.strTitle = strNumber;
        PhoneInfo num;
        num.Type = PType_Unknown;
        num.strNumber = strNumber;
        info.arrPhones += num;
        info.selected = 0;
    }

    return (true);
}//MainWindow::getInfoFrom

bool
MainWindow::findInfo (const QString &strNumber, ContactInfo &info)
{
    bool rv = true;
    info.init ();

    QString strTrunc = strNumber;
    GVAccess::simplify_number (strTrunc, false);
    strTrunc.remove(' ').remove('+');

    CacheDatabase &dbMain = Singletons::getRef().getDBMain ();
    if (!dbMain.getContactFromNumber (strTrunc, info)) {
        qDebug ("Could not find info about this number. Using dummy info");
        info.strTitle = strNumber;
        PhoneInfo num;
        num.Type = PType_Unknown;
        num.strNumber = strNumber;
        info.arrPhones += num;
        info.selected = 0;
    } else {
        // Found it, now set the "selected" field correctly
        info.selected = 0;
        foreach (PhoneInfo num, info.arrPhones) {
            QString strNum = num.strNumber;
            GVAccess::simplify_number (strNum, false);
            strNum.remove(' ').remove('+');

            if (-1 != strNum.indexOf (strTrunc)) {
                break;
            }
            info.selected++;
        }

        if (info.selected >= info.arrPhones.size ()) {
            info.selected = 0;
            rv = false;
        }
    }

    return (rv);
}//MainWindow::findInfo

void
MainWindow::dialNow (const QString &strTarget)
{
    CalloutInitiator *ci;

    do // Begin cleanup block (not a loop)
    {
        if (!bLoggedIn) {
            setStatus ("User is not logged in yet. Cannot make any calls.");
            break;
        }

        QMutexLocker locker (&mtxDial);
        if (bCallInProgress) {
            setStatus ("Another call is in progress. Please try again later");
            break;
        }

        GVRegisteredNumber gvRegNumber;
        if (!getDialSettings (bDialout, gvRegNumber, ci))
        {
            setStatus ("Unable to dial because settings are not valid.");
            break;
        }

        if (strTarget.isEmpty ()) {
            setStatus ("Cannot dial empty number");
            break;
        }

        QString strTest = strTarget;
        strTest.remove(QRegExp ("\\d*"))
               .remove(QRegExp ("\\s"))
               .remove('+')
               .remove('-');
        if (!strTest.isEmpty ()) {
            setStatus ("Cannot use numbers with special symbols or characters");
            break;
        }

        DialContext *ctx = new DialContext(strSelfNumber, strTarget, this);
        if (NULL == ctx) {
            setStatus ("Failed to dial out because of allocation problem");
            break;
        }
        QObject::connect (ctx , SIGNAL(sigDialComplete(DialContext*,bool)),
                          this, SLOT(onSigDialComplete(DialContext*,bool)));

        GVAccess &webPage = Singletons::getRef().getGVAccess ();
        QVariantList l;
        l += strTarget;     // The destination number is common between the two
        l += QVariant::fromValue<void*>(ctx);

        OsDependent &osd = Singletons::getRef().getOSD ();
        osd.setLongWork (this, true);

        bCallInProgress = true;
        bDialCancelled = false;

        ctx->showMsgBox ();

        if (bDialout)
        {
            ctx->ci = ci;

            l += ci->selfNumber ();
            if (!webPage.enqueueWork (GVAW_dialOut, l, this,
                    SLOT (dialComplete (bool, const QVariantList &))))
            {
                setStatus ("Dialing failed instantly");
                bCallInProgress = bDialCancelled = false;
                ctx->deleteLater ();
                break;
            }
        }
        else
        {
            l += gvRegNumber.strDescription;
            l += QString (gvRegNumber.chType);
            if (!webPage.enqueueWork (GVAW_dialCallback, l, this,
                    SLOT (dialComplete (bool, const QVariantList &))))
            {
                setStatus ("Dialing failed instantly");
                bCallInProgress = bDialCancelled = false;
                ctx->deleteLater ();
                break;
            }
        }
    } while (0); // End cleanup block (not a loop)
}//MainWindow::dialNow

void
MainWindow::onSigText (const QString &strNumber)
{
    ContactInfo info;

    do { // Begin cleanup block (not a loop)
        // Get info about this number
        if (!findInfo (strNumber, info)) {
            qWarning () << "Unable to find information for " << strNumber;
            setStatus ("Unable to identify phone number");
            break;
        }

        SMSEntry entry;
        entry.strName = info.strTitle;
        entry.sNumber = info.arrPhones[info.selected];

        dlgSMS.addSMSEntry (entry);
        if (dlgSMS.isHidden ()) {
            dlgSMS.show ();
        }
    } while (0); // End cleanup block (not a loop)
}//MainWindow::onSigText

//! Invoked by the DBus Text server
/**
 * When the DBus Text server's text method is called, it finally reaches this
 * function. Here, we:
 * 1. Find out information (if there is any) about each number
 * 2. Add that info into the widget that collects numbers to send a text to
 * 3. Show the text widget
 */
void
MainWindow::onSendTextWithoutData (const QStringList &arrNumbers)
{
    foreach (QString strNumber, arrNumbers) {
        if (strNumber.isEmpty ()) {
            qWarning ("Cannot text empty number");
            continue;
        }

        ContactInfo info;

        // Get info about this number
        if (!findInfo (strNumber, info)) {
            qWarning () << "Unable to find information for " << strNumber;
            continue;
        }

        SMSEntry entry;
        entry.strName = info.strTitle;
        entry.sNumber = info.arrPhones[info.selected];

        dlgSMS.addSMSEntry (entry);
    }

    if (dlgSMS.isHidden ())
    {
        dlgSMS.show ();
    }
}//MainWindow::onSendTextWithoutData

void
MainWindow::onSigDialComplete (DialContext *ctx, bool ok)
{
    // Disconnecting this
    QMutexLocker locker (&mtxDial);
    if (ok) {
        if (!ctx->bDialOut) {
            emit dialCanFinish ();
        }
    } else {
        GVAccess &webPage = Singletons::getRef().getGVAccess ();
        bDialCancelled = true;
        webPage.cancelWork (ctx->bDialOut ? GVAW_dialOut : GVAW_dialCallback);
    }
}//MainWindow::onSigDialComplete

void
MainWindow::dialInProgress (const QString & /*strNumber*/)
{
}//MainWindow::dialInProgress

void
MainWindow::dialAccessNumber (const QString  &strAccessNumber,
                              const QVariant &context        )
{
    bool bSuccess = false;
    DialContext *ctx = (DialContext *) context.value<void *>();
    do // Begin cleanup block (not a loop)
    {
        if (NULL == ctx)
        {
            setStatus ("Invalid call out context", 3);
            setStatus ("Callout failed");
            break;
        }

        if (NULL == ctx->ci)
        {
            qWarning ("Invalid call out initiator");
            setStatus ("Callout failed");
            break;
        }

        ctx->ci->initiateCall (strAccessNumber);
        setStatus ("Callout in progress");
        bSuccess = true;
    } while (0); // End cleanup block (not a loop)
}//MainWindow::dialAccessNumber

void
MainWindow::dialComplete (bool bOk, const QVariantList &params)
{
    QMutexLocker locker (&mtxDial);
    DialContext *ctx = (DialContext *) params[1].value <void*> ();
    if (!bOk)
    {
        if (bDialCancelled)
        {
            setStatus ("Cancelled dial out");
        }
        else
        {
            setStatus ("Dialing failed", 10*1000);
            this->showMsgBox ("Dialing failed");
        }
    }
    else
    {
        setStatus (QString("Dial successful to %1.").arg(params[0].toString()));
    }
    bCallInProgress = false;

    OsDependent &osd = Singletons::getRef().getOSD ();
    osd.setLongWork (this, false);

    ctx->deleteLater ();
}//MainWindow::dialComplete

void
MainWindow::sendSMS (const QStringList &arrNumbers, const QString &strText)
{
    GVAccess &webPage = Singletons::getRef().getGVAccess ();
    QStringList arrFailed;
    QString msg;

    for (int i = 0; i < arrNumbers.size (); i++)
    {
        if (arrNumbers[i].isEmpty ()) {
            qWarning ("Cannot text empty number");
            continue;
        }

        QVariantList l;
        l += arrNumbers[i];
        l += strText;
        if (!webPage.enqueueWork (GVAW_sendSMS, l, this,
                SLOT (sendSMSDone (bool, const QVariantList &))))
        {
            arrFailed += arrNumbers[i];
            msg = QString ("Failed to send an SMS to %1").arg (arrNumbers[i]);
            qWarning () << msg;
            break;
        }
    } // loop through all the numbers

    if (0 != arrFailed.size ())
    {
        this->showMsgBox (QString("Failed to send %1 SMS")
                                 .arg (arrFailed.size ()));
        msg = QString("Could not send a text to %1")
                .arg (arrFailed.join (", "));
        setStatus (msg);
    }
}//MainWindow::sendSMS

void
MainWindow::sendSMSDone (bool bOk, const QVariantList &params)
{
    QString msg;
    if (!bOk)
    {
        msg = QString("Failed to send SMS to %1").arg (params[0].toString());
    }
    else
    {
        msg = QString("SMS sent to %1").arg (params[0].toString());
    }

    setStatus (msg);
}//MainWindow::sendSMSDone

bool
MainWindow::refreshRegisteredNumbers ()
{
    GVAccess &webPage = Singletons::getRef().getGVAccess ();

    bool rv = false;
    do { // Begin cleanup block (not a loop)
        if (!bLoggedIn)
        {
            qWarning ("Not logged in. Will not refresh registered numbers.");
            break;
        }

        arrNumbers.clear ();

        QVariantList l;
        QObject::connect(
            &webPage, SIGNAL (registeredPhone    (const GVRegisteredNumber &)),
             this   , SLOT   (gotRegisteredPhone (const GVRegisteredNumber &)));
        if (!webPage.enqueueWork (GVAW_getRegisteredPhones, l, this,
                SLOT (gotAllRegisteredPhones (bool, const QVariantList &))))
        {
            QObject::disconnect(
                &webPage,
                    SIGNAL (registeredPhone    (const GVRegisteredNumber &)),
                 this   ,
                    SLOT   (gotRegisteredPhone (const GVRegisteredNumber &)));
            qWarning ("Failed to retrieve registered contacts!!");
            break;
        }

        rv = true;
    } while (0); // End cleanup block (not a loop)

    return (rv);
}//MainWindow::refreshRegisteredNumbers

void
MainWindow::gotRegisteredPhone (const GVRegisteredNumber &info)
{
    QString msg = QString("\"%1\"=\"%2\"")
                    .arg (info.strName)
                    .arg (info.strDescription);
    qDebug () << msg;

    arrNumbers += info;
}//MainWindow::gotRegisteredPhone

void
MainWindow::gotAllRegisteredPhones (bool bOk, const QVariantList &)
{
    GVAccess &webPage = Singletons::getRef().getGVAccess ();
    QObject::disconnect(
        &webPage, SIGNAL (registeredPhone    (const GVRegisteredNumber &)),
         this   , SLOT   (gotRegisteredPhone (const GVRegisteredNumber &)));

    do { // Begin cleanup block (not a loop)
        if (!bOk)
        {
            this->showMsgBox ("Failed to retrieve registered phones");
            setStatus ("Failed to retrieve registered phones");
            break;
        }

        this->onCallInitiatorsChange (true);

        setStatus ("GV callbacks retrieved.");
    } while (0); // End cleanup block (not a loop)
}//MainWindow::gotAllRegisteredPhones

bool
MainWindow::getDialSettings (bool                 &bDialout   ,
                             GVRegisteredNumber   &gvRegNumber,
                             CalloutInitiator    *&initiator  )
{
    initiator = NULL;

    bool rv = false;
    do { // Begin cleanup block (not a loop)
        RegNumData data;
        if (!modelRegNumber.getAt (indRegPhone, data)) {
            qDebug ("Invalid registered phone index");
            break;
        }

        gvRegNumber.chType = data.chType;
        gvRegNumber.strName = data.strName;
        gvRegNumber.strDescription = data.strDesc;
        bDialout = (data.type == RNT_Callout);
        if (bDialout) {
            initiator = (CalloutInitiator *) data.pCtx;
        }

        rv = true;
    } while (0); // End cleanup block (not a loop)

    return (rv);
}//MainWindow::getDialSettings

void
MainWindow::retrieveVoicemail (const QString &strVmailLink)
{
    GVAccess &webPage = Singletons::getRef().getGVAccess ();

    do // Begin cleanup block (not a loop)
    {
        if (mapVmail.contains (strVmailLink))
        {
            setStatus ("Playing cached vmail");
            playVmail (mapVmail[strVmailLink]);
            break;
        }

        QString strTemplate = QDir::tempPath ()
                            + QDir::separator ()
                            + "qgv_XXXXXX.tmp.mp3";
        QTemporaryFile tempFile (strTemplate);
        if (!tempFile.open ())
        {
            qWarning ("Failed to get a temp file name");
            break;
        }
        QString strTemp = QFileInfo (tempFile.fileName ()).absoluteFilePath ();
        tempFile.close ();

        QVariantList l;
        l += strVmailLink;
        l += strTemp;
        if (!webPage.enqueueWork (GVAW_playVmail, l, this,
                SLOT (onVmailDownloaded (bool, const QVariantList &))))
        {
            qWarning ("Failed to play Voice mail");
            break;
        }
    } while (0); // End cleanup block (not a loop)
}//MainWindow::retrieveVoicemail

void
MainWindow::onVmailDownloaded (bool bOk, const QVariantList &arrParams)
{
    QString strFilename = arrParams[1].toString ();
    if (bOk)
    {
        QString strVmailLink = arrParams[0].toString ();
        if (!mapVmail.contains (strVmailLink))
        {
            mapVmail[strVmailLink] = strFilename;
            setStatus ("Voicemail downloaded");
        }
        else
        {
            setStatus ("Voicemail already existed. Using cached vmail");
            if (strFilename != mapVmail[strVmailLink]) {
                QFile::remove (strFilename);
            }
        }

        playVmail (mapVmail[strVmailLink]);
    }
    else
    {
        QFile::remove (strFilename);
    }
}//MainWindow::onVmailDownloaded

void
MainWindow::playVmail (const QString &strFile)
{
    do // Begin cleanup block (not a loop)
    {
        // Convert it into a file:// url
        QUrl url = QUrl::fromLocalFile(strFile).toString ();

        qDebug() << "Play vmail file:" << strFile << "Url =" << url;

        vmailPlayer.setMedia (QMediaContent(url));
        vmailPlayer.setVolume (50);
        vmailPlayer.play();
    } while (0); // End cleanup block (not a loop)
}//MainWindow::playVmail

void
MainWindow::onVmailPlayerStateChanged(QMediaPlayer::State state)
{
    int newstate = (int) state;
    qDebug() << "Vmail player state changed to" << newstate;
    QDeclarativeContext *ctx = this->rootContext();
    ctx->setContextProperty ("g_vmailPlayerState", newstate);
}//MainWindow::onVmailPlayerStateChanged

void
MainWindow::onSigVmailPlayback (int newstate)
{
    switch(newstate) {
    case 0:
        qDebug ("QML asked us to stop vmail");
        vmailPlayer.stop ();
        break;
    case 1:
        qDebug ("QML asked us to play vmail");
        vmailPlayer.play ();
        break;
    case 2:
        qDebug ("QML asked us to pause vmail");
        vmailPlayer.pause ();
        break;
    default:
        qDebug() << "Unknown newstate =" << newstate;
        break;
    }
}//MainWindow::onSigVmailPlayback

void
MainWindow::onRegPhoneSelectionChange (int index)
{
    indRegPhone = index;

    CacheDatabase &dbMain = Singletons::getRef().getDBMain ();
    dbMain.putCallback (QString("%1").arg (indRegPhone));

    RegNumData data;
    if (!modelRegNumber.getAt (indRegPhone, data)) {
        data.strName = "<Unknown>";
    }

    QDeclarativeContext *ctx = this->rootContext();
    ctx->setContextProperty ("g_CurrentPhoneName", data.strName);

    OsDependent &osd = Singletons::getRef().getOSD ();
    osd.setLongWork (this, false);
}//MainWindow::onRegPhoneSelectionChange

void
MainWindow::onRefresh ()
{
    qDebug ("Refresh all requested.");

    refreshRegisteredNumbers ();
    oInbox.refresh ();
    oContacts.refreshContacts ();
}//MainWindow::onRefresh

void
MainWindow::onRefreshAll ()
{
    qDebug ("Refresh all requested.");

    refreshRegisteredNumbers ();
    oInbox.refreshFullInbox ();
    oContacts.refreshAllContacts ();
}//MainWindow::onRefreshAll

void
MainWindow::onSigProxyChanges(bool bEnable, bool bUseSystemProxy,
                              const QString &host, int port, bool bRequiresAuth,
                              const QString &user, const QString &pass)
{
    // Send to WebPage.
    GVAccess &webPage = Singletons::getRef().getGVAccess ();
    webPage.setProxySettings (bEnable, bUseSystemProxy, host, port,
                              bRequiresAuth, user, pass);

    CacheDatabase &dbMain = Singletons::getRef().getDBMain ();
    dbMain.setProxySettings (bEnable, bUseSystemProxy, host, port,
                             bRequiresAuth, user, pass);

    do // Begin cleanup block (not a loop)
    {
        QObject *pRoot = this->rootObject ();
        if (NULL == pRoot) {
            qWarning ("Couldn't get root object in QML for ProxySettingsPage");
            break;
        }

        QObject *pProxySettings = pRoot->findChild <QObject*>
                                                  ("ProxySettingsPage");
        if (NULL == pProxySettings) {
            qWarning ("Could not get to ProxySettingsPage");
            break;
        }

        QMetaObject::invokeMethod (pProxySettings, "setValues",
                                   Q_ARG (QVariant, QVariant(bEnable)),
                                   Q_ARG (QVariant, QVariant(bUseSystemProxy)),
                                   Q_ARG (QVariant, QVariant(host)),
                                   Q_ARG (QVariant, QVariant(port)),
                                   Q_ARG (QVariant, QVariant(bRequiresAuth)),
                                   Q_ARG (QVariant, QVariant(user)),
                                   Q_ARG (QVariant, QVariant(pass)));
    } while (0); // End cleanup block (not a loop)
}//MainWindow::onSigProxyChanges

void
MainWindow::onLinkActivated (const QString &strLink)
{
    qDebug() << "MainWindow: Link activated" << strLink;
    QDesktopServices::openUrl (QUrl::fromUserInput (strLink));
}//MainWindow::onLinkActivated

void
MainWindow::onSigMosquittoChanges (bool bEnable, const QString &host, int port,
                                   const QString &topic)
{
    CacheDatabase &dbMain = Singletons::getRef().getDBMain ();
    dbMain.setMqSettings (bEnable, host, port, topic);

    do // Begin cleanup block (not a loop)
    {
        QObject *pRoot = this->rootObject ();
        if (NULL == pRoot) {
            qWarning ("Couldn't get root object in QML for MosquittoPage");
            break;
        }

        QObject *pMqSettings = pRoot->findChild <QObject*>
                                                  ("MosquittoPage");
        if (NULL == pMqSettings) {
            qWarning ("Could not get to MosquittoPage");
            break;
        }

        QString strHost = host, strTopic = topic;
        if (host.isEmpty ()) {
            // This definitely does not exist.
            strHost = "mosquitto.example.com";
        }
        if (0 == port) {
            // Default mosquitto port
            port = 1883;
        }
        if (topic.isEmpty ()) {
            // Default topic
            strTopic = "gv_notify";
        }

        QMetaObject::invokeMethod (pMqSettings, "setValues",
                                   Q_ARG (QVariant, QVariant(bEnable)),
                                   Q_ARG (QVariant, QVariant(strHost)),
                                   Q_ARG (QVariant, QVariant(port)),
                                   Q_ARG (QVariant, QVariant(strTopic)));
    } while (0); // End cleanup block (not a loop)

#if MOSQUITTO_CAPABLE
    mqThread.setSettings (bEnable, host, port);
    bRunMqThread = bEnable;
    QObject::disconnect (&mqThread, SIGNAL(finished()),
                       this    , SLOT(onMqThreadFinished()));
    QObject::connect (&mqThread, SIGNAL(finished()),
                       this    , SLOT(onMqThreadFinished()));
    if (mqThread.isRunning ()) {
        mqThread.setQuit ();
    } else {
        onMqThreadFinished ();
    }
#endif
}//MainWindow::onSigMosquittoChanges

void
MainWindow::onMqThreadFinished ()
{
#if MOSQUITTO_CAPABLE
    QObject::disconnect (&mqThread, SIGNAL(finished()),
                          this    , SLOT(onMqThreadFinished()));
    if (bRunMqThread) {
        qDebug ("Finished waiting for Mq thread, restarting thread.");
        mqThread.start();
    } else {
        qDebug ("Finished waiting for Mq thread. Not restarting");
    }
#endif
}//MainWindow::onMqThreadFinished

void
MainWindow::showMsgBox (const QString &strMessage)
{
    QDeclarativeContext *ctx = this->rootContext();
    ctx->setContextProperty ("g_bShowMsg", true);
    ctx->setContextProperty ("g_strMsgText", strMessage);
}//MainWindow::showMsgBox

void
MainWindow::onSigMsgBoxDone (bool /*ok*/)
{
    QDeclarativeContext *ctx = this->rootContext();
    ctx->setContextProperty ("g_bShowMsg", false);
}//MainWindow::onSigMsgBoxDone

void
MainWindow::onCallInitiatorsChange (bool bSave)
{
    // Set the correct callback
    CacheDatabase &dbMain = Singletons::getRef().getDBMain ();
    QString strCallback;
    bool bGotCallback = dbMain.getCallback (strCallback);

    QString strCiName;
    modelRegNumber.clear ();
    for (int i = 0; i < arrNumbers.size (); i++)
    {
        strCiName = "Dial back: " + arrNumbers[i].strName;
        modelRegNumber.insertRow (strCiName,
                                  arrNumbers[i].strDescription,
                                  arrNumbers[i].chType);
    }

    // Store the callouts in the same widget as the callbacks
    CallInitiatorFactory& cif = Singletons::getRef().getCIFactory ();
    CalloutInitiatorList listCi = cif.getInitiators ();
    foreach (CalloutInitiator *ci, listCi) {
        if (ci->isValid ()) {
            strCiName = "Dial out: " + ci->name ();
            modelRegNumber.insertRow (strCiName, ci->selfNumber (), ci);
        }
    }

    if (bGotCallback) {
        indRegPhone = strCallback.toInt ();
    }

    if (bSave)
    {
        // Save all callbacks into the cache
        dbMain.putRegisteredNumbers (arrNumbers);
    }

    onRegPhoneSelectionChange (indRegPhone);
}//MainWindow::onCallInitiatorsChange

void
MainWindow::setUsername(const QString &strU)
{
    do // Begin cleanup block (not a loop)
    {
        QObject *pRoot = this->rootObject ();
        if (NULL == pRoot) {
            qWarning ("Couldn't get QML root object for setUsername");
            break;
        }

        QObject *pSettingsPage = pRoot->findChild <QObject*>("SettingsPage");
        if (NULL == pSettingsPage) {
            qWarning ("Could not get to SettingsPage for setUsername");
            break;
        }

        QMetaObject::invokeMethod (pSettingsPage, "setUsername",
                                   Q_ARG (QVariant, QVariant(strU)));
    } while (0); // End cleanup block (not a loop)
}//MainWindow::setUsername

void
MainWindow::setPassword(const QString &strP)
{
    do // Begin cleanup block (not a loop)
    {
        QObject *pRoot = this->rootObject ();
        if (NULL == pRoot) {
            qWarning ("Couldn't get QML root object for setPassword");
            break;
        }

        QObject *pSettingsPage = pRoot->findChild <QObject*>("SettingsPage");
        if (NULL == pSettingsPage) {
            qWarning ("Could not get to SettingsPage for setPassword");
            break;
        }

        QMetaObject::invokeMethod (pSettingsPage, "setPassword",
                                   Q_ARG (QVariant, QVariant(strP)));
    } while (0); // End cleanup block (not a loop)
}//MainWindow::setPassword
