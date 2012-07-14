<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
    <meta charset="utf-8">
    <title>Cryptominisat 3 visualization</title>

    <link rel="stylesheet" type="text/css" href="jquery.jqplot.css" />
    <script type="text/javascript" src="jquery/jquery.js"></script>
    <script type="text/javascript" src="dygraphs/dygraph-dev.js"></script>
<!--     <script type="text/javascript" src="http://ajax.googleapis.com/ajax/libs/prototype/1.6.1/prototype.js"></script> -->
    <script type="text/javascript" src="scriptaculous-js-1.9.0/lib/prototype.js"></script>
    <script type="text/javascript" src="scriptaculous-js-1.9.0/src/scriptaculous.js"></script>
    <script type="text/javascript" src="dragdrop/js/portal.js"></script>
    <style>
/*     @import url(//fonts.googleapis.com/css?family=Yanone+Kaffeesatz:400,700); */
    @import url(style.css);
    </style>


    <!-- Start of StatCounter Code -->
    <!--<script type="text/javascript">
    var sc_project=6140803;
    var sc_invisible=1;
    var sc_security="26273f9f";
    </script>

    <script type="text/javascript"
    src="http://www.statcounter.com/counter/counter.js">
    </script>

    <noscript><div class="statcounter"><a title="tumblr page counter"
    href="http://statcounter.com/tumblr/" target="_blank">
    <img class="statcounter" src="http://c.statcounter.com/6140803/0/26273f9f/1/"
    alt="tumblr page counter"></a></div>
    </noscript>-->
    <!-- End of StatCounter Code -->

</head>

<body>
<h1>Cryptominisat 3</h1>

<h3>Replacing wordy authority with visible certainty</h4>
<p>This webpage shows the partial solving of two SAT instances, visually.
I was amazed by <a href="http://www.edwardtufte.com/tufte/">Edward Tufte</a>'s
work (hence the subtitle) and this came out of it. Tufte would probably not
approve, as some of the layout is terrible. However, it might allow you to
understand SAT better, and may offer inspiration... or, rather, <i>vision</i>.
Enjoy.
</p>


<!--<p>Please select averaging level:
<input type="range" min="1" max="21" value="1" step="1" onchange="showValue(this.value)"/>
<span id="range">1</span>
</p>-->
<h2>Search restart statistics</h2>
<p>Below you will find conflicts in the X axis and several interesting data
on the Y axis. There are two columns, each selectable what to show.
Every datapoint corresponds to a restart. You may zoom in by
clicking on an interesting point and dragging the cursor along the X axis.
Double-click to unzoom. You can rearrange the order and layout by dragging the
labels on the right. Blue vertical lines indicate the positions of
<i>simplification sessions</i>. Between the blue lines are <i>search
sessions</i>. The angle of the "time" graph indicates conflicts/second.
Simplification sessions are not detailed. However, time jumps during
simplifcaition, and the solver behaviour changes afterwards. The angle
of the "restart no." graph indicates how often restarts were made. You can
find a full list of terms below.
</p>

<script type="text/javascript">
function showValue(newValue)
{
    document.getElementById("range").innerHTML=newValue;
    setRollPeriod(newValue);
}
</script>

<script type="text/javascript">
var myData=new Array();
</script>

<table class="doubleSize">
<tr><td>
    <div id="columns">
    <div id="column-0" class="column menu"></div>
    <div id="column-1" class="column menu"></div>
    </div>
</td></tr>
</table>


<p style="clear:both"></p>

<?
$runID  = 44;
$runID2 = 44;
$maxconfl = 1000000;
error_reporting(E_ALL);
//error_reporting(E_STRICT);
//error_reporting(E_STRICT);

$username="presenter";
$password="presenter";
$database="cryptoms";

mysql_connect("localhost", $username, $password);
@mysql_select_db($database) or die( "Unable to select database");

