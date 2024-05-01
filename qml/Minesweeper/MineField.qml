import QtQuick 2.15

import Minesweeper.Backend 1.0

import "minesweeper.js" as MinesweeperJS

Rectangle {
    id: mineField

    property QMinesweeperGame game: QMinesweeperGame {

    }

    property color colBackground: "#f0f0f0"
    property color colBackground2: Qt.rgba(colBackground.r * 19.0 / 20, colBackground.g * 19.0 / 20, colBackground.b * 19.0 / 20, 1)

    property var digitsColors: [
        Qt.rgba(0, 0, 0, 0),
        Qt.rgba(0, 0, 1, 1), // 1
        Qt.rgba(0, 0.5, 0, 1), // 2
        Qt.rgba(1, 0, 0, 1), // 3
        Qt.rgba(0, 0, 0.5, 1), // 4
        Qt.rgba(0.5, 0, 0, 1), // 5
        Qt.rgba(0, 0.5, 0.5, 1), // 6
        Qt.rgba(0, 0, 0, 1), // 7
        Qt.rgba(0.5, 0.5, 0.5, 1)  // 8
    ]

    property color colMine: Qt.rgba(0, 0, 0, 1)
    property color colBang: Qt.rgba(1, 0, 0, 1)
    property color colCross: Qt.rgba(1, 0, 0, 1)
    property color colFlag: Qt.rgba(1, 0, 0, 1)
    property color colFlagBase: Qt.rgba(0, 0, 0, 1)
    property color colHighlight: Qt.rgba(1, 1, 1, 1)
    property color colLowLight: Qt.rgba(colBackground.r * 2.0 / 3, colBackground.g * 2.0 / 3, colBackground.b * 2.0 / 3, 1)
    property color colWrongNumber: Qt.rgba(1, 0.6, 0.6, 1)

    property int minimumTileSize: Qt.platform.os === "android" ? 80 : 20
    property int tileSize: minimumTileSize

    onTileSizeChanged: {
        frame.requestPaint();
        contentCanvas.requestPaint();
    }

    onMinimumTileSizeChanged: {
        if (tileSize < minimumTileSize) {
            tileSize = minimumTileSize;
        }
    }

    function setTileSize(ts) {
        tileSize = Math.max(Math.floor(ts), minimumTileSize);
        scalePlaceholder.scale = tileSize / minimumTileSize;
    }

    property real zoomFactor: 1.2

    function zoomIn() {
        setTileSize(tileSize * zoomFactor);
    }

    function zoomOut() {
        setTileSize(tileSize / zoomFactor);
    }

    function fitToScreen() {
        let bbs = 2 * borderBaseSize;
        let tsw = (mineField.width - bbs) / (game.gridSize.width + 2*borderExtraTileSizeRatio + 2*outerHighlightWidthTileSizeRatio);
        let tsh = (mineField.height - bbs) / (game.gridSize.height + 2*borderExtraTileSizeRatio + 2*outerHighlightWidthTileSizeRatio);
        let ts = Math.min(tsw, tsh);
        setTileSize(ts);
    }

    property bool autoFit: Qt.platform.os === "android" ? false : true

    property int borderBaseSize: Qt.platform.os === "android" ? minimumTileSize : 0
    property real borderExtraTileSizeRatio: Qt.platform.os === "android" ? 0.1 : 3/2
    property int borderExtra: Math.floor(tileSize * borderExtraTileSizeRatio)
    property int border: borderBaseSize + borderExtra;
    property real outerHighlightWidthTileSizeRatio: 3/20
    property int outerHighlightWidth: Math.max(Math.floor(outerHighlightWidthTileSizeRatio * tileSize), 1)

    property real flashFrame: 0.13

    /*
     * Private parts begin here
     */

    color: colBackground

    property int highlightWidth: Math.max(Math.floor(tileSize / 10), 1)

    property size naturalSize: Qt.size(gridSize.width * mineField.tileSize + 2 * outerHighlightWidth, gridSize.height * mineField.tileSize + 2 * outerHighlightWidth)
    property size maximumSize: Qt.size(mineField.width - 2*border, mineField.height - 2*border)

    property size gridSize: game.gridSize
    property var gameMines: game.mines
    property var gameGrid: game.grid
    property int gameState: game.gameState

    property size mysize: Qt.size(width, height)
    onMysizeChanged: {
        if (autoFit) {
            fitToScreen();
        }
    }
    onGridSizeChanged: {
        if (autoFit) {
            fitToScreen();
        }
    }

    Timer {
        id: flashTimer
        running: false
        interval: 130
        repeat: true

        property int frameNumber: 0
        property int nbFrames: 0

        onTriggered: {
            frameNumber = frameNumber + 1;

            if (frameNumber > nbFrames) {
                frameNumber = 0;
                stop();
            }

            contentCanvas.requestPaint();
        }
    }

    function beginFlash(flash_is_death) {
        flashTimer.nbFrames = flash_is_death ? 3 : 2;
        flashTimer.frameNumber = 0;
        flashTimer.start();
    }

    onGameStateChanged: {
        if (gameState == QMinesweeperGame.Won || gameState == QMinesweeperGame.Lost) {
            beginFlash();
        }
    }

    function pos2index(x, y) {
        return y * gridSize.width + x;
    }

    onGameGridChanged: {
        contentCanvas.requestPaint();
    }

    property int hx: -1
    property int hy: -1
    property int hradius: 0

    signal cursorHighlightChanged

    function updateCursorHighlight(x, y, r) {
        hx = x;
        hy = y;
        hradius = r;
        cursorHighlightChanged();
    }

    onCursorHighlightChanged: {
        contentCanvas.requestPaint();
    }

    Canvas {
        id: frame

        anchors.centerIn: parent
        width: Math.min(naturalSize.width, maximumSize.width)
        height: Math.min(naturalSize.height, maximumSize.height)

        onPaint: {
            let ctx = frame.getContext("2d");
            MinesweeperJS.drawMinefieldFrame(ctx, {
                                        "tileSize": mineField.tileSize,
                                        "outerHighlightWidth": outerHighlightWidth,
                                        "colHighlight": colHighlight,
                                        "colLowLight": colLowLight,
                                    });
        }

        Item {
            id: content

            anchors.fill: parent
            anchors.margins: outerHighlightWidth

            clip: true

            property int contentWidth: contentCanvas.width
            property int contentHeight: contentCanvas.height
            property int contentOverflowX: contentWidth - width
            property int contentOverflowY: contentHeight - height
            property bool contentFitsX: contentOverflowX <= 0
            property bool contentFitsY: contentOverflowY <= 0
            property bool contentFits: contentFitsX && contentFitsY

            onContentFitsChanged: {
                if(contentFits) {
                    contentCanvas.x = 0;
                    contentCanvas.y = 0;
                }
            }

            onContentOverflowXChanged: {
                if (contentCanvas.x < -contentOverflowX) {
                    contentCanvas.x = -contentOverflowX;
                } else if(contentCanvas.x > 0) {
                    contentCanvas.x = 0;
                }
            }

            onContentOverflowYChanged: {
                if (contentCanvas.y < -contentOverflowY) {
                    contentCanvas.y = -contentOverflowY;
                } else if(contentCanvas.y > 0) {
                    contentCanvas.y = 0;
                }
            }

            Canvas {
                id: contentCanvas

                width: gridSize.width * mineField.tileSize
                height: gridSize.height * mineField.tileSize

                FontMetrics {
                    id: fontMetrics
                    font.family: "sans-serif"
                    font.pixelSize: Math.floor(7*mineField.tileSize/8)
                }

                onPaint: {
                    let gameData = {
                        "dead": gameState == QMinesweeperGame.Lost,
                        "grid": gameGrid,
                        "mines": gameMines,
                        "gridSize": gridSize
                    };

                    let options = {
                        "tileSize": mineField.tileSize,
                        "highlightWidth": highlightWidth,
                        "outerHighlightWidth": outerHighlightWidth,
                        "colBackground": colBackground,
                        "colHighlight": colHighlight,
                        "colLowLight": colLowLight,
                        "colFlagBase": colFlagBase,
                        "colFlag": colFlag,
                        "colWrongNumber": colWrongNumber,
                        "colBang": colBang,
                        "colBackground2": colBackground2,
                        "colMine": colMine,
                        "colCross": colCross,
                        "digitsColors": digitsColors,
                        "font": {
                            "pixelSize": fontMetrics.font.pixelSize,
                            "family": fontMetrics.font.family,
                            "ascent": fontMetrics.ascent
                        },
                        "flash": flashTimer.running,
                        "flashFrameNumber": flashTimer.frameNumber
                    };

                    let ctx = contentCanvas.getContext("2d");
                    MinesweeperJS.drawMinefield(ctx, gameData, options);
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: ui.clamp(ui.em(0.5), 4, 12)
                color: "black"
                opacity: 0.5
                visible: contentCanvas.x < 0
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: ui.clamp(ui.em(0.5), 4, 12)
                color: "black"
                opacity: 0.5
                visible: contentCanvas.y < 0
            }

            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: ui.clamp(ui.em(0.5), 4, 12)
                color: "black"
                opacity: 0.5
                visible: contentCanvas.x > -content.contentOverflowX
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: ui.clamp(ui.em(0.5), 4, 12)
                color: "black"
                opacity: 0.5
                visible: contentCanvas.y > -content.contentOverflowY
            }

            function getTileX(pos) {
                let x = pos.x;
                return Math.floor((x - contentCanvas.x) / tileSize);
            }

            function getTileY(pos) {
                let y = pos.y;
                return Math.floor((y - contentCanvas.y) / tileSize);
            }

            TapHandler {
                acceptedButtons: Qt.MiddleButton
                acceptedPointerTypes: PointerDevice.GenericPointer

                enabled: !game.finished

                onTapped: (eventPoint) => {
                    let x = content.getTileX(eventPoint.position);
                    let y = content.getTileY(eventPoint.position);
                    let v = gameGrid[pos2index(x,y)];
                    console.log(`tap middle (${x}, ${y}, ${v})`);

                    if(v >= 1)
                        game.openNeighbors(x,y);
                }
            }

            TapHandler {
                acceptedButtons: Qt.RightButton
                acceptedPointerTypes: PointerDevice.GenericPointer

                enabled: !game.finished

                onTapped: (eventPoint) => {
                    let x = content.getTileX(eventPoint.position);
                    let y = content.getTileY(eventPoint.position);
                    let v = gameGrid[pos2index(x,y)];
                    console.log(`tap right (${x}, ${y}, ${v})`);

                    if (v === -2 || v === -1)
                        game.toggleMark(x,y);
                }
            }

            PointHandler{
                acceptedButtons: Qt.LeftButton | Qt.MiddleButton

                enabled: !game.finished

                onGrabChanged: (transition, point) => {
                    /*
                    qml: 16: GrabExclusive
                    qml: 32: UngrabExclusive
                    qml: 48: CancelGrabExclusive
                    qml: 1: GrabPassive
                    qml: 2: UngrabPassive
                    qml: 3: CancelGrabPassive
                    */

                    if(transition == EventPoint.GrabPassive) {
                        let x = content.getTileX(point.position);
                        let y = content.getTileY(point.position);
                        let v = gameGrid[pos2index(x,y)];
                        if(gameItem.inputMode === "desktop" || v >= 0 || gameState == QMinesweeperGame.NotStarted)
                            updateCursorHighlight(x, y, v >= 0 ? 1 : 0);
                    }
                }

                onActiveChanged: {
                    if(!active) {
                        updateCursorHighlight(-1, -1, 0);
                    }
                }
            }

            TapHandler {

                longPressThreshold: 0.4

                enabled: !game.finished && gameItem.inputMode === "mobile"

                onTapped:  (eventPoint) => {
                    let x = content.getTileX(eventPoint.position);
                    let y = content.getTileY(eventPoint.position);
                    let v = gameGrid[pos2index(x, y)];

                    console.log(`tap (${x}, ${y}, ${v})`);

                    if (v === -2 || v === -1)
                        game.toggleMark(x, y);
                    else if(v >= 1)
                        game.openNeighbors(x, y);

                }

                onLongPressed: {
                    console.log("long pressed");
                    let x = content.getTileX(point.position);
                    let y = content.getTileY(point.position);

                    let v = gameGrid[pos2index(x, y)];
                    game.open(x, y);
                }
            }

            TapHandler {

                longPressThreshold: 0.4

                enabled: !game.finished && gameItem.inputMode === "desktop"

                onTapped:  (eventPoint) => {
                    let x = content.getTileX(eventPoint.position);
                    let y = content.getTileY(eventPoint.position);
                    let v = gameGrid[pos2index(x, y)];

                    console.log(`tap (${x}, ${y}, ${v})`);

                    if (v == -2) {
                        game.open(x, y);
                    } else if(v >= 1) {
                        game.openNeighbors(x, y);
                    }
                }
            }


            DragHandler {
                enabled: !content.contentFits

                target: contentCanvas
                xAxis.enabled: !content.contentFitsX
                xAxis.minimum: -content.contentOverflowX
                xAxis.maximum: 0

                yAxis.enabled: !content.contentFitsY
                yAxis.minimum: -content.contentOverflowY
                yAxis.maximum: 0

                onActiveChanged: {
                    if(active) {
                        console.log("starting drag")
                    }
                }
            }

            Item {
                id: scalePlaceholder
                width:1
                height:1

                onScaleChanged: {
                    tileSize = Math.floor(minimumTileSize * scale);
                }
            }

            PinchHandler {
                target: scalePlaceholder
                minimumScale: 1
                maximumScale: 4
            }
        }
    }
}
