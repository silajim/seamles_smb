import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    EditDialog{
        id:eddial;
        z:2
        height: parent.height - 50
        width:  parent.width - 50
        anchors.centerIn: parent
    }

    ListView{
        id:listview
        model:app.model
        anchors{
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: buttons.top
            margins: 5
        }

        delegate: Rectangle {
            height: grid.implicitHeight + 15

            width: listview.width

            MouseArea{
                id:mouse
                anchors.fill: parent
                onDoubleClicked: {
                    //open editor
                }
            }

            GridLayout{
                id: grid
                rows: 3
                columns: 2
                anchors.fill: parent
                anchors.margins: 5


                Label{
                    Layout.row: 0
                    Layout.column: 0
                    Layout.columnSpan: 1;
                    Layout.rowSpan: 1

                    text: "Root Dir: " + Rootdirectory
                }
                Label{
                    text: "Mount point: " + MountPoint
                    Layout.row: 0
                    Layout.column: 1
                    Layout.columnSpan: 1;
                    Layout.rowSpan: 1
                }

                Label{
                    text: "UNCName: " + UNCName
                    Layout.row: 1
                    Layout.column: 0
                    Layout.columnSpan: 1;
                    Layout.rowSpan: 1
                }

                Label{
                    text: "Volume name: " + volname
                    Layout.row: 1
                    Layout.column: 1
                    Layout.columnSpan: 1;
                    Layout.rowSpan: 1
                }

                Row{
                    Layout.row: 2
                    Layout.column: 0
                    Layout.columnSpan: 1;
                    Layout.rowSpan: 1
                    spacing: 5
                    Label{

                        text: running ? "Running" : "Not running"
                        color: running ? "Green" : "Red"
                    }

                    Label{
                        text:  "CaseSensitive:"  + CaseSensitive
                    }

                    Label{
                        text: "Enabled:" + model.enabled
                    }

                    Label{
                        text: "Keep alive:" + keepAlive
                    }

                }

                Row{
                    Layout.row: 2
                    Layout.column: 1
                    Layout.columnSpan: 1;
                    Layout.rowSpan: 1

                    Button{
                        width: implicitContentWidth + 10
                        padding: 0
                        text: "Start"
                        onClicked: {

                        }
                    }
                    Button{
                        width: implicitContentWidth + 10
                        padding: 0
                        text: "Stop"
                    }
                    Button{
                        width: implicitContentWidth + 10
                        padding: 0
                        text: "Delete"
                        onClicked: app.model.remove(model.id)
                    }
                    Button{
                        width: implicitContentWidth + 10
                        padding: 0
                        text: "Edit"
                        onClicked:{
                            eddial.editor = app.model.edit(id);
                            eddial.open();
                        }
                    }

                }

            }

            Rectangle{
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: "lightgrey"
            }

        }
    }

    Rectangle{
        //        color:"orange"

        id:buttons
        height: 50
        anchors{
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        RowLayout{
            id:row

            anchors.fill: parent

            Button{
                text: "New"
                Layout.leftMargin: 10
                Layout.rightMargin: 0
                Layout.alignment: Qt.AlignLeft || Qt.AlignVCenter
                onClicked: {
                    eddial.editor = app.model.newMount();
                    eddial.open();
                }
            }

            Button{
                text: "Start"
                enabled: !app.isRunning && app.isInstalled
                Layout.leftMargin: 10
                Layout.rightMargin: 0
                Layout.alignment: Qt.AlignLeft || Qt.AlignVCenter

                onClicked: app.start()
            }
            Button{
                text: "Stop"
                enabled: app.isRunning && app.isInstalled
                //                Layout.leftMargin: 10
                Layout.alignment: Qt.AlignLeft || Qt.AlignVCenter
                onClicked: app.stop()
            }


            Button{
                text: "Install"
                enabled: !app.isInstalled
                //                Layout.leftMargin: 10
                Layout.alignment: Qt.AlignLeft || Qt.AlignVCenter

                onClicked: app.install()
            }
            Button{
                text: "Uninstall"
                enabled: app.isInstalled
                //                Layout.leftMargin: 10
                Layout.alignment: Qt.AlignLeft || Qt.AlignVCenter

                onClicked: app.uninstall()
            }

            Item{
                Layout.fillWidth: true
            }


        }
    }
}
