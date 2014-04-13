/*
qgvdial is a cross platform Google Voice Dialer
Copyright (C) 2009-2014  Yuvraaj Kelkar

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
import com.nokia.symbian 1.1

Page {
    id: container

    signal done(bool accepted)
    signal setNumberToDial(string number)

    property alias imageSource: contactImage.source
    property alias name: contactName.text
    property alias phonesModel: detailsView.model

    Column {
        anchors.fill: parent
        spacing: 5

        Row {
            id: titleRow
            width: parent.width
            height: contactImage.height

            Image {
                id: contactImage
                fillMode: Image.PreserveAspectFit
                height: 200
                width: 200
            }
            Label {
                id: contactName
                anchors.verticalCenter: parent.verticalCenter
                font.pixelSize: 70
                smooth: true
                width: parent.width - contactImage.width - parent.spacing
            }
        }

        ListView {
            id: detailsView

            width: parent.width - 40
            height: parent.height - titleRow.height - parent.spacing

            anchors {
                left: parent.left
                leftMargin: 20
                right: parent.right
                rightMargin: 20
            }

            delegate: Item {
                height: lblNumber.height + 10
                width: parent.width

                Rectangle {
                    id: hitRectNumber
                    anchors.fill: parent
                    color: "orange"
                    opacity: 0
                }

                Row {
                    width: parent.width

                    Label {
                        id: lblType
                        text: type
                        font.pixelSize: 50
                    }

                    Label {
                        id: lblNumber
                        text: number
                        width: parent.width - lblType.width - parent.spacing
                        horizontalAlignment: Text.AlignRight
                        font.pixelSize: 50
                    }
                }//Row: type (work/home/mobile) and number

                MouseArea {
                    anchors.fill: parent
                    onPressed: { hitRectNumber.opacity = 1.0; }
                    onReleased: { hitRectNumber.opacity = 0.0; }
                    onPressAndHold: {
                        hitRectNumber.opacity = 0.0;
                        container.setNumberToDial(number);
                        container.done(true);
                    }
                }
            }//delegate
        }//ListView
    }//Column
}//Contact details page