$numberingScheme = 0;
function printOneThing(
    $datanames
    , $nicedatanames
    , $data
    , $nrows
    , &$orderNum
    , $colnum
    , $doSlideAvg = 0
) {
    global $numberingScheme;
    $fullname = "toplot_".$numberingScheme."_".$colnum;
    //$nameLabel = "Label-$numberingScheme-$colnum";

    echo "
    <div class=\"block\" id=\"block".$orderNum."AT".$colnum."\">
    <table id=\"plot-table-a\">
    <tr>
    <td><div id=\"$fullname"."_datadiv\" class=\"myPlotData\"></div></td>
    <td><div id=\"$fullname"."_labeldiv\" class=\"draghandle\"></div></td>
    </tr>
    </table>
    </div>";
    $orderNum++;

    echo "<script type=\"text/javascript\">";
    echo $fullname."_data = [";

    echo "[0, ";

    $i = 0;
    while($i < sizeof($datanames)) {
        echo "null";

        $i++;
        if ($i < sizeof($datanames)) {
            echo ", ";
        }
    }
    echo "],";

    $i=0;
    $total_sum = 0.0;
    $last_confl = 0.0;
    while ($i < $nrows) {
        //Print conflicts
        $confl=mysql_result($data, $i, "conflicts");
        echo "[$confl";

        //Calc local sum
        $local_sum = 0;
        foreach ($datanames as $dataname) {
            $local_sum += mysql_result($data, $i, $dataname);
        }

        //Print for each
        foreach ($datanames as $dataname) {
            $tmp = mysql_result($data, $i, $dataname);
            if (sizeof($datanames) > 1) {
                $tmp /= $local_sum;
                $tmp *= 100.0;
                echo ", $tmp";
            } else {
                $total_sum += $tmp*($confl-$last_confl);
                $last_confl = $confl;
                $sliding_avg = $total_sum / $confl;
                echo ", $tmp";
                if ($doSlideAvg) {
                    echo ", $sliding_avg";
                }
            }
        }
        echo "]\n";

        $i++;
        if ($i < $nrows) {
            echo ",";
        }
    }
    echo "];\n";

    //Add name & data
    echo "myData.push({data: $fullname"."_data, div: \"$fullname"."_datadiv\"";

    //Calculate labels
    echo ", labels: [\"Conflicts\"";
    foreach ($nicedatanames as $dataname) {
        echo ", \"($colnum) $dataname\"";
    }
    if (sizeof($datanames) == 1) {
        echo ", \"$fullname$colnum (sliding avg.)\"";
    }
    echo "]";

    //Stacked?
    echo ", stacked: ".(int)(sizeof($datanames) > 1);
    echo ", labeldiv: \"$fullname"."_labeldiv\"";
    echo ", colnum: \"$colnum\"";
    echo " });\n";

    echo "</script>\n";
    $numberingScheme++;
}

