import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Dialog {
    property var editor:null

    standardButtons: Dialog.Ok | Dialog.Cancel
    GridLayout{
        anchors.fill: parent
        columns: 2
        rows: 3
        anchors.margins: 5

        StackedTextField{
            Layout.row: 0
            Layout.column: 0
            Layout.columnSpan: 1;
            Layout.rowSpan: 1

            name: "Root Directory:"
            text: editor.RootDirectory
        }
        StackedTextField{
            Layout.row: 0
            Layout.column: 1
            Layout.columnSpan: 1;
            Layout.rowSpan: 1
            name: "Mount point:"
            text: editor.MountPoint

        }

        StackedTextField{

            Layout.row: 1
            Layout.column: 0
            Layout.columnSpan: 1;
            Layout.rowSpan: 1
            name: "UNCName:"
            text: editor.UNCName
        }

        StackedTextField{
            Layout.row: 1
            Layout.column: 1
            Layout.columnSpan: 1;
            Layout.rowSpan: 1
            name: "Volume name:"
            text: editor.volname
        }

        Row{
            Layout.row: 2
            Layout.column: 0
            Layout.columnSpan: 1;
            Layout.rowSpan: 1
            spacing: 5
            CheckBox {
                text:  "CaseSensitive:"
                checked: editor.CaseSensitive
            }

            CheckBox{
                text: "Enabled:"
                checked: editor.enabled
            }

            CheckBox{
                text: "Keep alive:"
                checked: model.keepAlive
            }

        }

    }
}
