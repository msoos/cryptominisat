<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
    <meta charset="utf-8">
    <title>Cryptominisat 3 visualization</title>

    <link rel="stylesheet" type="text/css" href="jquery.jqplot.css" />
    <script type="text/javascript" src="jquery/jquery.min.js"></script>
    <script type="text/javascript" src="dygraphs/dygraph-combined.js"></script>
    <script type="text/javascript" src="highcharts/js/highcharts.js"></script>
<!--     <script type="text/javascript" src="http://ajax.googleapis.com/ajax/libs/prototype/1.6.1/prototype.js"></script> -->
    <script type="text/javascript" src="scriptaculous-js-1.9.0/lib/prototype.js"></script>
    <script type="text/javascript" src="scriptaculous-js-1.9.0/src/scriptaculous.js"></script>
    <script type="text/javascript" src="dragdrop/js/portal.js"></script>
    <style>
/*     @import url(//fonts.googleapis.com/css?family=Yanone+Kaffeesatz:400,700); */
    @import url(style.css);
    </style>


    <script type="text/javascript">
      var _gaq = _gaq || [];
      _gaq.push(['_setAccount', 'UA-15197329-2']);
      _gaq.push(['_trackPageview']);

      (function() {
        var ga = document.createElement('script'); ga.type = 'text/javascript'; ga.async = true;
        ga.src = ('https:' == document.location.protocol ? 'https://ssl' : 'http://www') + '.google-analytics.com/ga.js';
        var s = document.getElementsByTagName('script')[0]; s.parentNode.insertBefore(ga, s);
      })();
    </script>

</head>

<body>
<h1>Cryptominisat 3</h1>

<h3>Replacing wordy authority with visible certainty</h4>
<p>This webpage shows the partial solving of two SAT instances, visually. I was amazed by <a href="http://www.edwardtufte.com/tufte/">Edward Tufte</a>'s work (hence the subtitle) and this came out of it. Tufte would probably not approve, as some of the layout is terrible. However, it might allow you to understand SAT better, and may offer inspiration... or, rather, <i>vision</i>. Enjoy.</p>


<!--<p>Please select averaging level:
<input type="range" min="1" max="21" value="1" step="1" onchange="showValue(this.value)"/>
<span id="range">1</span>
</p>-->
<h2>Search restart statistics</h2>
<p>Below you will find conflicts in the X axis and several interesting data on the Y axis. There are two columns, the left column is solving <a href="http://www.cril.univ-artois.fr/SAT09/results/bench.php?idev=29&idbench=69562">mizh-md5-47-3.cnf</a> (a crypto problem), the right column is solving <a href="http://www.cril.univ-artois.fr/SAT09/results/bench.php?idev=29&idbench=71037">UTI-20-10-p0.cnf</a> (a diagnosis problem) - both were aborted at 60'000 conflicts. Every datapoint corresponds to a restart. You may zoom in by clicking on an interesting point and dragging the cursor along the X axis. Double-click to unzoom. You can rearrange the order and layout by dragging the labels on the right. Blue vertical lines indicate the positions of <i>simplification sessions</i>. Between the blue lines are <i>search sessions</i>. The angle of the "time" graph indicates conflicts/second. Simplification sessions are not detailed. However, time jumps during simplifcaition, and the solver behaviour changes afterwards. The angle of the "restart no." graph indicates how often restarts were made. You can find a full list of terms below.</p>

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
$runID = 2923895824;
$runID2 = 840192416;
//$runID = 2427843192;
//$runID2 = 2941286659;
$maxconfl = 100000;
//$runID = 1628198452;
//$runID = 3097911473;
//$runID = 456297562;
//$runID = 657697659;
//$runID = 3348265795;
error_reporting(E_ALL);
error_reporting(E_STRICT);
//error_reporting(E_STRICT);

$username="presenter";
$password="presenter";
$database="cryptoms";

mysql_connect(localhost, $username, $password);
@mysql_select_db($database) or die( "Unable to select database");

