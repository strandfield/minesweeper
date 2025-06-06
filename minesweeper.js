
// Javascript code for drawing the minesweeper grid in a canvas.
// Adapted from the game "Mines" from Simon Tatham's Puzzle Collection
// @ https://www.chiark.greenend.org.uk/~sgtatham/puzzles/
// Distributed under the MIT license (same as upstream).

function drawMinefieldFrame(ctx, options) {

    let ts = options.tileSize;
    let outerHighlightWidth = options.outerHighlightWidth;
    let colHighlight = options.colHighlight;
    let colLowLight = options.colLowLight;

    let canvas = ctx.canvas;
    let w = options.rect ? options.rect.width : canvas.width;
    let h = options.rect ? options.rect.height : canvas.height;
    let dx = options.rect ? options.rect.x : 0;
    let dy = options.rect ? options.rect.y : 0;

    ctx.fillStyle = colHighlight;
    ctx.beginPath();
    ctx.moveTo(dx + w, dy + h);
    ctx.lineTo(dx + w, dy + 0);
    ctx.lineTo(dx + w - outerHighlightWidth - ts, dy + outerHighlightWidth + ts);
    ctx.lineTo(dx + outerHighlightWidth + ts,  dy + h - outerHighlightWidth - ts);
    ctx.lineTo(dx + 0, dy + h);
    ctx.closePath();
    ctx.fill();

    ctx.fillStyle = colLowLight;
    ctx.beginPath();
    ctx.moveTo(dx + 0, dy + 0);
    ctx.lineTo(dx + w, dy + 0);
    ctx.lineTo(dx + w - outerHighlightWidth - ts, dy + outerHighlightWidth + ts);
    ctx.lineTo(dx + outerHighlightWidth + ts,  dy + h - outerHighlightWidth - ts);
    ctx.lineTo(dx + 0, dy + h);
    ctx.closePath();
    ctx.fill();
}

function drawMinefieldTile(ctx, x, y, ts, v, bg, options) {
    let highlightWidth = options.highlightWidth;
    let colBackground = options.colBackground;
    let colHighlight = options.colHighlight;
    let colLowLight = options.colLowLight;
    let colFlagBase = options.colFlagBase;
    let colFlag = options.colFlag;
    let colWrongNumber = options.colWrongNumber;
    let colBang = options.colBang;
    let colBackground2 = options.colBackground2;
    let colMine = options.colMine;
    let digitsColors = options.digitsColors;
    let colCross = options.colCross;
    let font = options.font;

    if (v < 0)
    {
        if (v === -22 || v === -23) {
            v += 20;

            ctx.fillStyle = colBackground2;
            ctx.fillRect(x, y, ts, ts);

            ctx.strokeStyle = colLowLight;
            ctx.beginPath();
            ctx.moveTo(x + ts - 1, y);
            ctx.lineTo(x, y);
            ctx.lineTo(x, y + ts - 1);
            ctx.stroke();
        } else {
            ctx.fillStyle = colLowLight;
            ctx.beginPath();
            ctx.moveTo(x + ts, y + ts);
            ctx.lineTo(x + ts, y);
            ctx.lineTo(x, y + ts);
            ctx.closePath();
            ctx.fill();

            ctx.fillStyle = colHighlight;
            ctx.beginPath();
            ctx.moveTo(x, y);
            ctx.lineTo(x + ts, y);
            ctx.lineTo(x, y + ts);
            ctx.closePath();
            ctx.fill();

            ctx.fillStyle = colBackground;
            ctx.fillRect(x + highlightWidth, y + highlightWidth, ts - 2 * highlightWidth, ts - 2 * highlightWidth);
        }

        if (v === -1) { // draw flag

            ctx.fillStyle = colFlagBase;
            ctx.beginPath();
            ctx.moveTo(x + Math.floor(0.6 * ts), y + Math.floor(0.35 * ts));
            ctx.lineTo(x + Math.floor(0.6 * ts), y + Math.floor(0.7 * ts));
            ctx.lineTo(x + Math.floor(0.8 * ts), y + Math.floor(0.8 * ts));
            ctx.lineTo(x + Math.floor(0.25 * ts), y + Math.floor(0.8 * ts));
            ctx.lineTo(x + Math.floor(0.55 * ts), y + Math.floor(0.7 * ts));
            ctx.lineTo(x + Math.floor(0.55 * ts), y + Math.floor(0.35 * ts));
            ctx.closePath();
            ctx.fill();

            ctx.fillStyle = colFlag;
            ctx.beginPath();
            ctx.moveTo(x + Math.floor(0.6 * ts), y + Math.floor(0.2 * ts));
            ctx.lineTo(x + Math.floor(0.6 * ts), y + Math.floor(0.5 * ts));
            ctx.lineTo(x + Math.floor(0.2 * ts), y + Math.floor(0.35 * ts));
            ctx.closePath();
            ctx.fill();
        }
    }
    else
    {
        if(v & 32) {
            bg = colWrongNumber;
            v -= 32;
        }

        if (v == 65) {
            ctx.fillStyle = colBang;
        } else {
            ctx.fillStyle = bg == colBackground ? colBackground2 : bg;
        }
        ctx.fillRect(x, y, ts, ts);

        ctx.strokeStyle = colLowLight;
        ctx.beginPath();
        ctx.moveTo(x + ts - 1, y);
        ctx.lineTo(x, y);
        ctx.lineTo(x, y + ts - 1);
        ctx.stroke();

        if (v > 0 && v <= 8) {
            ctx.textAlign = "center";
            ctx.fillStyle = digitsColors[v];
            ctx.font = `${font.pixelSize}px ${font.family}`;
            //ctx.fillText("" + v, Math.round(x + ts / 2), Math.round(y + ts / 2));
            ctx.fillText("" + v, x+ts/2, Math.floor(y + (ts+font.ascent)/2));
        }
        else if (v >= 64) { // draw a mine

            let cx = Math.floor(x + ts/2);
            let cy = Math.floor(y + ts/2);
            let r = Math.floor(ts / 2 - 3);
            let radius = Math.floor(5 * r / 6);
            ctx.fillStyle = colMine;
            ctx.beginPath();
            if (options.qmlCanvas) {
                ctx.ellipse(cx - radius, cy - radius, 2*radius, 2*radius);
            } else {
                ctx.ellipse(cx, cy, radius, radius, 0, 0, 2 * Math.PI);
            }
            ctx.fill();
            ctx.fillRect(cx - r / 6, cy - r, 2 * (r / 6) + 1, 2*r + 1);
            ctx.fillRect(cx - r, cy - r / 6, 2*r + 1, 2 * (r / 6) + 1);
            ctx.fillStyle = colHighlight;
            ctx.fillRect(cx - r/3, cy - r/3, r/3, r/4);

            if (v == 66) {  // Cross through the mine.
                ctx.strokeStyle = colCross;
                ctx.lineWidth = 3;
                ctx.beginPath();
                ctx.moveTo(x + 3, y + 2);
                ctx.lineTo(x + ts - 3, y + ts - 2);
                ctx.moveTo(x + ts - 3, y + 2);
                ctx.lineTo(x + 3, y + ts - 2);
                ctx.stroke();
                ctx.lineWidth = 1;
            }
        }
    }
}

