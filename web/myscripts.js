// Stores the original X sizes of the graphs
// used when zooming out fully
origSizes = new Array();
gs = new Array();
var blockRedraw = false;

//Draw all graphs
function drawAllGraphs()
{
    for (var i = 0; i < myData.length; i++) {
        gs.push(drawOneGraph(i));
    }
}

//Draw one of the graphs
function drawOneGraph(i)
{
    graph = new Dygraph(
        document.getElementById(myData[i].div),
        myData[i].data
        , {
            stackedGraph: myData[i].stacked,
            labels: myData[i].labels,
            underlayCallback: function(canvas, area, g) {
                canvas.fillStyle = "rgba(105, 105, 185, 185)";
                //Draw simplification points
                //colnum = myData[i].colnum;
                colnum = 0;

                for(var k = 0; k < simplificationPoints[colnum].length-1; k++) {
                    var bottom_left = g.toDomCoords(simplificationPoints[colnum][k], -20);
                    var left = bottom_left[0];
                    canvas.fillRect(left, area.y, 2, area.h);
                }
            },
            axes: {
              x: {
                valueFormatter: function(d) {
                  return 'Conflicts: ' + d;
                },
                pixelsPerLabel: 100,
                includeZero: true
              }
            },
            //stepPlot: true,
            //strokePattern: [0.1, 0, 0, 0.5],
            strokeWidth: 0.3,
            highlightCircleSize: 3,
            rollPeriod: 1,
            drawXAxis: false,
            legend: 'always',
            xlabel: false,
            labelsDiv: document.getElementById(myData[i].labeldiv),
            labelsSeparateLines: true,
            labelsKMB: true,
            drawPoints: true,
            pointSize: 1,
            drawXGrid: false,
            drawYGrid: false,
            drawYAxis: false,
            strokeStyle: "black",
            colors: ['#000000', '#05fa03', '#d03332', '#4e4ea8', '#689696'],
            fillAlpha: 0.8,
            errorBars: myData[i].noisy,
            drawCallback: function(me, initial) {

                //Fill original sizes, so if we zoom out, we know where to
                //zoom out
                if (initial)
                    origSizes[myData[i].colnum] = me.xAxisRange();

                //Initial draw, ignore
                if (blockRedraw || initial)
                    return;

                blockRedraw = true;
                var xrange = me.xAxisRange();

                //Is this full reset?
                fullreset = false;
                for (var j = 0; j < myData.length; j++) {
                    if (gs[j] == me) {
                        if (origSizes[myData[j].colnum][0] == xrange[0]
                            &&origSizes[myData[j].colnum][1] == xrange[1]
                        ) {
                            fullreset = true;
                        }
                    }
                }

                /*
                //Must zoom the clause distribution as well
                if (fullreset) {
                    drawPattern(clauseDistrib[0], 0, origSizes[0][0], origSizes[0][1]);
                    drawPattern(clauseDistrib[1], 1, origSizes[1][0], origSizes[1][1]);
                } else {
                    drawPattern(clauseDistrib[0], 0, xrange[0], xrange[1]);
                    drawPattern(clauseDistrib[1], 1, xrange[0], xrange[1]);
                }*/

                //Zoom every one the same way
                for (var j = 0; j < myData.length; j++) {
                    //Don't go into loop
                    if (gs[j] == me)
                        continue;

                    //If this is a full reset, then zoom out maximally
                    if (fullreset) {
                        gs[j].updateOptions( {
                            dateWindow: origSizes[myData[j].colnum]
                        } );
                    } else {
                        gs[j].updateOptions( {
                            dateWindow: xrange
                        } );
                    }
                }

                blockRedraw = false;
            }
        }
    )

    return graph;
}