function printOneThing(
    $name
    , $datanames
    , $nicedatanames
    , $data
    , $nrows
    , &$orderNum
    , $colnum
    , $doSlideAvg = 0
) {
    $fullname = $name."Data";
    $nameLabel = $name."Label";

    echo "
    <div class=\"block\" id=\"block".$orderNum."AT".$colnum."\">
    <table id=\"plot-table-a\">
    <tr>
    <td><div id=\"$name$colnum\" class=\"myPlotData\"></div></td>
    <td><div id=\"$nameLabel$colnum\" class=\"draghandle\"></div></td>
    </tr>
    </table>
    </div>";
    $orderNum++;

    echo "<script type=\"text/javascript\">";
    echo "$fullname$colnum = [";

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
    echo "myData.push({data: $fullname$colnum, name: \"$name$colnum\"";

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
    echo ", stacked : ".(int)(sizeof($datanames) > 1);
    echo ", labeldiv: \"".$nameLabel.$colnum."\"";
    echo ", colnum: \"$colnum\"";
    echo " });\n";

    echo "</script>\n";
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
    printOneThing("time", array("time")
        , array("time"), $result, $nrows, $orderNum, $colnum);

    printOneThing("restarts" , array("restarts")
        , array("restart no."), $result, $nrows, $orderNum, $colnum);

    printOneThing("propsPerDec", array("propsPerDec")
        , array("avg. no. propagations per decision"), $result, $nrows, $orderNum, $colnum);

    printOneThing("branchDepth", array("branchDepth")
        , array("avg. branch depth"), $result, $nrows, $orderNum, $colnum);

    printOneThing("branchDepthDelta", array("branchDepthDelta")
        , array("avg. no. of levels backjumped"), $result, $nrows, $orderNum, $colnum);

    printOneThing("trailDepth", array("trailDepth")
        , array("avg. trail depth"), $result, $nrows, $orderNum, $colnum);

    printOneThing("trailDepthDelta", array("trailDepthDelta")
        , array("avg. trail depth delta"), $result, $nrows, $orderNum, $colnum);

    printOneThing("glue", array("glue")
        , array("newly learnt clauses avg. glue"), $result, $nrows, $orderNum, $colnum);

    printOneThing("size", array("size")
        , array("newly learnt clauses avg. size"), $result, $nrows, $orderNum, $colnum);

    printOneThing("resolutions", array("resolutions")
        , array("avg. no. resolutions for 1UIP"), $result, $nrows, $orderNum, $colnum);

    printOneThing("agility", array("agility")
        , array("avg. agility"), $result, $nrows, $orderNum, $colnum);

    printOneThing("flippedPercent", array("flippedPercent")
        , array("var polarity flipped %"), $result, $nrows, $orderNum, $colnum);

    printOneThing("conflAfterConfl", array("conflAfterConfl")
        , array("conflict after conflict %"), $result, $nrows, $orderNum, $colnum);

    /*printOneThing("conflAfterConflVar", array("conflAfterConfl")
        , array("conflict after conflict std dev %"), $result, $nrows, $orderNum, $colnum);*/

    printOneThing("watchListSizeTraversed", array("watchListSizeTraversed")
        , array("avg. traversed watchlist size"), $result, $nrows, $orderNum, $colnum);

    printOneThing("watchListSizeTraversedVar", array("watchListSizeTraversedVar")
        , array("avg. traversed watchlist size std dev"), $result, $nrows, $orderNum, $colnum);

    /*printOneThing("litPropagatedSomething", array("litPropagatedSomething")
        , array("literal propagated something with binary clauses %"), $result, $nrows, $orderNum, $colnum);*/

    printOneThing("replaced", array("replaced")
        , array("vars replaced"), $result, $nrows, $orderNum, $colnum);

    printOneThing("set", array("set")
        , array("vars set"), $result, $nrows, $orderNum, $colnum);

    printOneThing("polarity", array("varSetPos", "varSetNeg")
        , array("propagated polar pos %", "propagated polar neg %"), $result, $nrows, $orderNum, $colnum);

    printOneThing("learntsSt", array("learntUnits", "learntBins", "learntTris", "learntLongs")
        ,array("new learnts unit %", "new learnts bin %", "new learnts tri %", "new learnts long %"), $result, $nrows, $orderNum, $colnum);

    printOneThing("propSt", array("propBinIrred", "propBinRed", "propTri", "propLongIrred", "propLongRed")
        ,array("prop by bin irred %", "prop by bin red %", "prop by tri %", "prop by long irred %", "prop by long red %"), $result, $nrows, $orderNum, $colnum);

    printOneThing("conflSt", array("conflBinIrred", "conflBinRed", "conflTri", "conflLongIrred", "conflLongRed")
        ,array("confl by bin irred %", "confl by bin red %", "confl by tri %", "confl by long irred %", "confl by long red %"), $result, $nrows, $orderNum, $colnum);

    printOneThing("branchDepthSD", array("branchDepthSD")
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
        , array("std dev no. resolutions for 1UIP"), $result, $nrows, $orderNum, $colnum);

    return $orderNum;
}