function printOneSolve($runID, $colnum, $maxconfl) {
    $query="
    SELECT *
    FROM `restart`
    where `runID` = $runID
    and conflicts < $maxconfl
    order by `conflicts`";
    $result=mysql_query($query);
    if (!$result) {
        die('Invalid query: ' . mysql_error());
    }
    $nrows=mysql_numrows($result);

    $orderNum = 0;
    printOneThing(array("time")
        , array("time"), $result, $nrows, $orderNum, $colnum);

    printOneThing(array("restarts")
        , array("restart no."), $result, $nrows, $orderNum, $colnum);

    /*printOneThing(array("propsPerDec")
        , array("avg. no. propagations per decision"), $result, $nrows, $orderNum, $colnum);*/

    printOneThing(array("branchDepth")
        , array("avg. branch depth"), $result, $nrows, $orderNum, $colnum);

    printOneThing(array("branchDepthDelta")
        , array("avg. no. of levels backjumped"), $result, $nrows, $orderNum, $colnum);

    printOneThing(array("trailDepth")
        , array("avg. trail depth"), $result, $nrows, $orderNum, $colnum);

    printOneThing(array("trailDepthDelta")
        , array("avg. trail depth delta"), $result, $nrows, $orderNum, $colnum);

    printOneThing(array("glue")
        , array("newly learnt clauses avg. glue"), $result, $nrows, $orderNum, $colnum);

    printOneThing(array("size")
        , array("newly learnt clauses avg. size"), $result, $nrows, $orderNum, $colnum);

    printOneThing(array("resolutions")
        , array("avg. no. resolutions for 1UIP"), $result, $nrows, $orderNum, $colnum);

    printOneThing(array("agility")
        , array("avg. agility"), $result, $nrows, $orderNum, $colnum);

    /*printOneThing(array("flippedPercent")
        , array("var polarity flipped %"), $result, $nrows, $orderNum, $colnum);*/

    printOneThing(array("conflAfterConfl")
        , array("conflict after conflict %"), $result, $nrows, $orderNum, $colnum);

    /*printOneThing("conflAfterConflSD", array("conflAfterConfl")
        , array("conflict after conflict std dev %"), $result, $nrows, $orderNum, $colnum);*/

    printOneThing(array("watchListSizeTraversed")
        , array("avg. traversed watchlist size"), $result, $nrows, $orderNum, $colnum);

    printOneThing(array("watchListSizeTraversedSD")
        , array("avg. traversed watchlist size std dev"), $result, $nrows, $orderNum, $colnum);

    /*printOneThing("litPropagatedSomething", array("litPropagatedSomething")
        , array("literal propagated something with binary clauses %"), $result, $nrows, $orderNum, $colnum);*/

    printOneThing(array("replaced")
        , array("vars replaced"), $result, $nrows, $orderNum, $colnum);

    printOneThing(array("set")
        , array("vars set"), $result, $nrows, $orderNum, $colnum);

    printOneThing(array("varSetPos", "varSetNeg")
        , array("propagated polar pos %", "propagated polar neg %"), $result, $nrows, $orderNum, $colnum);

    printOneThing(array("learntUnits", "learntBins", "learntTris", "learntLongs")
        ,array("new learnts unit %", "new learnts bin %", "new learnts tri %", "new learnts long %"), $result, $nrows, $orderNum, $colnum);

    printOneThing(array(
        "propBinIrred", "propBinRed"
        , "propTriIrred", "propTriRed"
        , "propLongIrred", "propLongRed"
        )
        ,array("prop by bin irred %", "prop by bin red %"
        , "prop by tri irred %", "prop by tri red %"
        , "prop by long irred %", "prop by long red %"
        ), $result, $nrows, $orderNum, $colnum);

    printOneThing(array(
        "conflBinIrred"
        , "conflBinRed"
        , "conflTriIrred"
        , "conflTriRed"
        , "conflLongIrred"
        , "conflLongRed"
        )
        ,array(
        "confl by bin irred %"
        , "confl by bin red %"
        , "confl by tri irred %"
        , "confl by tri red %"
        , "confl by long irred %"
        , "confl by long red %"
        )
        , $result, $nrows, $orderNum, $colnum
    );

    /*printOneThing("branchDepthSD", array("branchDepthSD")
        , array("branch depth std dev"), $result, $nrows, $orderNum, $colnum);

    printOneThing("branchDepthDeltaSD", array("branchDepthDeltaSD")
        , array("branch depth delta std dev"), $result, $nrows, $orderNum, $colnum);

    printOneThing("trailDepthSD", array("trailDepthSD")
        , array("trail depth std dev"), $result, $nrows, $orderNum, $colnum);

    printOneThing("trailDepthDeltaSD", array("trailDepthDeltaSD")
        , array("trail depth delta std dev"), $result, $nrows, $orderNum, $colnum);

    printOneThing("glueSD", array("glueSD")
        , array("newly learnt clause glue std dev"), $result, $nrows, $orderNum, $colnum);

    printOneThing("sizeSD", array("sizeSD")
        , array("newly learnt clause size std dev"), $result, $nrows, $orderNum, $colnum);

    printOneThing("resolutionsSD", array("resolutionsSD")
        , array("std dev no. resolutions for 1UIP"), $result, $nrows, $orderNum, $colnum);*/

    return $orderNum;
}

