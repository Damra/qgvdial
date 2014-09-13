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

import Qt 4.7
import "meego"
import "generic"
import "s3"

Item {
    id: container
    objectName: "PinSettingsPage"

    height: mainColumn.height + 2

    function setValues(bEnable, pin) {
        console.debug ("QML: Setting Pin settings")
        pinSupport.checked = bEnable;
        textPin.text = pin;
    }

    signal sigDone(bool bSave)
    signal sigPinSettingChanges(bool bEnable, string pin)

    property bool bEnable: pinSupport.checked

    onSigDone: {
        textPin.closeSoftwareInputPanel();
    }

    Column {
        id: mainColumn
        anchors {
            top: parent.top
            left: parent.left
        }
        spacing: 2
        width: parent.width

        QGVRadioButton {
            id: pinSupport
            width: parent.width

            text: "Use PIN for GV dial"
            pointSize: (8 * g_fontMul)
        }//QGVRadioButton (pinSupport)

        Row {
            width: parent.width
            spacing: 2

            opacity: (bEnable ? 1 : 0)

            QGVLabel {
                id: lblPin
                text: "Pin:"
                anchors.verticalCenter: parent.verticalCenter
                height: paintedHeight + 2
            }//QGVLabel ("Pin:")

            QGVTextInput {
                id: textPin
                width: parent.width - lblPin.width
                anchors.verticalCenter: parent.verticalCenter
                text: "0000"
                validator: IntValidator { bottom: 0; top: 9999 }
                height: lblPin.height
                fontPointMultiplier: 8.0 / 10.0
            }//QGVTextInput (pin)
        }// Row (pin)

        SaveCancel {
            anchors {
                left: parent.left
                leftMargin: 1
            }
            width: parent.width - 1

            onSigSave: {
                container.sigPinSettingChanges (bEnable, textPin.text);
                container.sigDone(true);
            }

            onSigCancel: container.sigDone(false);
        }// Save and cancel buttons
    }// Column
}// Item (top level)