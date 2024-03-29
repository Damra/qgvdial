TARGET=qgvdial

# Common to Qt4 and Qt5:
QT *= core gui webkit
# Only for Qt5:
greaterThan(QT_MAJOR_VERSION, 4) {
message(Qt version $$QT_MAJOR_VERSION)
QT *= widgets webkitwidgets
}

# Add files and directories to ship with the application
# by adapting the examples below.
# file1.source = myfile
# dir1.source = mydir
DEPLOYMENTFOLDERS = # file1 dir1

!win32{
MOC_DIR = moc
OBJECTS_DIR = obj
}

INCLUDEPATH += $$PWD

SOURCES  += $$PWD/MainWindow.cpp \
            $$PWD/MainWindow_p.cpp \
            $$PWD/OsDependant.cpp \
            $$PWD/ContactDialog.cpp \
            $$PWD/InboxEntryDialog.cpp \
            $$PWD/CINumberDialog.cpp \
            $$PWD/CiListView.cpp \
            $$PWD/GvNumComboBox.cpp \
            $$PWD/DblClickLabel.cpp \
            $$PWD/DummySystemTray.cpp \
            $$PWD/MyLineEdit.cpp \
            $$PWD/SmsDialog.cpp \
            $$PWD/VmailDialog.cpp

HEADERS  += $$PWD/MainWindow.h \
            $$PWD/MainWindow_p.h \
            $$PWD/OsDependant.h \
            $$PWD/ContactDialog.h \
            $$PWD/InboxEntryDialog.h \
            $$PWD/CINumberDialog.h \
            $$PWD/CiListView.h \
            $$PWD/GvNumComboBox.h \
            $$PWD/DblClickLabel.h \
            $$PWD/DummySystemTray.h \
            $$PWD/MyLineEdit.h \
            $$PWD/SmsDialog.h \
            $$PWD/VmailDialog.h

FORMS    += $$PWD/mainwindow.ui \
            $$PWD/ContactDialog.ui \
            $$PWD/InboxEntryDialog.ui \
            $$PWD/CINumberDialog.ui \
            $$PWD/SmsDialog.ui \
            $$PWD/VmailDialog.ui

RESOURCES += $$PWD/qtnotqml.qrc

include($$PWD/../common/common.pri)
include($$PWD/../features/mqlib/mqlib.pri)

!blackberry {
    include($$PWD/../features/qtsingleapplication/qtsingleapplication.pri)
}
