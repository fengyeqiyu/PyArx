// PPCombo.qml —— 电压下拉（options 为 [{value,label}]，对齐 HTML 的 select）
import QtQuick 2.15
import QtQuick.Controls 2.15

ComboBox {
    id: root
    property var options: []
    property string value: "380"

    font.pixelSize: 12
    font.family: "Microsoft YaHei"
    padding: 4
    implicitHeight: 26
    flat: false

    model: root.options.map(function (o) { return o.label; })
    currentIndex: {
        for (var i = 0; i < root.options.length; i++) {
            if (root.options[i].value === root.value) return i;
        }
        return 0;
    }

    onActivated: {
        if (index >= 0 && index < root.options.length)
            root.value = root.options[index].value;
    }

    contentItem: Text {
        leftPadding: 6
        text: root.displayText
        font: root.font
        color: "#1F2A37"
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: 3
        border.width: 1
        border.color: root.activeFocus ? "#2F6FBA" : "#D0D5DD"
        color: "#FFFFFF"
    }

    indicator: Text {
        x: root.width - width - 8
        y: root.topPadding + (root.availableHeight - height) / 2
        text: "▾"
        font.pixelSize: 11
        color: "#667085"
    }

    popup: Popup {
        y: root.height
        width: root.width
        padding: 1
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator { }
        }
        background: Rectangle {
            border.width: 1
            border.color: "#D0D5DD"
            color: "#FFFFFF"
        }
    }

    delegate: ItemDelegate {
        width: root.width
        height: 26
        contentItem: Text {
            text: modelData
            font: root.font
            color: "#1F2A37"
            verticalAlignment: Text.AlignVCenter
            leftPadding: 4
        }
        highlighted: root.highlightedIndex === index
    }
}
