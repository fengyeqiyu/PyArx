import QtQuick 2.15

// 桥接验证面板：点按钮 -> C++ backend.addNumbers -> Python 计算 -> 结果回到这里
Item {
    id: root
    width: 340
    height: 620

    Rectangle { anchors.fill: parent; color: "#1e1f22" }

    Text {
        id: title
        x: 12; y: 12
        text: "Python 桥接测试"
        color: "#6cb6ff"
        font.pixelSize: 18
        font.bold: true
    }
    Text {
        x: 12; y: 42
        text: "QML 按钮 → C++ → Python → 回到 QML 文本"
        color: "#9aa0a6"
        font.pixelSize: 12
    }
    Rectangle { x: 12; y: 70; width: parent.width - 24; height: 1; color: "#3a3d42" }

    // 点击入口：坐标固定 (12,96)-(328,140)，便于自动点击
    Rectangle {
        id: button
        x: 12; y: 96
        width: parent.width - 24
        height: 44
        radius: 6
        color: mouse.pressed ? "#3c6fa8" : "#2f6390"
        border.color: "#6cb6ff"
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: "点我：1234 + 5678"
            color: "#e3e5e8"
            font.pixelSize: 14
        }

        MouseArea {
            id: mouse
            anchors.fill: parent
            onClicked: {
                backend.reportClick("按钮")
                backend.addNumbers(1234, 5678)
            }
        }
    }

    // 结果区（由 Python 计算后回写）
    Rectangle {
        x: 12; y: 160
        width: parent.width - 24
        height: 96
        radius: 6
        color: "#26282c"

        Column {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            Text {
                text: "Python 计算结果"
                color: "#9aa0a6"
                font.pixelSize: 12
            }
            Text {
                id: resultText
                objectName: "resultText"
                text: backend.result
                color: "#4ec97c"
                font.pixelSize: 20
                font.bold: true
            }
        }
    }

    // 通道状态（Python 侧 push 进来的文本）
    Rectangle {
        x: 12; y: 272
        width: parent.width - 24
        height: 60
        radius: 6
        color: "#26282c"

        Text {
            anchors.fill: parent
            anchors.margins: 12
            text: backend.status
            color: "#e3e5e8"
            font.pixelSize: 12
            wrapMode: Text.WordWrap
        }
    }

    Text {
        x: 12; y: 348
        text: "尺寸 " + Math.round(root.width) + "×" + Math.round(root.height)
        color: "#9aa0a6"
        font.pixelSize: 12
    }
}
