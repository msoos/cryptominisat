<DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Cryptominisat</title>
    <style>

    @import url(//fonts.googleapis.com/css?family=Yanone+Kaffeesatz:400,700);
    @import url(style.css);
    @import url(tables/style.css);
    #example1         { min-height: 155px; }

    </style>
    <style type="text/css">
        .jqplot-data-label {
          /*color: #444;*/
    /*      font-size: 1.1em;*/
        }
    </style>

    <link rel="stylesheet" type="text/css" href="jquery.jqplot.css" />
    <script type="text/javascript" src="jquery/jquery.min.js"></script>
<!--     <script type="text/javascript" src="jquery/jquery.jqplot.min.js"></script> -->
<!--     <script type="text/javascript" src="jquery/plugins/jqplot.pieRenderer.min.js"></script> -->
<!--     <script type="text/javascript" src="jquery/plugins/jqplot.donutRenderer.min.js"></script> -->
    <script type="text/javascript" src="dygraphs/dygraph-dev.js"></script>
    <script type="text/javascript" src="highcharts/js/highcharts.js"></script>
<!--     <script type="text/javascript" src="highcharts/js/modules/exporting.js"></script> -->
</head>

<body>
<h1>Cryptominisat 3</h1>

<h3>Replacing wordy authority with visible certainty</h4>
<p>This webpage shows the solving of a SAT instance, visually. I was amazed by Edward Tufte's work  while others in the office were dazzled by a PowerPoint professional, in German. Tufte would probably not approve, as some of the layout is terrible. However, it might allow you to understand SAT better, and may offer inspiration... or, rather, <i>vision</i>. Enjoy.</p>


<!--<p>Please select averaging level:
<input type="range" min="1" max="21" value="1" step="1" onchange="showValue(this.value)"/>
<span id="range">1</span>
</p>-->
<h2>Search restart statistics</h2>
<p>Below you will find conflicts in the X axis and several interesting data on the Y axis. Every datapoint corresponds to a restart. You may zoom in by clicking on an interesting point and dragging the cursor along the X axis. Double-click to unzoom. Blue vertical lines indicate the positions of <i>simplification sessions</i>. Between the blue lines are what I call <i>search sessions</i>. The angle of the "time" graph indicates how much time is spent per restart. Time jumps during search sessions. The angle of the "restart no." graph indicates how often restarts were made.</p>

<script type="text/javascript">
function showValue(newValue)
{
    document.getElementById("range").innerHTML=newValue;
    setRollPeriod(newValue);
}
</script>


<script type="text/javascript">
var myDataFuncs=new Array();
var myDataNames=new Array();
var myDataTypes=new Array();
var myDataLabels=new Array();
</script>
<?
$runID = 1628198452;
//$runID = 3097911473;
//$runID = 456297562;
//$runID = 657697659;
//$runID = 3348265795;
error_reporting(E_ALL);
//error_reporting(E_STRICT);

$username="presenter";
$password="presenter";
$database="cryptoms";

mysql_connect(localhost, $username, $password);
@mysql_select_db($database) or die( "Unable to select database");

$query="
SELECT *
FROM `restart`
where `runID` = $runID
order by `conflicts`";

$result=mysql_query($query);
if (!$result) {
    die('Invalid query: ' . mysql_error());
}
$nrows=mysql_numrows($result);

/*function printOneThing($name, $nicename, $data, $nrows)
{
    $fullname = $name."Data";
    $nameLabel = $name."Label";
    echo "<tr><td>
    <div id=\"$name\" class=\"myPlotData\"></div>
    </td><td valign=top>
    <div id=\"$nameLabel\" class=\"myPlotLabel\"></div>
    </td></tr>";
    echo "<script type=\"text/javascript\">
    $fullname = [";

    $i=0;
    while ($i < $nrows) {
        $conf=mysql_result($data, $i, "conflicts");
        $b=mysql_result($data, $i, $name);
        echo "[$conf, $b]\n";

        $i++;
        if ($i < $nrows) {
            echo ",";
        }
    }
    echo "];";

    echo "myDataFuncs.push($fullname);\n";
    echo "myDataNames.push(\"$name\");\n";
    echo "myDataLabels.push([\"Conflicts\", \"$nicename\"]);\n";
    echo "myDataTypes.push(0);\n";
    echo "</script>\n";
}*/

function printOneThing($name, $datanames, $nicedatanames, $data, $nrows)
{
    $fullname = $name."Data";
    $nameLabel = $name."Label";
    echo "<tr><td>
    <div id=\"$name\" class=\"myPlotData\"></div>
    </td><td valign=top>
    <div id=\"$nameLabel\" class=\"myPlotLabel\"></div>
    </td></tr>";
    echo "<script type=\"text/javascript\">";
    echo "$fullname = [";

    $i=0;
    while ($i < $nrows) {
        $conf=mysql_result($data, $i, "conflicts");
        echo "[$conf";
        $sum = 0;
        $num = 0;
        foreach ($datanames as $dataname) {
            $sum += mysql_result($data, $i, $dataname);
            $num++;
        }
        foreach ($datanames as $dataname) {
            if ($num > 1) {
                $tmp=mysql_result($data, $i, $dataname)/$sum*100.0;
            } else {
                $tmp=mysql_result($data, $i, $dataname);
            }
            echo ", $tmp";
        }
        echo "]\n";

        $i++;
        if ($i < $nrows) {
            echo ",";
        }
    }
    echo "];\n";
    echo "myDataFuncs.push($fullname);\n";
    echo "myDataNames.push(\"$name\");\n";
    echo "myDataLabels.push([\"Conflicts\"";
    $num = 0;
    foreach ($nicedatanames as $dataname) {
        $num++;
        echo ", \"$dataname\"";
    }
    echo "]);\n";
    echo "myDataTypes.push(".(int)($num > 1).");\n";
    echo "</script>\n";
}

echo "<table id=\"plot-table-a\">";
printOneThing("time", array("time")
    , array("time"), $result, $nrows);
    
printOneThing("restarts" , array("restarts")
    , array("restart no."), $result, $nrows);

printOneThing("branchDepth", array("branchDepth")
    , array("avg. branch depth"), $result, $nrows);

printOneThing("branchDepthDelta", array("branchDepthDelta")
    , array("avg. branch depth delta"), $result, $nrows);
    
printOneThing("trailDepth", array("trailDepth")
    , array("avg. trail depth"), $result, $nrows);

printOneThing("trailDepthDelta", array("trailDepthDelta")
    , array("avg. trail depth delta"), $result, $nrows);

printOneThing("glue", array("glue")
    , array("avg. glue"), $result, $nrows);

printOneThing("size", array("size")
    , array("avg. size"), $result, $nrows);

printOneThing("resolutions", array("resolutions")
    , array("avg. no. resolutions"), $result, $nrows);

printOneThing("learntsSt", array("learntUnits", "learntBins", "learntTris", "learntLongs")
    ,array("unit learnts %", "bin learnts %", "tri learnts %", "long learnts %"), $result, $nrows);

printOneThing("propSt", array("propBinIrred", "propBinRed", "propTri", "propLongIrred", "propLongRed")
    ,array("prop by bin irred %", "prop by bin red %", "prop by tri %", "prop by long irred %", "prop by long red %"), $result, $nrows);

printOneThing("conflSt", array("conflBinIrred", "conflBinRed", "conflTri", "conflLongIrred", "conflLongRed")
    ,array("confl by bin irred %", "confl by bin red %", "confl by tri %", "confl by long irred %", "confl by long red %"), $result, $nrows);

$query="
SELECT `conflicts`, `replaced`, `set`
from `vars`
where `runID` = $runID
order by `conflicts`";
$result=mysql_query($query);
if (!$result) {
    die('Invalid query: ' . mysql_error());
}
$nrows=mysql_numrows($result);
printOneThing("set", array("set")
    , array("vars set"), $result, $nrows);

printOneThing("replaced", array("replaced")
    , array("vars replaced"), $result, $nrows);
echo "</table>";

function fillSimplificationPoints($runID)
{
    $query="
    SELECT max(conflicts) as confl, simplifications as simpl
    FROM restart
    where runID = $runID
    group by simplifications";

    $result=mysql_query($query);
    if (!$result) {
        die('Invalid query: ' . mysql_error());
    }
    $nrows=mysql_numrows($result);

    echo "simplificationPoints = [";
    $i=0;
    while ($i < $nrows) {
        $conf=mysql_result($result, $i, "confl");
        echo "$conf";
        $i++;
        if ($i < $nrows) {
            echo ", ";
        }
    }
    echo "];";
}
echo "<script type=\"text/javascript\">";
fillSimplificationPoints($runID);
echo "</script>\n";
?>

<script type="text/javascript">
function todisplay(i,len)
{
if (i == len-1)
    return "Conflicts";
else
    return "";
};

gs = [];
var blockRedraw = false;
for (var i = 0; i <= myDataNames.length; i++) {
    gs.push(new Dygraph(
        document.getElementById(myDataNames[i]),
        myDataFuncs[i]
        , {
            stackedGraph: myDataTypes[i],
            labels: myDataLabels[i],
            underlayCallback: function(canvas, area, g) {
                canvas.fillStyle = "rgba(185, 185, 245, 245)";
                for(var k = 0; k <= simplificationPoints.length; k++) {
                    var bottom_left = g.toDomCoords(simplificationPoints[k], -20);
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
              }
            },
            //stepPlot: true,
            strokePattern: [0.1, 0, 0, 0.5],
            strokeWidth: 2,
            highlightCircleSize: 3,
            rollPeriod: 1,
            drawXAxis: i == myDataNames.length-1,
            legend: 'always',
            xlabel: todisplay(i, myDataNames.length),
            labelsDivStyles: {
                'text-align': 'right',
                'background': 'none'
            },
            labelsDiv: document.getElementById(myDataNames[i]+"Label"),
            labelsSeparateLines: true,
            labelsKMB: true,
            drawPoints: true,
            pointSize: 1,
            drawXGrid: false,
            drawYGrid: false,
            //errorBars: false,
            drawCallback: function(me, initial) {
                if (blockRedraw || initial)
                    return;

                blockRedraw = true;
                var xrange = me.xAxisRange();
                //var yrange = me.yAxisRange();
                for (var j = 0; j < myDataNames.length; j++) {
                    if (gs[j] == me)
                        continue;

                    gs[j].updateOptions( {
                        dateWindow: xrange
                    } );
                }
                blockRedraw = false;
            }
        }
    ));
}

/*for (i = 0; i <= myDataAnnotations.length; i++)  {
    gs[i].setAnnotations(myDataAnnotations[i]);
}*/

function setRollPeriod(num)
{
    for (var j = 0; j < myDataNames.length; j++) {
        gs[j].updateOptions( {
            rollPeriod: num
        } );
    }
}
</script>

<script type="text/javascript">
var clauseStatsData=new Array();
</script>

<!-- <h2>Clause statistics before each clause database cleaning</h2> -->
<?
/*function createDataClauseStats($reduceDB, $runID)
{
    $query="
    SELECT
        sum(`numPropAndConfl`) as mysum
        , avg(`numPropAndConfl`) as myavg
        , count(`numPropAndConfl`) as mycnt
        , `size`
    FROM `clauseStats`
    where `runID` = $runID and `reduceDB`= $reduceDB and `learnt` = 1
    group by `size`
    order by `size`
    limit 200";
    //and `numPropAndConfl` > 0

    $result=mysql_query($query);
    if (!$result) {
        die('Invalid query: ' . mysql_error());
    }
    $nrows=mysql_numrows($result);

    echo "clauseStatsData.push([\n";
    $i=0;
    $numprinted = 0;
    while ($i < $nrows) {
        $myavg=mysql_result($result, $i, "myavg");
        $mysum=mysql_result($result, $i, "mysum");
        $mycnt=mysql_result($result, $i, "mycnt");
        $size=mysql_result($result, $i, "size");
        if ($mycnt < 10 || $mycnt == 0 || $size <= 3) {
            $i++;
            continue;
        }
        $numprinted++;
        if ($numprinted > 1) {
            echo ",";
        }

        echo "[$size, $myavg]\n";

        $i++;
    }
    echo "]);\n";
}

//Get maximum number of simplifications
$query="
SELECT max(reduceDB) as mymax
FROM `clauseStats`
where `runID` = $runID";
$result=mysql_query($query);
if (!$result) {
    die('Invalid query: ' . mysql_error());
}
$maxNumReduceDB = mysql_result($result, 0, "mymax");

echo "<script type=\"text/javascript\">\n";
for($i = 1; $i < $maxNumReduceDB; $i++) {
    createDataClauseStats($i, $runID);
}
echo "</script>\n";

echo "<table id=\"plot-table-a\">";
for($i = 1; $i < $maxNumReduceDB; $i++) {
    echo "<tr><td>
    <div id=\"clauseStatsPlot$i\" class=\"myPlotData\"></div>
    </td><td valign=top>
    <div id=\"clauseStatsPlotLabel$i\" class=\"myPlotLabel\"></div>
    </td></tr>";
}
echo "</tr></table>";
*/
?>

<script type="text/javascript">
for(i = 0; i < clauseStatsData.length; i++) {
    var i2 = i+1;
    var gzz = new Dygraph(
        document.getElementById('clauseStatsPlot' + i2),
        clauseStatsData[i],
        {
            drawXAxis: i == clauseStatsData.length-1,
            legend: 'always',
            labels: ['size', 'num prop&confl'],
            connectSeparatedPoints: true,
            drawPoints: true,
            labelsDivStyles: {
                'text-align': 'right',
                'background': 'none'
            },
            labelsDiv: document.getElementById('clauseStatsPlotLabel'+ i2),
            labelsSeparateLines: true,
            labelsKMB: true
            //,title: "Most propagating&conflicting clauses before clause clean " + i
        }
    );
}
var varPolarsData = new Array();
</script>



<h2>Variable statistics per search session</h2>
<p> These graphs show how many times the topmost set variables were set to positive or negative polarity. Also, it shows how many times they were flipped, relative to their stored, old polarity.</p>
<?
function createDataVarPolars($simpnum, $runID)
{
    $query="
    SELECT *
    FROM `polarSet`
    where `runID` = $runID and `simplifications`= $simpnum
    order by `order`
    limit 200";

    $result=mysql_query($query);
    if (!$result) {
        die('Invalid query: ' . mysql_error());
    }
    $nrows=mysql_numrows($result);

    echo "varPolarsData.push([\n";
    $i=0;
    while ($i < $nrows) {
        $order=mysql_result($result, $i, "order");
        $pos=mysql_result($result, $i, "pos");
        $neg=mysql_result($result, $i, "neg");
        $total=mysql_result($result, $i, "total");
        $flipped=mysql_result($result, $i, "flipped");

        echo "[$order, $pos, $neg, $total, $flipped]\n";

        $i++;
        if ($i < $nrows) {
            echo ",";
        }
    }
    echo "]);\n";
}

//Get maximum number of simplifications
$query="
SELECT max(simplifications) as mymax
FROM `polarSet`
where `runID` = $runID";
$result=mysql_query($query);
if (!$result) {
    die('Invalid query: ' . mysql_error());
}
$maxNumSimp = mysql_result($result, 0, "mymax");

echo "<script type=\"text/javascript\">\n";
for($i = 1; $i < $maxNumSimp; $i++) {
    createDataVarPolars($i, $runID);
}
echo "</script>\n";

echo "<table id=\"plot-table-a\">";
for($i = 1; $i < $maxNumSimp; $i++) {
    echo "<tr><td>
    <div id=\"varPolarsPlot$i\" class=\"myPlotData2\"></div>
    </td><td valign=top>
    <div id=\"varPolarsPlotLabel$i\" class=\"myPlotLabel\"></div>
    </td></tr>";
}
echo "</tr></table>";
?>

<script type="text/javascript">
for(i = 0; i < varPolarsData.length; i++) {
    var i2 = i+1;
    var gzz = new Dygraph(
        document.getElementById('varPolarsPlot' + i2),
        varPolarsData[i],
        {
            legend: 'always',
            labels: ['no.', 'pos polar', 'neg polar', 'total set', 'flipped polar' ],
            connectSeparatedPoints: true,
            drawPoints: true,
            labelsDivStyles: {
                'text-align': 'right',
                'background': 'none'
            },
            labelsDiv: document.getElementById('varPolarsPlotLabel'+ i2),
            labelsSeparateLines: true,
            labelsKMB: true
//             ,xlabel: "Top 200 most set variables",
            //,title: "Most set variables during search session " + i
        }
    );
}
</script>


<h2>Search session statistics</h2>
<p>Here are some pie charts detailing propagations and other stats for each search. "red." means reducible, also called "learnt", while "irred." means irreducible, also called "non-learnt". Terminology shamelessly taken from A. Biere.</p>

<?


?>

<?
function getLearntData($runID)
{
    print "<script type=\"text/javascript\">";
    //Get data for learnts
    $query="
    SELECT *
    FROM `learnts`
    where `runID` = $runID
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
        $units=mysql_result($result, $i, "units");
        $bins=mysql_result($result, $i, "bins");
        $tris=mysql_result($result, $i, "tris");
        $longs=mysql_result($result, $i, "longs");

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
    SELECT *
    FROM `props`
    where `runID` = $runID
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
        $unit=mysql_result($result, $i, "unit");
        $binIrred=mysql_result($result, $i, "binIrred");
        $binRed=mysql_result($result, $i, "binRed");
        $tri=mysql_result($result, $i, "tri");
        $longIrred=mysql_result($result, $i, "longIrred");
        $longRed=mysql_result($result, $i, "longRed");

        echo "propData.push([ ['unit', $unit],['bin irred.', $binIrred],['bin red.', $binRed]
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
    SELECT *
    FROM `confls`
    where `runID` = $runID
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
    echo "<table id=\"box-table-a\">\n";
    echo "<tr><th>Search session</th><th>Learnt Clause type</th><th>Propagation by</th><th>Conflicts by</th></tr>\n";
    while ($i < $nrows) {
        echo "<tr>\n";
        echo "<td width=\"1%\">$i</td>\n";
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

<h2>The End</h2>
<p>If you enjoyed this visualization, there are two things you can do. First, enjoy, and send it to others. Second, you can contact my employer, and he will be happy to find a way for us to help you with your SAT problems.</p>


<!--<div id="fig" style="width:20px; height:20px"></div>
<script type="text/javascript" src="mbostock-protovis-1a61bac/protovis.min.js"></script>
<script type="text/javascript+protovis">

// Sizing and scales
var data = pv.range(5).map(Math.random);
var w = 200,
    h = 200,
    r = w / 2,
    a = pv.Scale.linear(0, pv.sum(data)).range(0, 2 * Math.PI);

// The root panel.
var vis = new pv.Panel()
    .width(w)
    .height(h);

// The wedge, with centered label.
vis.add(pv.Wedge)
    .data(data.sort(pv.reverseOrder))
    .bottom(w / 2)
    .left(w / 2)
    .innerRadius(r - 40)
    .outerRadius(r)
    .angle(a)
    .event("mouseover", function() this.innerRadius(0))
    .event("mouseout", function() this.innerRadius(r - 40))
    .anchor("center").add(pv.Label)
    .visible(function(d) d > .15)
    .textAngle(0)
    .text(function(d) d.toFixed(2));

vis.render();
</script>-->

<!--<div id="example1"></div>

<script src="d3/d3.v2.js"></script>
<script src="cubism/cubism.v1.js"></script>
<script>
function myvalues(name) {
  var value = 0,
      values = [],
      i = 0,
      last;
  return context.metric(function(start, stop, step, callback) {
    start = +start;
    stop = +stop;
    if (isNaN(last))
        last = start;

    while (last < stop) {
      last += step;
      //value = Math.max(-10, Math.min(10, value + .8 * Math.random() - .4 + .2 * Math.cos(i += .2)));
      values.push(10);
    }
    callback(null, values = values.slice((start - stop) / step));
  }, name);
}

var context = cubism.context()
    .serverDelay(0)
    .clientDelay(0)
    //.step(10)
    .size(800);

//    var foo = random("glue");
var foo = myvalues("glue");

d3.select("#example1").call(function(div) {

  div.append("div")
      .attr("class", "axis")
      .call(context.axis().orient("top"));

  div.selectAll(".horizon")
      .data([foo])
    .enter().append("div")
      .attr("class", "horizon")
      .call(context.horizon().extent([-20, 20]));

  div.append("div")
      .attr("class", "rule")
      .call(context.rule());

});

// On mousemove, reposition the chart values to match the rule.
context.on("focus", function(i) {
  d3.selectAll(".value").style("right", i == null ? null : context.size() - i + "px");
});
</script>-->


</html>