$orderNum = printOneSolve($runID, 0, $maxconfl);
$orderNum = printOneSolve($runID2, 1, $maxconfl);
?>

<div class="block" id="blockSpecial0">
    <table id="plot-table-a">
    <tr>
    <td><div id="MYdrawingPad0Parent" class="myPlotData">
    <canvas id="MYdrawingPad0" width="420" height="100">no support for canvas</canvas>
    </div></td>

    <td><div class="draghandle"><b>
    (0) Newly learnt clause size distribution.
    Bottom: unitary clause. Top: largest clause.
    Black: Many learnt. White: None learnt.
    Horizontal resolution: 1000 conflicts.
    Vertical resolution: 1 literal
    </b></div></td>
    </tr>
    </table>
</div>

<div class="block" id="blockSpecial1">
    <table id="plot-table-a">
    <tr>
    <td><div id="MYdrawingPad0Parent" class="myPlotData">
    <canvas id="MYdrawingPad1" width="420" height="100">no support for canvas
    </canvas>
    </div></td>

    <td><div class="draghandle"><b>
    (1) Newly learnt clause size distribution.
    Bottom: unitary clause. Top: largest clause.
    Black: Many learnt. White: None learnt.
    Horizontal resolution: 1000 conflicts.
    Vertical resolution: 1 literal
    </b></div></td>
    </tr>
    </table>
</div>

<script type="text/javascript">
<?
function getMaxSize($runID)
{
    $query="
    SELECT max(size) as mymax FROM clauseSizeDistrib
    where runID = $runID
    and num > 10";
    $result=mysql_query($query);

    if (!$result) {
        die('Invalid query: ' . mysql_error());
    }
    return mysql_result($result, 0, "mymax");
}

function getMaxConflDistrib($runID, $runID2)
{
    $query="
    SELECT max(conflicts) as mymax FROM clauseSizeDistrib
    where (runID = $runID or  runID = $runID2)
    and num > 0";
    $result=mysql_query($query);

    if (!$result) {
        die('Invalid query: ' . mysql_error());
    }
    return mysql_result($result, 0, "mymax");
}

function getMaxConflRestart($runID, $maxconfl)
{
    $query="
    SELECT max(conflicts) as mymax FROM `restart`
    where conflicts < $maxconfl
    and runID = $runID";
    $result=mysql_query($query);

    if (!$result) {
        die('Invalid query: ' . mysql_error());
    }
    return mysql_result($result, 0, "mymax");
}

$maxSize = [];
$maxSize[0] = getMaxSize($runID) - 1; //Because no use for size 0
$maxSize[1] = getMaxSize($runID2) - 1; //Because no use for size 0
echo "var maxSize = [$maxSize[0], $maxSize[1]];\n";
$maxConflDistrib = getMaxConflDistrib($runID, $runID2);
echo "var maxConflDistrib = $maxConflDistrib;\n";

$maxConflRestart = [];
$maxConflRestart[0] = getMaxConflRestart($runID, $maxconfl);
$maxConflRestart[1] = getMaxConflRestart($runID2, $maxconfl);
echo "var maxConflRestart = [$maxConflRestart[0], $maxConflRestart[1]];\n";

$minConflRestart = [];
$minConflRestart[0] = 0;
$minConflRestart[1] = 0;
echo "var minConflRestart = [$minConflRestart[0], $minConflRestart[1]];\n";

//$statPer = 1000;
//echo "var statPer = $statPer;\n";

