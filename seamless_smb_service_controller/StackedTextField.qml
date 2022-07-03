import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Item {    
    property alias name: lbl.text
    property alias text: txtf.text
    height: lay.implicitHeight
    width: lay.implicitWidth
    ColumnLayout{
        id:lay
        Label{
            id: lbl
        }
        TextField{
            id: txtf

        }
    }
}