function DrawClauseDistrib(_data, _divID, _simpPoints)
{
    var data = _data;
    var divID = _divID;
    var simpPoints = _simpPoints;
    var width = 415;
    var height = 100;

    //For SVG pattern, a rectangle
    function drawDistribBox(x1, x2, y1, y2, relHeight, imgData)
    {
        num = 255-relHeight*255.0;
        type = "rgb(" + Math.floor(num) + "," + Math.floor(num) + "," + Math.floor(num) + ")";
        imgData.fillStyle = type;
        imgData.strokeStyle = type;
        imgData.fillRect(x1, y1, x2-x1, y2-y1);
    }

    function drawSimpLine(x1, x2, y1, y2, imgData)
    {
        imgData.strokeStyle = "rgba(105, 105, 185, 185)";
        imgData.fillStyle = "rgba(105, 105, 185, 185)";
        imgData.strokeRect(x1, y1, (x2-x1), (y2-y1));
    }

    function calcNumElemsVertical(from, to)
    {
        //Calculate highest point for this range
        numElementsVertical = 0;
        for(var i = 0 ; i < data.length ; i ++ ){
            //out of range, ignore
            if (data[i].conflEnd < from) {
                continue;
            }
            if (data[i].conflStart > to) {
                break;
            }

            //Check which is the highest
            for(i2 = data[i].darkness.length-1; i2 >= 0 ; i2--) {
                if (data[i].darkness[i2] > 20) {
                    numElementsVertical = Math.max(numElementsVertical, i2);
                    break;
                }
            }
        }

        //alert(from + " " + to + " " + numElementsVertical + " " + i);
        return numElementsVertical;
    }

    this.drawPattern = function(from , to)
    {
        alert("drawing between " + from + " , " + to);
        myDiv = document.getElementById(divID);
        myDiv.style.height = 100;
        myDiv.style.width= 420;
        ctx = myDiv.getContext("2d");
        Xdelta = 0.5;

        onePixelisConf = width/(to-from);
        numElementsVertical = calcNumElemsVertical(from, to);

        //Cut-off lines for Y
        var vAY = new Array();
        for(i = numElementsVertical; i >= 0; i--) {
            vAY.push(Math.round(i*(height/numElementsVertical)));
        }
        vAY.push(0);

        //Start drawing from X origin
        lastXEnd = 0;
        startFound = 0;
        for( i = 0 ; i < data.length ; i ++) {
            if (startFound == 0 && data[i].conflEnd < from)
                continue;

            if (startFound == 1 && data[i].conflStart > to)
                continue;

            //Calculate maximum darkness
            maxDark = 0;
            for(i2 = 0; i2 < data[i].darkness.length; i2++) {
                maxDark = Math.max(maxDark, data[i].darkness[i2]);
            }

            xStart = lastXEnd;

            xEnd = data[i].conflEnd - from;
            xEnd *= onePixelisConf;
            xEnd += Xdelta;
            xEnd = Math.max(0, xEnd);
            xEnd = Math.min(xEnd, width);
            xEnd = Math.round(xEnd);
            lastXEnd = xEnd;

            //Go through each Y component
            for(i2 = 0; i2 < data[i].darkness.length; i2++) {
                yStart = vAY[i2+1];
                yEnd   = vAY[i2];

                //How dark should it be?
                if (data[i].darkness[i2] != 0) {
                    darkness = data[i].darkness[i2]/maxDark;
                } else {
                    darkness = 0;
                }

                //Draw the rectangle
                drawDistribBox(xStart, xEnd, yStart, yEnd, darkness, ctx);
            }
        }

        //Handle simplification lines
        for(var k = 0; k < simpPoints.length-1; k++) {
            var point = simpPoints[k] - from;
            point *= onePixelisConf;
            point += Xdelta;

            //Draw blue line
            if (point > 0) {
                drawSimpLine(point, point+1, 0, height, ctx);
            }
        }
    }
}

drawAllGraphs();
var dists = [];
for(i = 0; i < 2; i++) {
    //alert("drawing now..." + i);
    a = new DrawClauseDistrib(
            clDistrib[i]
            , "MYdrawingPad" + i
            , simplificationPoints[i]
        );
    a.drawPattern(0, maxConflRestart[i]);
    dists.push(a);
}


var settings = {'column-0': columnDivs[0], 'column-1': columnDivs[1]};
var options = { portal : 'columns', editorEnabled : true};
var data = {};
var portal;
Event.observe(window, 'load', function() {
        portal = new Portal(settings, options, data);
});