function fillClauseDistrib($num, $runID)
{
    global $maxSize;
    global $maxConflDistrib;

    echo "clauseDistrib.push([]);\n";
    $lastConflicts = 0;
    $query = "
    SELECT conflicts, num FROM clauseSizeDistrib
    where runID = $runID
    and size <= $maxSize[$num]
    and size > 0
    order by conflicts, size";
    $result=mysql_query($query);
    if (!$result) {
        die('Invalid query: ' . mysql_error());
    }
    $nrows=mysql_numrows($result);

    $rownum = 0;
    $lastConfl = 0;
    while($rownum < $nrows) {
        $confl = mysql_result($result, $rownum, "conflicts");
        echo "tmp = {conflStart: $lastConfl, conflEnd: $confl, height: [";
        $lastConfl = $confl;
        while($rownum < $nrows) {
            $numberOfCl = mysql_result($result, $rownum, "num");
            echo "$numberOfCl";

            //More in this bracket?
            $rownum++;
            if ($rownum >= $nrows
                || mysql_result($result, $rownum, "conflicts") != $confl
            ) {
                break;
            }

            echo ",";
        }
        echo "]};\n";
        echo "clauseDistrib[$num].push(tmp);\n";
    }
}

echo "var clauseDistrib = [];\n";
fillClauseDistrib(0, $runID, $maxConflDistrib);
fillClauseDistrib(1, $runID2, $maxConflDistrib);

echo "var settings = {";
for($i2 = 0; $i2 < 2; $i2++) {
    echo "'column-$i2' : [";
    for($i = 0; $i < $orderNum; $i++) {
        echo "'block".$i."AT".$i2."'";

        if ($i+1 < $orderNum)
            echo ", ";
    };
    echo ",'blockSpecial$i2' ]";

    if ($i2+1 < 2) {
        echo ",";
    }
}
echo "};";

echo "var options = { portal : 'columns', editorEnabled : true};
    var data = {};
    var portal;
    Event.observe(window, 'load', function() {
        portal = new Portal(settings, options, data);
    });";

function fillSimplificationPoints($runID)
{
    $query="
    SELECT max(conflicts) as confl, simplifications as simpl
    FROM restart
    where runID = $runID
    group by simplifications
    order by simplifications";

    $result=mysql_query($query);
    if (!$result) {
        die('Invalid query: ' . mysql_error());
    }
    $nrows=mysql_numrows($result);

    echo "tmp = [0,";
    $i=0;
    while ($i < $nrows) {
        $confl = mysql_result($result, $i, "confl");
        echo "$confl";
        $i++;
        if ($i < $nrows) {
            echo ", ";
        }
    }
    echo "];";
    echo "simplificationPoints.push(tmp);\n";
}
echo "var simplificationPoints = [];\n";
fillSimplificationPoints($runID);
fillSimplificationPoints($runID2);
?>
</script>

<script type="text/javascript">
//Stores the original X sizes of the graphs
//Used when zooming out fully
origSizes = [];

