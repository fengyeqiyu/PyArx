import QtQuick 2.15

// PyArxQml 示例面板 —— 演示 Python 与 QML 双向控制。
// QML -> Python：backend.addNumbers / backend.call("名字") / backend.reportClick
// Python -> QML：PyArxQml.set_prop("pyResult", x) / set_named("pyText", "text", x) / call("flash", x)
Item {
    id: root
    width: 340
    height: 620

    // Python 可以直接写这些属性
    property string pyResult: "（等待 Python 写入）"
    property string pyTip: "Python 尚未调用 set_prop"
    property color pyColor: "#4ec97c"

    // Python 可以直接调用这个函数：PyArxQml.call("flash", "文本")
    function flash(text) {
        pyTip = text
        pyResult = text
        return "qml 收到：" + text
    }

    Rectangle { anchors.fill: parent; color: "#1e1f22" }

    Text {
        x: 12; y: 12
        text: "PyArxQml 面板"
        color: "#6cb6ff"
        font.pixelSize: 18
        font.bold: true
    }
    Text {
        x: 12; y: 42
        text: "QML ↔ C++ backend ↔ Python 双向"
        color: "#9aa0a6"
        font.pixelSize: 12
    }
    Rectangle { x: 12; y: 68; width: parent.width - 24; height: 1; color: "#3a3d42" }

    // ── 方向一：QML 主动调 Python ──
    Rectangle {
        id: button
        x: 12; y: 88
        width: parent.width - 24
        height: 44
        radius: 6
        color: mouse.pressed ? "#3c6fa8" : "#2f6390"
        border.color: "#6cb6ff"
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: "点我：1234 + 5678（Python 算）"
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

    Rectangle {
        x: 12; y: 142
        width: parent.width - 24
        height: 86
        radius: 6
        color: "#26282c"

        Column {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8
            Text { text: "Python 计算结果（backend.result）"; color: "#9aa0a6"; font.pixelSize: 12 }
            Text {
                objectName: "resultText"
                text: backend.result
                color: "#4ec97c"
                font.pixelSize: 20
                font.bold: true
            }
        }
    }

    Rectangle {
        x: 12; y: 238
        width: parent.width - 24
        height: 54
        radius: 6
        color: "#26282c"

        Text {
            objectName: "statusText"
            anchors.fill: parent
            anchors.margins: 12
            text: backend.status
            color: "#e3e5e8"
            font.pixelSize: 12
            wrapMode: Text.WordWrap
        }
    }

    // ── 方向二：Python 主动写 QML ──
    Rectangle {
        x: 12; y: 302
        width: parent.width - 24
        height: 110
        radius: 6
        color: "#26282c"
        border.color: "#46506b"
        border.width: 1

        Column {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 6
            Text { text: "Python 写入 QML（set_prop 的 pyResult）"; color: "#9aa0a6"; font.pixelSize: 12 }
            Text {
                objectName: "pyText"
                text: root.pyResult
                color: root.pyColor
                font.pixelSize: 20
                font.bold: true
            }
            Text {
                text: root.pyTip
                color: "#9aa0a6"
                font.pixelSize: 11
                width: parent.width
                elide: Text.ElideRight
            }
        }
    }

    Text {
        x: 12; y: 424
        text: "尺寸 " + Math.round(root.width) + "×" + Math.round(root.height)
        color: "#9aa0a6"
        font.pixelSize: 12
    }
}
