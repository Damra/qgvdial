/*
qgvdial is a cross platform Google Voice Dialer
Copyright (C) 2009-2013  Yuvraaj Kelkar

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

import QtQuick 1.1
import com.nokia.meego 1.1
import com.nokia.extras 1.1

PageStackWindow {
    id: appWindow
    objectName: "MainPageStack"

    Component.onCompleted: {
        // Use the dark theme.
        theme.inverted = true;
    }

    function pushTfaDlg() {
        pageStack.push(tfaPinDlg);
    }
    function showMsgBox(msg) {
        msgBox.message = msg;
        pageStack.push(msgBox);
    }

    initialPage: Page {
        tools: commonTools

        TabGroup {
            id: tabgroup
            objectName: "MainTabGroup"
            currentTab: dialTab

            function setTab(index) {
                if (0 === index) {
                    currentTab = dialTab;
                } else if (1 === index) {
                    currentTab = contactsTab;
                } else if (2 === index) {
                    currentTab = inboxTab;
                } else if (3 === index) {
                    currentTab = settingsTab;
                }
            }

            DialPage {
                id: dialTab
                toolbarHeight: appWindow.platformToolBarHeight
            }
            ContactsPage {
                id: contactsTab
                toolbarHeight: appWindow.platformToolBarHeight
            }
            InboxPage {
                id: inboxTab
                toolbarHeight: appWindow.platformToolBarHeight
            }
            SettingsPage {
                id: settingsTab
                toolbarHeight: appWindow.platformToolBarHeight
            }
        }//TabGroup

        ToolBarLayout {
            id: commonTools
            visible: true

            ToolIcon {
                iconId: "toolbar-back";
                onClicked: pageStack.pop();
            }

            ButtonRow {
                TabButton {
                    iconSource: "qrc:/dialpad.svg"
                    tab: dialTab
                }
                TabButton {
                    iconSource: "qrc:/people.svg"
                    tab: contactsTab
                }
                TabButton {
                    iconSource: "qrc:/history.svg"
                    tab: inboxTab
                }
                TabButton {
                    iconSource: "qrc:/settings.svg"
                    tab: settingsTab
                }
            }
            ToolIcon {
                platformIconId: "toolbar-view-menu"
                anchors.right: (parent === undefined) ? undefined : parent.right
                onClicked: (myMenu.status === DialogStatus.Closed) ? myMenu.open() : myMenu.close()
            }
        }//ToolBarLayout
    }

    Menu {
        id: myMenu
        visualParent: appWindow
        MenuLayout {
            MenuItem {
                text: qsTr("Refresh")
            }
        }
    }//Menu

    TfaPinPage {
        id: tfaPinDlg
        objectName: "TFAPinDialog"

        onDone: appWindow.pageStack.pop();
    }//TFA Dialog

    MessageBox {
        id: msgBox
        onDone: appWindow.pageStack.pop();
    }

    InfoBanner {
        id: infoBanner
        objectName: "InfoBanner"
    }
}