//Draw graphs
gs = [];
var blockRedraw = false;
for (var i = 0; i < myData.length; i++) {
    gs.push(new Dygraph(
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
            //errorBars: false,
            drawCallback: function(me, initial) {
                if (initial) {
                    origSizes[myData[i].colnum] = me.xAxisRange();
                }
                if (blockRedraw || initial)
                    return;

                blockRedraw = true;
                var xrange = me.xAxisRange();
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
                if (fullreset) {
                    drawPattern(clauseDistrib[0], 0, origSizes[0][0], origSizes[0][1]);
                    drawPattern(clauseDistrib[1], 1, origSizes[1][0], origSizes[1][1]);
                } else {
                    drawPattern(clauseDistrib[0], 0, xrange[0], xrange[1]);
                    drawPattern(clauseDistrib[1], 1, xrange[0], xrange[1]);
                }

                for (var j = 0; j < myData.length; j++) {
                //for (var j = 0; j < 5; j++) {
                    if (gs[j] == me) {
                        continue;
                    }

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
    ));
}

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

//SVG pattern
function drawPattern(data, num, from , to)
{
    var myDiv = document.getElementById( "MYdrawingPad" + num);
    myDiv.style.height = 100;
    myDiv.style.width= 420;

    var width = 415;
    var height = 100;
    ctx = myDiv.getContext("2d");
    Xdelta = 0.5;
    var i;

    var onePixelisConf = width/(to-from);

    //Y components' limits are here
    var vAY = new Array();
    numElementsVertical = data[0].height.length;
    for(i = numElementsVertical; i >= 0; i--) {
        vAY.push(Math.round(i*(height/numElementsVertical)));
    }
    vAY.push(0);

    //Start drawing from X origin
    lastXEnd = 0;
    for( i = 0 ; i < data.length ; i ++ ){

        //Calculate maximum height
        maxHeight = 0;
        for(i2 = 0; i2 < data[i].height.length; i2++) {
            maxHeight = Math.max(maxHeight, data[i].height[i2]);
        }

        xStart = lastXEnd;

        xEnd = data[i].conflEnd - from;
        xEnd *= onePixelisConf;
        xEnd += Xdelta;
        xEnd = Math.max(0, xEnd);
        xEnd = Math.min(xEnd, width);
        xEnd = Math.round(xEnd);
        lastXEnd = xEnd;

        //Go through each Y component now
        for(i2 = 0; i2 < data[i].height.length; i2++) {
            yStart = vAY[i2+1];
            yEnd   = vAY[i2];

            //How dark should it be?
            if (data[i].height[i2] != 0) {
                relHeight = data[i].height[i2]/maxHeight;
            } else {
                relHeight  = 0;
            }

            //Create the rectangle
            drawDistribBox(xStart, xEnd, yStart, yEnd, relHeight, ctx);
        }
    }

    //Handle simplification lines
    for(var k = 0; k < simplificationPoints[num].length-1; k++) {
        var point = simplificationPoints[num][k] - from;
        point *= onePixelisConf;
        point += Xdelta;

        //Draw blue line
        if (point > 0) {
            drawSimpLine(point, point+1, 0, height, ctx);
        }
    }
}
drawPattern(clauseDistrib[0], 0, minConflRestart[0], maxConflRestart[0]);
drawPattern(clauseDistrib[1], 1, minConflRestart[1], maxConflRestart[1]);

function setRollPeriod(num)
{
    for (var j = 0; j < myDataNames.length; j++) {
        gs[j].updateOptions( {
            rollPeriod: num
        } );
    }
}
</script>

<h2>Terms used</h2><p style="clear:both"></p><table class="box-table-b">
<tr><th>Abbreviation</th><th>Explanation</th></tr>
<tr><td>red.</td><td>reducible, also called learnt</td></tr>
<tr><td>irred.</td><td>irreducible, also called non-learnt</td></tr>
<tr><td>confl</td><td>conflict reached by the solver</td></tr>
<tr><td>learnt</td><td>clause learnt during 1UIP conflict analysis</td></tr>
<tr><td>trail depth</td><td>depth of serach i.e. the number of variables set when the solver reached a conflict</td></tr>
<tr><td>brach depth</td><td>the number of branches made before conflict was encountered</td></tr>
<tr><td>trail depth delta</td><td>the number of variables we jumped back when doing conflict-directed backjumping</td></tr>
<tr><td>branch depth delta</td><td>the number of branches jumped back during conflict-directed backjumping</td></tr>
<tr><td>propagations/decision</td><td>number of variables set due to the propagation of a decision (note that there is always at least one, the variable itself)</td></tr>
<tr><td>vars replaced</td><td>the number or variables replaced due to equivalent literal simplfication</td></tr>
<tr><td>polarity flipped</td><td>polarities of variables are <a href="http://dx.doi.org/10.1007/978-3-540-72788-0_28">saved</a> and then used if branching is needed, but if propagation takes place, they are sometimes flipped</td></tr>
<tr><td>std dev</td><td>standard deviation, the square root of variance</td></tr>
<tr><td>confl by</td><td>the clause that caused the conflict</td></tr>
<tr><td>agility</td><td>See <a href="http://www.inf.ucv.cl/~bcrawford/PapersAutonomousSearch_julio2008/BRODERICK_CRAWFORD_AGO_01_X.pdf">here</a>.</td></tr>
<tr><td>glue</td><td>the number of different decision levels of the literals found in newly learnt clauses. See <a href = "http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.150.1911">here</a></td></tr>
<tr><td>conflict after conflict %</td><td>How often does it happen that a conflict, after backtracking and propagating immeediately (i.e. without branching) leads to a conflict. This is displayed because it's extremely high percentage relative to what most would expect. Thanks to <a href="http://www.cril.univ-artois.fr/~jabbour/">Said Jabbour</a> for this.</td></tr>
</table>

<h2>Why did I do this?</h2>
<p>There has been some
<a href="http://www-sr.informatik.uni-tuebingen.de/~sinz/DPvis/">
past work</a> on statically visualizing SAT problems by
<a href="http://www.carstensinz.de/">Carsten Sinz</a>, but not much on
dynamic solving visualization - in fact, nothing comes to my mind that
is comparable to what is above. However, the point of this excercise was
not only to visually display dynamic solver behaviour. Rather, I
think we could do dynamic analysis and heuristic adjustment instead
of the static analysis and static heuristic selection as done by current
<a href="http://www.jair.org/media/2490/live-2490-3923-jair.pdf">
portifolio solvers</a>. Accordingly, CryptoMiniSat 3 has an extremely
large set of options - e.g. swithcing between cleaning using glues,
activities, clause sizes, or number of propagations+conflicts made by
a clause is only a matter of setting a variable, and can be done on-the-fly.
Problems tend to evolve as simplication and solving steps are made,
so search heuristics should evolve with the problem.
</p>


<!--all is up in the
<a href="https://github.com/msoos/cryptominisat">GIT</a>, including SQL,
PHP, HTML, CSS and more.-->

<h2>The End</h2>
<p>If you enjoyed this visualization, there are three things you can do.
First, tell me about your impressions  <a href="http://www.msoos.org/">
here</a> and send the link to a friend. Second, you can
<a href="http://www.srlabs.de">contact my employer</a>, and he will be happy
to find a way for us to help you with your SAT problems.
</p>

<h2>Acknowledgements</h2>
<p>I would like to thank my employer for letting me play with SAT,
my collegue <a href="http://www.flickr.com/photos/lucamelette/">
Luca Melette</a> for helping me with ideas and coding,
<a href="http://folk.uio.no/vegardno/">Vegard Nossum</a> for the many
discussions we had about visualization,
<a href="http://www.inra.fr/mia/T/katsirelos/">George Katsirelos</a> for
improvement ideas, <a href="http://www.cril.univ-artois.fr/~jabbour/">
Said Jabbour</a> for further improvement ideas,
<a href="http://dygraphs.com/">Dygraphs</a> for the visually pleasing graphs,
<a href="http://www.michelhiemstra.nl/blog/igoogle-like-drag-drop-portal-v20/">
Portal</a> for the drag-and-drop feature and Edward Tufte for all his wonderful
<a href="http://www.edwardtufte.com/tufte/books_vdqi">books</a>.
</p>

<br/>
<p><small>Copyright <a href="http://www.msoos.org">Mate Soos</a>, 2012.
Licensed under <a href="http://creativecommons.org/licenses/by-nc-sa/2.5/">
CC BY-NC-SA 2.5</a></small>
</p>

</body>
</html>
