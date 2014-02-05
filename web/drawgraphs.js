// Stores the original X sizes of the graphs
// used when zooming out fully

//for graphs
var origSizes = new Array();
var blockRedraw = false;
var graphs = new Array();

//For distibutions
var dists = [];

//For portal
var portal;

function setRollPeriod(num)
{
    for (var column = 0; column < graph_data.length; column++) {
        for (var j = 0; j < graph_data[column].length; j++) {
            graphs[column][j].updateOptions( {
                rollPeriod: num
            } );
        }
    }
}

//Draws all graphs using dygraphs
function drawAllGraphs()
{
    graphs = new Array();
    for (var column = 0; column < graph_data.length; column++) {
        graphs.push(new Array());
        for (var i = 0; i < graph_data[column].length; i++) {
            graphs[column].push(drawOneGraph(column, i));
        }
    }
}

//Draw one of the graphs
function drawOneGraph(column, i)
{
    graph = new Dygraph(
        document.getElementById(graph_data[column][i].dataDivID),
        graph_data[column][i].data
        , {
            stackedGraph: graph_data[column][i].stacked,
            includeZero: graph_data[column][i].stacked,
            labels: graph_data[column][i].labels,
            //title: graph_data[column][i].title,
            underlayCallback: function(canvas, area, g) {
                canvas.fillStyle = "rgba(105, 105, 185, 185)";
                //Draw simplification points
                colnum = graph_data[column][i].colnum;

                for(var k = 0; k < simplificationPoints[colnum].length-1; k++) {
                    var bottom_left = g.toDomCoords(simplificationPoints[colnum][k], -20);
                    var left = bottom_left[0];
                    canvas.fillRect(left, area.y, 2, area.h);
                }
            },
            axes: {
              x: {
                valueFormatter: function(d) {
                  return 'Conflict ' + d;
                },
                pixelsPerLabel: 100,
                includeZero: true,
                drawGrid: false,
                drawAxis: false,
              },
              y: {
                  drawGrid: false,
                  drawAxis: false,
              }
            },
            //stepPlot: true,
            //strokePattern: [0.1, 0, 0, 0.5],
            strokeWidth: 0.3,
            highlightCircleSize: 3,
            rollPeriod: (graph_data[column][i].tablename == "restart") ? 10: 0,
            legend: 'always',
            xlabel: false,
            labelsDiv: document.getElementById(graph_data[column][i].labelDivID),
            labelsSeparateLines: true,
            labelsKMB: true,
            drawPoints: true,
            pointSize: 1,
            strokeStyle: "black",
            colors: ['#000000', '#05fa03', '#d03332', '#4e4ea8', '#689696'],
            fillAlpha: 0.8,
            errorBars: graph_data[column][i].noisy,
            dateWindow: [0, maxConflRestart[graph_data[column][i].colnum]],
            drawCallback: function(me, initial) {

                //Fill original sizes, so if we zoom out, we know where to
                //zoom out
                if (initial)
                    origSizes[graph_data[column][i].colnum] = me.xAxisRange();

                //Initial draw, ignore
                if (blockRedraw || initial)
                    return;

                blockRedraw = true;
                var xrange = me.xAxisRange();

                draw_all_clause_distributions(xrange[0], xrange[1]);

                //Zoom every one the same way
                for (var j = 0; j < graph_data[column].length; j++) {
                    //Don't go into loop
                    if (graphs[column][j] == me)
                        continue;

                    //If this is a full reset, then zoom out maximally
                    graphs[column][j].updateOptions( {
                        dateWindow: xrange
                    } );
                    //console.log(xrange);
                }

                blockRedraw = false;
            }
        }
    )

    return graph;
}

function draw_all_clause_distributions(from, to)
{
    for(var column = 0; column < columnDivs.length; column++) {
        if (clDistrib.length <= column) {
            continue;
        }

        for(var i2 = 0; i2 < clDistrib[column].length; i2++) {
            a = new draw_clause_distribution(
                    clDistrib[column][i2].data
                    , clDistrib[column][i2].canvasID
                    , clDistrib[column][i2].dataDivID
                    , simplificationPoints[column]
                );

            if (from === undefined)
                a.drawPattern(0, maxConflRestart[column]);
            else
                a.drawPattern(from, to);

            dists.push(a);
        }
    }
}

