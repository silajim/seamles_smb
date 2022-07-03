import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Item {
    id: itm
    property alias name: lbl.text
    property alias text: txtf.text
//    height: lay.preferredHeight
//    width: lay.preferredWidth
    property int preferredHeight
    property int preferredWidth

    signal editingFinished;
    ColumnLayout{
        id:lay
//        anchors.centerIn: parent
        Label{
            id: lbl
        }
        TextEdit {
            id: txtf
            selectByMouse: true
            Layout.preferredWidth: preferredWidth
            Layout.preferredHeight: preferredHeight
            wrapMode: TextEdit.Wrap
            onEditingFinished:{
                itm.editingFinished();
            }
        }
    }
}
