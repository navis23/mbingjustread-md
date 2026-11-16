import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: win
    width: 1000
    height: 820
    minimumWidth: 480
    minimumHeight: 360
    visible: true
    title: backend.fileName ? backend.fileName + " — mbingjustread-md" : "mbingjustread-md"
    color: theme.background

    Material.theme: theme.dark ? Material.Dark : Material.Light
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    property bool helpVisible: false
    property bool outlineVisible: false
    property bool searchVisible: false
    property int currentHeading: -1
    // Letter keys belong to the search field while it is being typed in.
    readonly property bool typing: searchField.activeFocus
    readonly property real maxScroll: Math.max(0, flick.contentHeight - flick.height)

    function scrollTo(y) { flick.contentY = Math.max(0, Math.min(y, maxScroll)) }
    function scrollBy(dy) { scrollTo(flick.contentY + dy) }
    function showPosition(position, margin) {
        scrollTo(page.y + page.positionToRectangle(position).y - margin)
        // A jump marks the heading it went to, even one near the end of the
        // file that can never scroll up to the reading line.
        if (outlineVisible)
            currentHeading = backend.headingAt(position)
    }
    // The character at the top of the view, which is how the page keeps its
    // place when the same file is rendered again.
    function topPosition() { return page.positionAt(0, Math.max(0, flick.contentY - page.y)) }

    function showDocument(sameFile) {
        var anchor = -1, offset = 0
        if (sameFile && flick.contentY > page.y) {
            anchor = topPosition()
            offset = flick.contentY - page.y - page.positionToRectangle(anchor).y
        }
        var keep = flick.contentY
        page.text = backend.html
        if (!sameFile)
            flick.contentY = 0
        else if (anchor >= 0)
            scrollTo(page.y + page.positionToRectangle(Math.min(anchor, page.length)).y + offset)
        else
            scrollTo(keep)
        backend.attachDocument(page.textDocument)
        updateCurrentHeading()
    }

    // The current section is the one being read: the last heading above a
    // line a quarter of the way down the view. A line at the very top marked
    // the previous section while the next heading was already on screen.
    function updateCurrentHeading() {
        if (outlineVisible)
            currentHeading = backend.headingAt(page.positionAt(0, Math.max(0, flick.contentY - page.y + flick.height / 4)))
    }

    function openSearch() {
        searchVisible = true
        searchField.forceActiveFocus()
        searchField.selectAll()
    }
    function closeSearch() {
        searchVisible = false
        page.deselect()
        flick.forceActiveFocus()
    }

    Component.onCompleted: if (backend.hasDocument) showDocument(false)
    onOutlineVisibleChanged: updateCurrentHeading()

    Connections {
        target: backend
        function onLoaded(sameFile) { win.showDocument(sameFile) }
        function onScrollToPosition(position) { win.showPosition(position, 16) }
        function onMatchSelected(start, end) {
            page.select(start, end)
            var r = page.positionToRectangle(start)
            var y = page.y + r.y
            if (y < flick.contentY + 40 || y + r.height > flick.contentY + flick.height - 40)
                win.scrollTo(y - flick.height / 3)
        }
        function onNotice(text) {
            toast.text = text
            toastTimer.restart()
        }
    }

    Shortcut { sequence: "?"; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: win.helpVisible = !win.helpVisible }
    Shortcut { sequence: "Q"; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: Qt.quit() }
    Shortcut { sequences: ["Space", "PgDown"]; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: win.scrollBy(flick.height * 0.9) }
    Shortcut { sequences: ["Shift+Space", "PgUp"]; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: win.scrollBy(-flick.height * 0.9) }
    Shortcut { sequences: ["J", "Down"]; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: win.scrollBy(60) }
    Shortcut { sequences: ["K", "Up"]; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: win.scrollBy(-60) }
    Shortcut { sequence: "D"; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: win.scrollBy(flick.height / 2) }
    Shortcut { sequence: "U"; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: win.scrollBy(-flick.height / 2) }
    Shortcut { sequences: ["G", "Home"]; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: win.scrollTo(0) }
    Shortcut { sequences: ["Shift+G", "End"]; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: win.scrollTo(win.maxScroll) }
    Shortcut { sequence: "O"; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: backend.openDialog() }
    Shortcut { sequence: "R"; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: backend.reload() }
    Shortcut { sequence: "T"; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: win.outlineVisible = !win.outlineVisible }
    Shortcut { sequence: "/"; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: win.openSearch() }
    Shortcut { sequences: [StandardKey.Find]; context: Qt.ApplicationShortcut; onActivated: win.openSearch() }
    Shortcut { sequence: "N"; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: backend.nextMatch() }
    Shortcut { sequence: "Shift+N"; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: backend.previousMatch() }
    Shortcut { sequences: ["+", "=", StandardKey.ZoomIn]; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: backend.zoomIn() }
    Shortcut { sequences: ["-", StandardKey.ZoomOut]; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: backend.zoomOut() }
    Shortcut { sequences: ["0", "Ctrl+0"]; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: backend.zoomReset() }
    Shortcut { sequences: ["Alt+Left", "Backspace"]; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: backend.back() }
    Shortcut { sequence: "Alt+Right"; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: backend.forward() }
    Shortcut { sequences: [StandardKey.Copy]; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: page.copy() }
    Shortcut { sequences: [StandardKey.SelectAll]; context: Qt.ApplicationShortcut; enabled: !win.typing; onActivated: page.selectAll() }
    Shortcut {
        sequence: "Esc"
        context: Qt.ApplicationShortcut
        onActivated: {
            if (win.helpVisible) win.helpVisible = false
            else if (win.searchVisible) win.closeSearch()
            else if (win.outlineVisible) win.outlineVisible = false
            else page.deselect()
        }
    }

    // Right-to-left languages mirror the window; the Translator sets the
    // application's layout direction.
    Item {
        anchors.fill: parent
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        // The outline: every heading of the document, with the one being
        // read marked.
        Rectangle {
            id: outline
            anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
            width: win.outlineVisible ? Math.min(300, parent.width * 0.4) : 0
            visible: width > 0
            color: theme.surface
            clip: true

            Text {
                anchors.centerIn: parent
                width: parent.width - 32
                visible: outlineList.count === 0
                text: qsTr("No headings")
                color: theme.mutedForeground
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            ListView {
                id: outlineList
                anchors.fill: parent
                anchors.topMargin: 12
                anchors.bottomMargin: 12
                model: backend.outline
                currentIndex: win.currentHeading
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {}

                delegate: Item {
                    id: entry
                    required property var modelData
                    required property int index
                    width: ListView.view.width
                    height: label.implicitHeight + 10

                    Rectangle {
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        anchors.rightMargin: 6
                        radius: Math.min(4, theme.radius)
                        color: theme.background
                        visible: hover.hovered || entry.index === win.currentHeading
                    }
                    Text {
                        id: label
                        anchors.verticalCenter: parent.verticalCenter
                        x: 16 + (entry.modelData.level - 1) * 14
                        width: parent.width - x - 16
                        text: entry.modelData.title
                        textFormat: Text.PlainText
                        elide: Text.ElideRight
                        font.pixelSize: 14
                        font.weight: entry.modelData.level === 1 ? Font.DemiBold : Font.Normal
                        color: entry.index === win.currentHeading ? theme.accent
                             : entry.modelData.level <= 2 ? theme.brightForeground : theme.foreground
                    }
                    HoverHandler { id: hover }
                    TapHandler { onTapped: win.showPosition(entry.modelData.position, 16) }
                }
            }
        }

        // The page. Flicking with the mouse is off so that dragging selects
        // text, as in any reader; the wheel and the keyboard scroll.
        Flickable {
            id: flick
            anchors { left: outline.right; right: parent.right; top: parent.top; bottom: searchBar.top }
            contentWidth: width
            contentHeight: page.y + page.height + 80
            clip: true
            interactive: false
            boundsBehavior: Flickable.StopAtBounds
            focus: true
            onContentYChanged: win.updateCurrentHeading()
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            TextEdit {
                id: page
                x: Math.round((flick.width - width) / 2)
                y: 40
                width: Math.max(200, Math.min(flick.width - 64, 860))
                visible: backend.hasDocument
                readOnly: true
                selectByMouse: true
                persistentSelection: true
                textFormat: TextEdit.RichText
                wrapMode: TextEdit.Wrap
                color: theme.foreground
                // The accent, so a search match stands out on any background.
                selectionColor: theme.accent
                selectedTextColor: theme.accentForeground
                font.pixelSize: backend.fontSize
                baseUrl: backend.baseUrl
                onLinkActivated: (link) => backend.activateLink(link)
                onWidthChanged: backend.contentWidth = width

                HoverHandler {
                    cursorShape: page.hoveredLink ? Qt.PointingHandCursor : Qt.IBeamCursor
                }
            }

            WheelHandler {
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                onWheel: (event) => win.scrollBy(-(event.pixelDelta.y !== 0 ? event.pixelDelta.y : event.angleDelta.y))
            }
        }

        // Shown before any file is open, and when one could not be read.
        Column {
            anchors.centerIn: flick
            width: Math.min(flick.width - 64, 520)
            spacing: 14
            visible: !backend.hasDocument

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: "mbingjustread-md"
                color: theme.brightForeground
                font.pixelSize: 28
                font.weight: Font.DemiBold
            }
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                visible: backend.error === ""
                text: qsTr("Press O to open a Markdown file, or drop one here.")
                color: theme.foreground
                font.pixelSize: 16
            }
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                visible: backend.error !== ""
                text: backend.error
                color: theme.red
                font.pixelSize: 16
            }
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: qsTr("Press ? for all shortcuts.")
                color: theme.mutedForeground
                font.pixelSize: 14
            }
        }

        // How far through the document the view is.
        Text {
            anchors { right: flick.right; bottom: flick.bottom; rightMargin: 22; bottomMargin: 12 }
            visible: backend.hasDocument && win.maxScroll > 0
            text: qsTr("%1%").arg(Math.round(100 * flick.contentY / Math.max(1, win.maxScroll)))
            color: theme.mutedForeground
            font.pixelSize: 12
        }

        Rectangle {
            id: searchBar
            anchors { left: outline.right; right: parent.right; bottom: parent.bottom }
            height: win.searchVisible ? 48 : 0
            visible: height > 0
            color: theme.surface
            clip: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 16

                TextInput {
                    id: searchField
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    clip: true
                    color: theme.brightForeground
                    selectionColor: theme.accent
                    selectedTextColor: theme.accentForeground
                    font.pixelSize: 16
                    onTextEdited: backend.search(text, win.topPosition())
                    Keys.onReturnPressed: (event) => (event.modifiers & Qt.ShiftModifier) ? backend.previousMatch() : backend.nextMatch()
                    Keys.onEnterPressed: backend.nextMatch()
                    Keys.onEscapePressed: win.closeSearch()

                    Text {
                        anchors.fill: parent
                        visible: searchField.text === ""
                        text: qsTr("Find in document")
                        color: theme.mutedForeground
                        font: searchField.font
                        elide: Text.ElideRight
                    }
                }
                Text {
                    Layout.alignment: Qt.AlignVCenter
                    text: searchField.text === "" ? ""
                        : backend.matchCount === 0 ? qsTr("No matches")
                        : qsTr("%1 of %2").arg(backend.currentMatch + 1).arg(backend.matchCount)
                    color: backend.matchCount === 0 && searchField.text !== "" ? theme.red : theme.mutedForeground
                    font.pixelSize: 14
                }
            }
        }

        DropArea {
            anchors.fill: parent
            onDropped: (drop) => {
                if (drop.hasUrls && drop.urls.length > 0)
                    backend.open(drop.urls[0].toString())
            }
        }

        // Brief messages: a link that leads nowhere, a file that went away.
        Rectangle {
            id: toastBox
            anchors { horizontalCenter: flick.horizontalCenter; bottom: flick.bottom; bottomMargin: 28 }
            width: Math.min(toast.implicitWidth + 32, flick.width - 48)
            height: toast.implicitHeight + 20
            radius: theme.radius
            color: theme.surface
            border.color: theme.border
            visible: toastTimer.running

            Text {
                id: toast
                anchors.centerIn: parent
                width: Math.min(implicitWidth, flick.width - 80)
                wrapMode: Text.Wrap
                color: theme.brightForeground
                font.pixelSize: 14
            }
            Timer { id: toastTimer; interval: 4000 }
        }

        // Every shortcut, on ?.
        Rectangle {
            anchors.fill: parent
            visible: win.helpVisible
            color: Qt.rgba(0, 0, 0, 0.6)

            TapHandler { onTapped: win.helpVisible = false }

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(parent.width - 48, 520)
                height: Math.min(parent.height - 48, helpColumn.implicitHeight + 48)
                radius: theme.radius
                color: theme.surface
                border.color: theme.border
                clip: true

                Flickable {
                    anchors.fill: parent
                    anchors.margins: 24
                    contentHeight: helpColumn.implicitHeight
                    boundsBehavior: Flickable.StopAtBounds

                    Column {
                        id: helpColumn
                        width: parent.width
                        spacing: 8

                        Text {
                            text: qsTr("Shortcuts")
                            color: theme.brightForeground
                            font.pixelSize: 18
                            font.weight: Font.DemiBold
                            bottomPadding: 8
                        }
                        Repeater {
                            model: [
                                ["O", qsTr("Open a file")],
                                ["R", qsTr("Reload")],
                                ["T", qsTr("Outline")],
                                ["/  Ctrl+F", qsTr("Find in document")],
                                ["N  Shift+N", qsTr("Next and previous match")],
                                ["Space  Shift+Space", qsTr("Page down and up")],
                                ["J  K", qsTr("Scroll down and up")],
                                ["D  U", qsTr("Half a page down and up")],
                                ["G  Shift+G", qsTr("Top and bottom")],
                                ["+  -  0", qsTr("Zoom in, out, and reset")],
                                ["Alt+←  Alt+→", qsTr("Back and forward")],
                                ["Ctrl+C", qsTr("Copy the selection")],
                                ["?", qsTr("Show or hide this help")],
                                ["Q", qsTr("Quit")]
                            ]
                            delegate: Row {
                                required property var modelData
                                width: helpColumn.width
                                spacing: 16
                                Text {
                                    width: 170
                                    text: parent.modelData[0]
                                    color: theme.accent
                                    font.pixelSize: 14
                                    font.family: "monospace"
                                }
                                Text {
                                    width: parent.width - 186
                                    text: parent.modelData[1]
                                    color: theme.foreground
                                    font.pixelSize: 14
                                    wrapMode: Text.Wrap
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
