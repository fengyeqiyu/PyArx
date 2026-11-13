// PPText.qml —— 单行输入框（工具栏 / 节点表单 / 编辑表格共用）
import QtQuick 2.15
import QtQuick.Controls 2.15

TextField {
    id: root
    property string hint: ""
    property int hintSize: 12

    font.pixelSize: 12
    font.family: "Microsoft YaHei"
    color: "#1F2A37"
    selectByMouse: true
    padding: 4
    leftPadding: 6
    rightPadding: 6
    implicitHeight: 26
    placeholderTextColor: "#8A94A6"
    placeholderText: root.hint

    background: Rectangle {
        radius: 3
        border.width: 1
        border.color: root.activeFocus ? "#2F6FBA" : "#D0D5DD"
        color: root.activeFocus ? "#FFFFFF" : "transparent"
    }
}