function drawMinefield(ctx, gameData, options)  {
    let ts = options.tileSize;
    let dead = gameData.dead;
    let gameGrid = gameData.grid;
    let colHighlight = options.colHighlight;
    let colLowLight = options.colLowLight;
    let colBackground = options.colBackground;
    let colBang = options.colBang;
    let flash = options.flash;
    let flashFrameNumber = options.flashFrameNumber;

    let gsh = gameData.gridSize.height, gsw = gameData.gridSize.width;

    let bg = colBackground;

    let dx = options.rect ? options.rect.x : 0;
    let dy = options.rect ? options.rect.y : 0;

    if (flash) {
        if (flashFrameNumber % 2)
            bg = (dead ? colBackground : colLowLight);
        else
            bg = (dead ? colBang : colHighlight);
    }

    for(let y = 0; y < gsh; y++) {
        for(let x = 0; x < gsw; x++) {
            let sqi = gsw * y + x;
            let v = gameGrid[sqi];

            if (v >= 0 && v <= 8){
                let flags = 0;

                for(let dy = -1; dy <= 1; dy++) {
                    for(let dx = -1; dx <= 1; dx++) {
                        let nx = x + dx, ny = y + dy;
                        if (nx >= 0 && nx < gsw && ny >= 0 && ny < gsh) {
                            if (gameGrid[gsw * ny + nx] === -1)
                                flags++;
                        }
                    }
                }

                if (flags > v)
                    v |= 32;
            }

            if (options.cursorHighlight) {
                let hx = options.cursorHighlight.x;
                let hy = options.cursorHighlight.y;
                let hradius = options.cursorHighlight.radius;
                if (v === -2 && Math.abs(x - hx) <= hradius && Math.abs(y - hy) <= hradius) {
                    v -= 20;
                }
            }

            // TODO: do not redraw the tile if nothing has changed
            drawMinefieldTile(ctx, dx + x*ts, dy + y*ts, ts, v, bg, options);
        }
    }
}

