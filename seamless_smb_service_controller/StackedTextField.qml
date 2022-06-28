import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Item {    
    property alias name: lbl.text
    property alias text: txtf.text
    ColumnLayout{
        Label{
            id: lbl
        }
        TextField{
            id: txtf

        }
    }
}
