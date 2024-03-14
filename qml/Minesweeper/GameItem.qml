import QtQuick 2.15

import Minesweeper.Backend 1.0

Rectangle {
    id: gameItem

    property alias game: mineField.game
    property alias mineFieldItem: mineField

    color: mineField.colBackground2
    clip: true

    property bool platformAndroid:  Qt.platform.os == "android"
    property bool platformMobile: platformAndroid
    property string inputMode: platformMobile ? "mobile" : "desktop"

    property QtObject ui: QtObject {
        id: theUiObject

        property FontMetrics fontMetrics: FontMetrics {
            id: theFontMetrics
            font.pointSize: platformMobile ? 18 : 12
        }

        property alias font: theFontMetrics.font

        function em(n) {
            return Math.floor(theFontMetrics.ascent * n);
        }

        function clamp(v, min, max) {
            return Math.max(min, Math.min(v, max));
        }

        function emInRange(v, min, max) {
            return clamp(em(v), min, max);
        }
    }

    property bool zoomMenuButtonVisible: platformMobile
    property bool zoomMenuVisible: false

    property string timeFormat: "s"

    function formatSecondsElapsed(n) {
        if(timeFormat == "s") {
            return "" + n;
        } else {
            let s = n % 60;
            let m = Math.floor(n / 60);
            return "" + m + ":" + (s < 10 ? "0" + s : "" + s);
        }
    }

    property var mineConfigs: [
        {"size": Qt.size(9,9), "minecount": [10, 35]},
        {"size": Qt.size(16,16), "minecount": [40,99]},
        {"size": Qt.size(30,16), "minecount": [99,170]}
    ]
    property QtObject currentConfig: QtObject {
        property int size: 1
        property int minecount: 0
    }

    function updateGameConfig() {
        let sconf = mineConfigs[currentConfig.size];
        game.changeConfig(sconf.size, sconf.minecount[currentConfig.minecount]);
    }

    Component.onCompleted: {
        updateGameConfig();
    }

    component UiText: Text {
        font: ui.font
    }

    Rectangle {
        id: topBar

        width: parent.width
        property int padding: ui.clamp(ui.em(0.2), 4, 12)
        height: theEmojiIcon.height + 2*padding

        property int spacing: ui.em(0.7)
        property int innerSpacing: ui.em(0.5)

        color: mineField.colBackground2

        Item {
            id: topBarContent
            anchors.fill: parent
            anchors.margins: parent.padding

            Item {
                id: topBarLeft

                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: theEmojiIcon.left
                anchors.rightMargin: topBar.spacing

                Row {
                    spacing: topBar.innerSpacing

                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter

                    UiText {
                        text: "💣" + (game.mineCount - game.flagCount)

                        TapHandler {
                            enabled: game.gameState == QMinesweeperGame.NotStarted
                            onTapped: {
                                let conf = mineConfigs[currentConfig.size];
                                currentConfig.minecount = (currentConfig.minecount + 1) % conf.minecount.length;
                                updateGameConfig();
                            }
                        }
                    }

                    UiText {
                        text: `⬜ ${game.gridSize.width}x${game.gridSize.height}`

                        visible: game.gameState == QMinesweeperGame.NotStarted

                        TapHandler {
                            onTapped: {
                                currentConfig.minecount = 0;
                                currentConfig.size = (currentConfig.size + 1) % mineConfigs.length;
                                updateGameConfig();
                            }
                        }
                    }

                    UiText {
                        property int toUncover: game.gridSize.width * game.gridSize.height - game.mineCount - game.uncoveredCount
                        visible: game.gameState != QMinesweeperGame.NotStarted && toUncover <= 10

                        text: "🏁" + toUncover
                    }
                }
            }

            UiText {
                id: theEmojiIcon
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                text: game.gameState == QMinesweeperGame.Lost ? "😵‍💫" : "😊"

                TapHandler {
                    onTapped: {
                        game.restart();
                    }
                }
            }

            Item {
                id: topBarRight

                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: theEmojiIcon.right
                anchors.right: parent.right
                anchors.leftMargin: topBar.spacing

                Row {
                    spacing: topBar.innerSpacing
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter

                    UiText {
                        text: "🤖"
                        anchors.verticalCenter: parent.verticalCenter
                        visible: zoomMenuVisible

                        TapHandler {
                            onTapped: {
                                mineField.fitToScreen();
                            }
                        }
                    }

                    UiText {
                        text: "➖"
                        anchors.verticalCenter: parent.verticalCenter
                        visible: zoomMenuVisible

                        TapHandler {
                            onTapped: {
                                mineField.zoomOut();
                            }
                        }
                    }

                    UiText {
                        text: "➕"
                        anchors.verticalCenter: parent.verticalCenter
                        visible: zoomMenuVisible

                        TapHandler {
                            onTapped: {
                                mineField.zoomIn();
                            }
                        }
                    }

                    UiText {
                        id: zoomButton
                        text: "🔎"
                        visible: zoomMenuButtonVisible
                        anchors.verticalCenter: parent.verticalCenter

                        TapHandler {
                            onTapped: {
                                zoomMenuVisible = !zoomMenuVisible;
                            }
                        }
                    }

                    UiText {
                        visible: game.gameState != QMinesweeperGame.NotStarted
                        text: "⏱️" + formatSecondsElapsed(game.secondsElapsed)

                        anchors.verticalCenter: parent.verticalCenter

                        TapHandler {
                            onTapped: {
                                if(timeFormat == "s") {
                                    timeFormat = "mm:ss";
                                } else {
                                    timeFormat = "s";
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    MineField {
        id: mineField
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: topBar.bottom
    }
}
