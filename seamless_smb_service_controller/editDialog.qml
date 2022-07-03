import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Dialog {
    property var editor:null

    onAccepted:{
//        app.model.editaccepted();
        editor.isaccepted();
    }
    onRejected: {
//        app.model.editrejected();
        editor.isrejected();
    }
    onClosed:  app.model.editrejected();

    standardButtons: Dialog.Ok | Dialog.Cancel
    GridLayout{
        id:grid
        anchors.fill: parent
        columns: 2
        rows: 3
        anchors.margins: 5        


        property double colMulti : grid.width / grid.columns
        property double rowMulti : grid.height / grid.rows

        function prefWidth(item){
            return colMulti * item.Layout.columnSpan
        }
        function prefHeight(item){
            return rowMulti * item.Layout.rowSpan
        }

        StackedTextField{
            Layout.row: 0
            Layout.column: 0
            Layout.columnSpan: 1;
            Layout.rowSpan: 1
            Layout.alignment: Qt.AlignCenter
            Layout.preferredWidth  : grid.prefWidth(this)
            Layout.preferredHeight : grid.prefHeight(this)


            name: "Root Directory:"
            text: editor.RootDirectory
            onTextChanged: editor.RootDirectory = text

//            Binding{
//                target:editor
//                property:  "RootDirectory"
//                value: parent.text
//            }
        }
        StackedTextField{
            Layout.row: 0
            Layout.column: 1
            Layout.columnSpan: 1;
            Layout.rowSpan: 1
            Layout.alignment: Qt.AlignCenter

            Layout.preferredWidth  : grid.prefWidth(this)
            Layout.preferredHeight : grid.prefHeight(this)
            name: "Mount point:"
            text: editor.MountPoint
            onTextChanged: editor.MountPoint = text

        }

        StackedTextField{

            Layout.row: 1
            Layout.column: 0
            Layout.columnSpan: 1;
            Layout.rowSpan: 1
            Layout.alignment: Qt.AlignCenter

            Layout.preferredWidth  : grid.prefWidth(this)
            Layout.preferredHeight : grid.prefHeight(this)

            name: "UNCName:"
            text: editor.UNCName
            onTextChanged: editor.UNCName = text
        }

        StackedTextField{
            Layout.row: 1
            Layout.column: 1
            Layout.columnSpan: 1;
            Layout.rowSpan: 1
            Layout.alignment: Qt.AlignCenter

            Layout.preferredWidth  : grid.prefWidth(this)
            Layout.preferredHeight : grid.prefHeight(this)

            name: "Volume name:"
            text: editor.Volname
            onTextChanged: editor.Volname = text
        }

        StackedTextEditor{
            Layout.row: 2
            Layout.column: 0
            Layout.columnSpan: 1;
            Layout.rowSpan: 1
            Layout.alignment: Qt.AlignCenter

            Layout.preferredWidth  : grid.prefWidth(this)
            Layout.preferredHeight : grid.prefHeight(this)

            preferredWidth:  Layout.preferredWidth
            preferredHeight: Layout.preferredHeight

            name: "Options:"
            text: editor.Options

//            onTextChanged: editor.Options = text
            onEditingFinished:{
               editor.Options = text
            }

        }

        Flow{
            Layout.row: 2
            Layout.column: 1
            Layout.columnSpan: 1;
            Layout.rowSpan: 1
            Layout.alignment: Qt.AlignCenter

            Layout.preferredWidth  : grid.prefWidth(this)
            Layout.preferredHeight : grid.prefHeight(this)

            spacing: 5
            CheckBox {
                text:  "CaseSensitive"
                checked: editor.CaseSenitive
                onCheckedChanged: editor.CaseSenitive = checked
            }

            CheckBox{
                text: "Enabled"
                checked: editor.Enabled
                onCheckedChanged: editor.Enabled = checked
            }

            CheckBox{
                text: "Keep alive"
                checked: editor.KeepAlive
                onCheckedChanged: editor.KeepAlive = checked
            }
            CheckBox{
                text: "Debug"
                checked: editor.Debug
                onCheckedChanged: editor.Debug = checked
            }

        }

    }
}
