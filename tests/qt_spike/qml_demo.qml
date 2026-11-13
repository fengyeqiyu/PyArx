import QtQuick 2.15

// Qt Quick 版状态面板：只用最基础的 QtQuick import（不碰 Controls），
// 用来验证 CAD 自带 Qt6Quick 能不能直接渲染、以及首帧要多久。
Item {
    id: root
    width: 340
    height: 620

    Rectangle { anchors.fill: parent; color: "#1e1f22" }

    Column {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Text {
            text: "MFCWebView 状态"
            color: "#6cb6ff"
            font.pixelSize: 18
            font.bold: true
        }
        Text {
            text: "Qt Quick 渲染 · 运行于 CAD 进程内"
            color: "#9aa0a6"
            font.pixelSize: 12
        }

        Rectangle {
            width: parent.width
            height: 1
            color: "#3a3d42"
        }

        // 五行状态
        Repeater {
            model: [
                { k: "运行时", v: "Qt Quick / QML" },
                { k: "引擎版本", v: "Qt 6.2.6 (CAD 自带)" },
                { k: "渲染后端", v: "RHI + OpenGL" },
                { k: "帧率驱动", v: "宿主泵 processEvents" },
                { k: "页面路径", v: "qml_demo.qml" }
            ]
            delegate: Rectangle {
                width: parent.width
                height: 40
                radius: 6
                color: "#26282c"

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 8
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "●"
                        color: "#4ec97c"
                        font.pixelSize: 14
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.k
                        color: "#9aa0a6"
                        width: 80
                        font.pixelSize: 13
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.v
                        color: "#e3e5e8"
                        font.pixelSize: 13
                    }
                }
            }
        }

        // 动画：证明帧在动（不是一张静态图）
        Rectangle {
            width: parent.width
            height: 6
            radius: 3
            color: "#2b2d31"

            Rectangle {
                id: bar
                height: 6
                radius: 3
                color: "#6cb6ff"
                width: 40
                SequentialAnimation on width {
                    loops: Animation.Infinite
                    NumberAnimation { from: 40; to: root.width - 24; duration: 1600; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: root.width - 24; to: 40; duration: 1600; easing.type: Easing.InOutQuad }
                }
            }
        }

        Text {
            text: "动画运行中 " + Math.round(root.width) + "×" + Math.round(root.height)
            color: "#4ec97c"
            font.pixelSize: 12
        }
    }
}