function draw_clause_distribution(_data, _canvas_div_ID, _div_ID, _simpPoints)
{
    var data = _data;
    var canvas_div_ID = _canvas_div_ID;
    var div_ID = _div_ID;
    var simpPoints = _simpPoints;
    var mywidth = document.getElementById(div_ID).offsetWidth-5;
    var myheight = document.getElementById(div_ID).offsetHeight;

    //For SVG pattern, a rectangle
    function drawDistribBox(x1, x2, y1, y2, relHeight, imgData)
    {
        var num = 255-relHeight*255.0;
        var type = "rgb(" + Math.floor(num) + "," + Math.floor(num) + "," + Math.floor(num) + ")";
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
        var numElementsVertical = 0;
        for(var i = 0 ; i < data.length ; i ++ ){
            //out of range, ignore
            if (data[i].conflEnd < from) {
                continue;
            }
            if (data[i].conflStart > to) {
                break;
            }

            //Check which is the highest
            for(var i2 = data[i].darkness.length-1; i2 >= 0 ; i2--) {
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
        var myDiv = document.getElementById(canvas_div_ID);
        var ctx = myDiv.getContext("2d");
        var Xdelta = 0.5;

        var onePixelisConf = mywidth/(to-from);
        var numElementsVertical = calcNumElemsVertical(from, to);

        //Cut-off lines for Y
        var vAY = new Array();
        for(var i = numElementsVertical; i >= 0; i--) {
            vAY.push(Math.round(i*(myheight/numElementsVertical)));
        }
        vAY.push(0);

        //Start drawing from X origin
        var lastXEnd = 0;
        var startFound = 0;
        for(var i = 0 ; i < data.length ; i ++) {
            if (startFound == 0 && data[i].conflEnd < from)
                continue;

            if (startFound == 1 && data[i].conflStart > to)
                continue;

            //Calculate maximum darkness
            maxDark = 0;
            for(var i2 = 0; i2 < data[i].darkness.length; i2++) {
                maxDark = Math.max(maxDark, data[i].darkness[i2]);
            }

            var xStart = lastXEnd;

            var xEnd = data[i].conflEnd - from;
            xEnd *= onePixelisConf;
            xEnd += Xdelta;
            xEnd = Math.max(0, xEnd);
            xEnd = Math.min(xEnd, mywidth);
            xEnd = Math.round(xEnd);
            lastXEnd = xEnd;

            //Go through each Y component
            for(var i2 = 0; i2 < data[i].darkness.length; i2++) {
                yStart = vAY[i2+1];
                yEnd   = vAY[i2];

                //How dark should it be?
                var darkness = 0;
                if (data[i].darkness[i2] != 0) {
                    darkness = data[i].darkness[i2]/maxDark;
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
                drawSimpLine(point, point+1, 0, myheight, ctx);
            }
        }
    }
}

function calc_width()
{
    var width = 600/maxConflRestart.length;
    return width;
}

//Creates HTML for dygraphs
function createHTMLforGraphs()
{
    var width = calc_width();
    var datagraphs = document.getElementById("datagraphs");
    for (var i = 0; i < graph_data[0].length; i++) {
        for (var column = 0; column < graph_data.length; column++) {
            datagraphs.innerHTML += "\
            <div class=\"block\" id=\"" + graph_data[column][i].blockDivID + "\">\
            <table id=\"plot-table-a\">\
            <tr>\
            <td width=\""+width+"\"><div id=\"" + graph_data[column][i].dataDivID + "\" class=\"myPlotData\" style=\"width:"+width+"px;\"></div></td>\
            <td>\
\
            <table id=\"plot-table-a\">\
            <tr><td>"+ graph_data[column][i].title +"</td></tr>\
            <tr><td><div id=\"" + graph_data[column][i].labelDivID + "\" class=\"draghandle\"></div></td></tr>\
            </table>\
\
            </td>\
            </tr>\
            </table>\
            </div>";
        }
    }
}

function createHTMLforDists()
{
    var width = calc_width();
    for(var column = 0; column < clDistrib.length; column++) {
        for(var i2 = 0; i2 < clDistrib[column].length; i2++) {
            var datagraphs = document.getElementById("datagraphs");

            datagraphs.innerHTML += "\
            <div class=\"block\" id=\"" + clDistrib[column][i2].blockDivID +"\"> \
            <table id=\"plot-table-a\"> \
            <tr> \
            <td> \
                <div id=\""+ clDistrib[column][i2].dataDivID + "\" class=\"myPlotData\" style=\"width:"+width+"px;\"> \
                <canvas id=\""+ clDistrib[column][i2].canvasID + "\" class=\"canvasPlot\"> \
                no support for canvas \
                </canvas> \
                </div> \
            </td> \
            <td> \
                <div id=\"" + clDistrib[column][i2].labelDivID + "\" class=\"draghandle\"><b> \
                (" + column + ") Newly learnt clause " + clDistrib[column][i2].lookAt + " distribution. \
                Bottom: 1. Top:  max. \
                Black: Many. White: 0. \
                </b></div> \
            </td> \
            </tr> \
            </table> \
            </div>";
        }
    }
}

function createPortal()
{
    var settings = {};
    for(var i = 0; i < columnDivs.length; i++) {
        tmp = Array();
        for(var i2 = 0; i2 < columnDivs[i].length; i2++) {
            tmp.push(columnDivs[i][i2].blockDivID);
        }
        settings["column-" + i] = tmp;
    }
    var options = { portal : 'columns', editorEnabled : true};
    var data = {};
    //Event.observe(window, 'load', function() {
            portal = new Portal(settings, options, data);
    //});
}

function create_columns(num)
{
    var columns = document.getElementById("columns");
    column_text = "\
    <table>\
    <tr>";
    for(var i = 0; i < num; i++) {
        column_text += "\
        <td><div id=\"column-" + i +"\" class=\"column menu\"></div></td>";
    }
    column_text += "\
    </tr>\
    </table>";
    columns.innerHTML = column_text;
}

function clear_everything()
{
    create_columns(0);
    origSizes = new Array();
    blockRedraw = false;
    dists = [];

    datagraphs = document.getElementById("datagraphs");
    datagraphs.innerHTML = "";
}

function print_all_graphs()
{
    clear_everything();
    create_columns(graph_data.length);

    //Clear & create HTML
    createHTMLforGraphs();
    createHTMLforDists();

    //Draws the graphs
    drawAllGraphs();
    draw_all_clause_distributions();
    createPortal();
}

