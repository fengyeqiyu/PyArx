// PPButton.qml —— 配电干线图工具栏按钮（对齐 HTML .btn 外观）
import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
    id: root
    property bool primary: false
    property bool hot: false          // 用于 pulse 高亮等场景
    hoverEnabled: true

    padding: 5
    leftPadding: 10
    rightPadding: 10
    font.pixelSize: 13
    font.family: "Microsoft YaHei"
    implicitHeight: 28
    focusPolicy: Qt.NoFocus

    contentItem: Text {
        text: root.text
        font: root.font
        color: root.primary ? "#2F6FBA" : "#1F2A37"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideNone
    }

    background: Rectangle {
        radius: 4
        border.width: 1
        border.color: root.primary ? "#2F6FBA" : "#CBD5E1"
        color: root.down ? "#E2E8F0" : (root.hovered ? "#F1F5F9" : "#FFFFFF")
    }
}