$orderNum = printOneSolve($runID, 0, $maxconfl);
$orderNum = printOneSolve($runID2, 1, $maxconfl);
?>

<div class="block" id="blockSpecial0">
    <table id="plot-table-a">
    <tr>
    <td><div id="MYdrawingPad0Parent" class="myPlotData"><canvas id="MYdrawingPad0" width="420" height="100">no support for canvas</canvas></div></td>
    <td><div class="draghandle"><b>(0) Newly learnt clause size distribution. Bottom: unitary clause. Top: largest clause. Black: Many learnt. White: None learnt. Horizontal resolution: 1000 conflicts. Vertical resolution: 1 literal</b></div></td>
    </tr>
    </table>
</div>

<div class="block" id="blockSpecial1">
    <table id="plot-table-a">
    <tr>
    <td><div id="MYdrawingPad0Parent" class="myPlotData"><canvas id="MYdrawingPad1" width="420" height="100">no support for canvas</canvas></div></td>
    <td><div class="draghandle"><b>(1) Newly learnt clause size distribution. Bottom: unitary clause. Top: largest clause. Black: Many learnt. White: None learnt. Horizontal resolution: 1000 conflicts. Vertical resolution: 1 literal</b></div></td>
    </tr>
    </table>
</div>

<script type="text/javascript">
<?
function getMaxSize($runID)
{
    $query="
    SELECT max(size) as mymax FROM clauseSizeDistrib
    where (runID = $runID)
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

function getMinConflRestart($runID, $maxconfl)
{
    $query="
    SELECT min(conflicts) as mymin FROM `restart`
    where conflicts < $maxconfl
    and runID = $runID";
    $result=mysql_query($query);

    if (!$result) {
        die('Invalid query: ' . mysql_error());
    }
    return mysql_result($result, 0, "mymin");
}

$maxSize1 = getMaxSize($runID) - 1; //Because no use for size 0
$maxSize2 = getMaxSize($runID2) - 1; //Because no use for size 0
echo "var maxSize = [$maxSize1, $maxSize2];\n";
$maxConflDistrib = getMaxConflDistrib($runID, $runID2);
echo "var maxConflDistrib = $maxConflDistrib;\n";

$maxConflRestart = getMaxConflRestart($runID, $maxconfl);
$maxConflRestart2 = getMaxConflRestart($runID2, $maxconfl);
echo "var maxConflRestart = [$maxConflRestart, $maxConflRestart2];";

$minConflRestart = 0; //getMinConflRestart($runID, $maxconfl);
$minConflRestart2 = 0; //getMinConflRestart($runID2, $maxconfl);
echo "var minConflRestart = [$minConflRestart, $minConflRestart2];";

$statPer = 1000;
echo "var statPer = $statPer;\n";


function fillClauseDistrib($num, $runID, $maxConflDistrib, $maxSize, $statPer)
{
    echo "clauseDistrib.push([]);";
    $lastConflicts = 0;
    for($i = $statPer; $i <= $maxConflDistrib; $i += $statPer) {

        $query = "
        SELECT conflicts, num FROM clauseSizeDistrib
        where runID = $runID
        and conflicts = $i
        and size <= $maxSize
        and size > 0
        order by size";
        $result=mysql_query($query);
        if (!$result) {
            die('Invalid query: ' . mysql_error());
        }
        $nrows=mysql_numrows($result);

        $i2=0;
        echo "tmp = {conflStart: $lastConflicts, conflEnd : $i, height: [";
        while ($i2 < $nrows) {
            $numberOfCl = mysql_result($result, $i2, "num");
            echo "$numberOfCl";

            $i2++;
            if ($i2 < $nrows)
                echo ",";
        }
        echo "]};";
        echo "clauseDistrib[$num].push(tmp);\n";
        $lastConflicts = $i;
    }
}

echo "var clauseDistrib = [];\n";
fillClauseDistrib(0, $runID, $maxConflDistrib, $maxSize1, $statPer);
fillClauseDistrib(1, $runID2, $maxConflDistrib, $maxSize2, $statPer);

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
function todisplay(i,len)
{
if (i == len-1)
    return "Conflicts";
else
    return "";
};


//Draw graphs
gs = [];
origSizes = [];
var blockRedraw = false;
for (var i = 0; i < myData.length; i++) {
    gs.push(new Dygraph(
        document.getElementById(myData[i].name),
        myData[i].data
        , {
            stackedGraph: myData[i].stacked,
            labels: myData[i].labels,
            underlayCallback: function(canvas, area, g) {
                canvas.fillStyle = "rgba(105, 105, 185, 185)";
                //canvas.fillRect(0, area.y, 2, area.h);
                for(var k = 0; k < simplificationPoints[myData[i].colnum].length-1; k++) {
                    var bottom_left = g.toDomCoords(simplificationPoints[myData[i].colnum][k], -20);
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
            drawXAxis: false, //i == myData.length-1,
            legend: 'always',
            xlabel: false, //todisplay(i, myData.length),
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
function makeRect(x1, x2, y1, y2, relHeight, imgData)
{
    num = 255-relHeight*255.0;
    type = "rgb(" + Math.floor(num) + "," + Math.floor(num) + "," + Math.floor(num) + ")";
    imgData.fillStyle = type;
    imgData.strokeStyle = type;
    //imgData.strokeRect(x1, y1, (x2-x1), (y2-y1));
    imgData.fillRect(x1, y1, x2-x1, y2-y1);
}

function makeRect2(x1, x2, y1, y2, imgData)
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
    var vAY = new Array();
    numElementsVertical = data[0].height.length;
    for(i = numElementsVertical; i >= 0; i--) {
        vAY.push(Math.round(i*(height/numElementsVertical)));
    }
    vAY.push(0);

    lastXEnd = 0;
    for( i = 0 ; i < data.length ; i ++ ){
        maxHeight = 0;
        for(i2 = 0; i2 < data[i].height.length; i2++) {
            maxHeight = Math.max(maxHeight, data[i].height[i2]);
        }

        /*xStart = data[i].conflStart - from;
        xStart *= onePixelisConf;
        xStart += Xdelta;
        xStart = Math.max(0, xStart);
        xStart = Math.min(xStart, width);
        //document.write(data[i].conflStart + ", " + xStart +  "...");*/
        xStart = lastXEnd;

        xEnd = data[i].conflEnd - from;
        xEnd *= onePixelisConf;
        xEnd += Xdelta;
        xEnd = Math.max(0, xEnd);
        xEnd = Math.min(xEnd, width);
        xEnd = Math.round(xEnd);
        lastXEnd = xEnd;
        //document.write(data[i].conflEnd + ", " + xEnd + " || ");

        for(i2 = 0; i2 < data[i].height.length; i2++) {
            yStart = vAY[i2+1];
            yEnd = vAY[i2];

            if (data[i].height[i2] != 0) {
                //relHeight = Math.log(data[i][i2])/maxHeight;
                relHeight = data[i].height[i2]/maxHeight;
                //relHeight = (data[i].height[i2]*data[i].height[i2])/maxHeight;
            } else {
                relHeight  = 0;
            }

            makeRect(xStart, xEnd, yStart, yEnd, relHeight, ctx);
        }
    }

    points = [];
    for(var k = 0; k < simplificationPoints[num].length-1; k++) {
        var point = simplificationPoints[num][k] - from;
        point *= onePixelisConf;
        point += Xdelta;
        if (point > 0) {
            points.push(point);
            makeRect2(point, point+1, 0, height, ctx);
        }
    }
    //alert(points);
    //alert(width + " , " + height);
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


<h2>Search session statistics</h2>
<p>These charts show clause types learnt, propagations made, and conflicting clause types for each search session of <a href="http://www.cril.univ-artois.fr/SAT09/results/bench.php?idev=29&idbench=69562">mizh-md5-47-3.cnf</a>, i.e. the problem on the left column. Note that these are just per-session summary graphs of learnt clause/propagation by/conflict by data that is already present above.</p>

<?
function getLearntData($runID)
{
    print "<script type=\"text/javascript\">";
    //Get data for learnts
    $query="
    SELECT sum(learntUnits) as `unit`, sum(learntBins) as `bin`, sum(learntTris) as `tri`, sum(learntLongs) as `long`
    FROM `restart`
    where `runID` = $runID
    group by `simplifications`
    order by `simplifications`";

    //Gather results
    $result=mysql_query($query);
    if (!$result) {
        die('Invalid query: ' . mysql_error());
    }
    $nrows=mysql_numrows($result);


    //Write learnt data to 'learntData'
    $i=0;
    echo "var learntData = new Array();";
    //echo "var learntDataStacked = [];";
    while ($i < $nrows) {
        $simplifications=mysql_result($result, $i, "simplifications");
        $units=mysql_result($result, $i, "unit");
        $bins=mysql_result($result, $i, "bin");
        $tris=mysql_result($result, $i, "tri");
        $longs=mysql_result($result, $i, "long");

        echo "learntData.push([ ['unit', $units],['bin', $bins],['tri', $tris],['long', $longs] ]);\n";
        //echo "learntDataStacked.push([$simplifications, $units, $bins, $tris, $longs]);";
        $i++;
    }
    echo "</script>";

    return $nrows;
}

function getPropData($runID)
{
    print "<script type=\"text/javascript\">";
    //Get data for learnts
    $query="
    SELECT sum(propBinIrred) as `binIrred`, sum(propBinRed) as `binRed`, sum(propTri) as `tri`, sum(propLongIrred) as `longIrred`, sum(propLongRed) as `longRed`
    FROM `restart`
    where `runID` = $runID
    group by `simplifications`
    order by `simplifications`";

    //Gather results
    $result=mysql_query($query);
    if (!$result) {
        die('Invalid query: ' . mysql_error());
    }
    $nrows=mysql_numrows($result);


    //Write prop data to 'propData'
    $i=0;
    echo "var propData = new Array();";
    $divplacement = "";
    while ($i < $nrows) {
        $binIrred=mysql_result($result, $i, "binIrred");
        $binRed=mysql_result($result, $i, "binRed");
        $tri=mysql_result($result, $i, "tri");
        $longIrred=mysql_result($result, $i, "longIrred");
        $longRed=mysql_result($result, $i, "longRed");

        echo "propData.push([ ['bin irred.', $binIrred],['bin red.', $binRed]
        , ['tri', $tri],['long irred.', $longIrred],['long red.', $longRed] ]);\n";

        $i++;
    }
    echo "</script>";

    return $nrows;
}

function getConflData($runID)
{
    print "<script type=\"text/javascript\">";
    //Get data for learnts
    $query="
    SELECT sum(conflBinIrred) as `binIrred`, sum(conflBinRed) as `binRed`, sum(conflTri) as `tri`, sum(conflLongIrred) as `longIrred`, sum(conflLongRed) as `longRed`
    FROM `restart`
    where `runID` = $runID
    group by `simplifications`
    order by `simplifications`";

    //Gather results
    $result=mysql_query($query);
    if (!$result) {
        die('Invalid query: ' . mysql_error());
    }
    $nrows=mysql_numrows($result);


    //Write prop data to 'propData'
    $i=0;
    echo "var conflData = new Array();";
    $divplacement = "";
    while ($i < $nrows) {
        $binIrred=mysql_result($result, $i, "binIrred");
        $binRed=mysql_result($result, $i, "binRed");
        $tri=mysql_result($result, $i, "tri");
        $longIrred=mysql_result($result, $i, "longIrred");
        $longRed=mysql_result($result, $i, "longRed");

        echo "conflData.push([ ['bin irred.', $binIrred] ,['bin red.', $binRed],['tri', $tri]
        ,['long irred.', $longIrred],['long red.', $longRed] ]);\n";

        $i++;
    }
    echo "</script>";

    return $nrows;
}

$nrows = getLearntData($runID);
getPropData($runID);
getConflData($runID);


//End script, create tables
function createTable($nrows)
{
    $i = 0;
    echo "<table class=\"box-table-a\">\n";
    echo "<tr><th>Search session</th><th>Learnt Clause type</th><th>Propagation by</th><th>Conflicts by</th></tr>\n";
    while ($i < $nrows) {
        echo "<tr>\n";
        echo "<td width=\"1%\" style=\"text-align:right;\">".($i+1)."</td>\n";
        echo " <td width=\"30%\"><div id=\"learnt$i\" class=\"piechart\"></div></td>\n";
        echo " <td width=\"30%\"><div id=\"prop$i\" class=\"piechart\"></div></td>\n";
        echo " <td width=\"30%\"><div id=\"confl$i\" class=\"piechart\"></div></td>\n";
        echo "</tr>\n";
        $i++;
    };
    echo "</table>\n";
}
/*echo "<table id=\"plot-table-a\">";
echo "<tr><td>
    <div id=\"learntStatsStacked\" class=\"myPlotData2\"></div>
    </td><td valign=top>
    <div id=\"learntStatsStackedLabel\" class=\"myPlotLabel\"></div>
    </td></tr>";
echo "</table>";*/
createTable($nrows);
mysql_close();
?>
</script>

<script type="text/javascript">
/*var g = new Dygraph(
    document.getElementById("learntStatsStacked")
    , learntDataStacked
    , {
        labels: ['x', 'unit', 'bin', 'tri', 'long']
        , stackedGraph: true

        , highlightCircleSize: 2
        , strokeWidth: 1
        , strokeBorderWidth: 1
        , legend: 'always'
        , labelsDivStyles: {
            'text-align': 'right',
            'background': 'none'
        }
        , labelsDiv: document.getElementById("learntStatsStackedLabel")
        , labelsSeparateLines: true
        , labelsKMB: true
        , drawPoints: true
        , pointSize: 1.5
        , highlightCircleSize: 4
        , title: "Learnt Clause type"
    }
);*/

function drawChart(name, num, data) {
    chart = new Highcharts.Chart(
    {
        chart: {
            renderTo: name + num,
            plotBackgroundColor: null,
            plotBorderWidth: null,
            plotShadow: false,
            spacingTop: 30,
            spacingRight: 30,
            spacingBottom: 30,
            spacingLeft: 30
        },
        title: {
            text: ''
        },
        tooltip: {
            formatter: function() {
                return '<b>'+ this.point.name +'</b>: '+ this.y + '(' + this.percentage.toFixed(1) + '%)';
            }
        },
        credits: {
            enabled: false
        },
        plotOptions: {
            pie: {
                allowPointSelect: true,
                cursor: 'pointer',
                dataLabels: {
                    enabled: true,
                    color: '#000000',
                    distance: 30,
                    connectorColor: '#000000',
                    /*formatter: function() {
                        return '<b>'+ this.point.name +'</b>: '+ this.percentage +' %';
                    },*/
                    overflow: "justify"
                }
            }
        },
        series: [{
            type: 'pie',
            name: 'Learnt clause types',
            data: data[num]
        }],
        exporting: {
            enabled: false
        }
    });
};

var chart;
for(i = 0; i < learntData.length; i++) {
    drawChart("learnt", i, learntData);
}

for(i = 0; i < propData.length; i++) {
    drawChart("prop", i, propData);
}

for(i = 0; i < conflData.length; i++) {
    drawChart("confl", i, conflData);
}
</script>

<h2>Why did I do this?</h2>
<p>There has been some <a href="http://www-sr.informatik.uni-tuebingen.de/~sinz/DPvis/">past work</a> on statically visualizing SAT problems by <a href="http://www.carstensinz.de/">Carsten Sinz</a>, but not much on dynamic solving visualization - in fact, nothing comes to my mind that is comparable to what is above. However, the point of this excercise was not only to visually display dynamic solver behaviour. Rather, I think we could do dynamic analysis and heuristic adjustment instead of the static analysis and static heuristic selection as done by current <a href="http://www.jair.org/media/2490/live-2490-3923-jair.pdf">portifolio solvers</a>. Accordingly, CryptoMiniSat 3 has an extremely large set of options - e.g. swithcing between cleaning using glues, activities, clause sizes, or number of propagations+conflicts made by a clause is only a matter of setting a variable, and can be done on-the-fly. Problems tend to evolve as simplication and solving steps are made, so search heuristics should evolve with the problem.</p>

<h2>Future work</h2>
<p>Data displayed above is nothing but a very small percentage of data that is gathered during solving. In particular, no data at all is shown about simplifcations. Also, note that the data above displays only ~40/400 seconds of solving time on a <i>very</i> slow machine. Time-out for SAT competition, considering computing speed, is on the order of 20x more. Futhermore, there are probably better ways to present the data that is displayed. Future work should try to fix these shortcomings. You can either <a href="mailto:mate@srlabs.de">send me a mail</a> if you have an idea, or implement it yourself - all is up in the <a href="https://github.com/msoos/cryptominisat">GIT</a>, including SQL, PHP, HTML, CSS and more.</p>

<h2>The End</h2>
<p>If you enjoyed this visualization, there are two things you can do. First, tell me about your impressions  <a href="http://www.msoos.org/">here</a> and send the link to a friend. Second, you can <a href="http://www.srlabs.de">contact my employer</a>, and he will be happy to find a way for us to help you with your SAT problems.</p>

<h2>Acknowledgements</h2>
<p>I would like to thank my employer for letting me play with SAT, my collegue <a href="http://www.flickr.com/photos/lucamelette/">Luca Melette</a> for helping me with ideas and coding, <a href="http://folk.uio.no/vegardno/">Vegard Nossum</a> for the many discussions we had about visualization, <a href="http://www.inra.fr/mia/T/katsirelos/">George Katsirelos</a> for improvement ideas, <a href="http://www.cril.univ-artois.fr/~jabbour/">Said Jabbour</a> for further improvement ideas, <a href="http://dygraphs.com/">Dygraphs</a> for the visually pleasing graphs, <a href="http://www.michelhiemstra.nl/blog/igoogle-like-drag-drop-portal-v20/">Portal</a> for the drag-and-drop feature, <a href="http://www.highcharts.com/">Highcharts</a> for the pie charts and Edward Tufte for all his wonderful <a href="http://www.edwardtufte.com/tufte/books_vdqi">books</a>.</p>

<br/>
<p><small>Copyright <a href="http://www.msoos.org">Mate Soos</a>, 2012. Licensed under <a href="http://creativecommons.org/licenses/by-nc-sa/2.5/">CC BY-NC-SA 2.5</a></small></p>
</body>
</html>
