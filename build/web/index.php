<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Cryptominisat</title>
<style>

@import url(//fonts.googleapis.com/css?family=Yanone+Kaffeesatz:400,700);
@import url(style.css);
#example1         { min-height: 155px; }

</style>
<style type="text/css">
    .jqplot-data-label {
      /*color: #444;*/
/*      font-size: 1.1em;*/
    }
</style>
<link rel="stylesheet" type="text/css" href="jquery.jqplot.css" />
</head>
<body>
<h1>Cryptominisat 3</h1>

<h2>Replacing wordy authority with visible certainty</h2>
<p>Please select averaging level:
<input type="range" min="1" max="21" value="1" step="1" onchange="showValue(this.value)"/>
<span id="range">1</span>
<script type="text/javascript">
function showValue(newValue)
{
    document.getElementById("range").innerHTML=newValue;
    setRollPeriod(newValue);
}
</script>
</p>

<script src="danvk-dygraphs-5cfed8c/dygraph-dev.js"></script>
<script type="text/javascript">
var myDataFuncs=new Array();
var myDataNames=new Array();
</script>
<?
$runID = 2009009927556288153;
// Report all PHP errors (see changelog)
//error_reporting(E_ALL);
error_reporting(E_STRICT);

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

function printOneThing($name, $nicename, $data, $nrows)
{
    $fullname = $name."Data";
    //<p>$nicename</p>
    echo "<div id=\"$name\" style=\"width:790px; height:100px\"></div>";
    echo "<script type=\"text/javascript\">
    function $fullname() {
    return \"Conflicts,$nicename\\n";
    $i=0;
    while ($i < $nrows) {
        $conf=mysql_result($data, $i, "conflicts");
        $b=mysql_result($data, $i, $name);
        if ($i == $nrows-1) {
            echo "$conf, $b;\"};\n";
        } else {
            echo "$conf, $b\\n";
        }
        $i++;
    }
    echo "myDataFuncs.push($fullname);\n";
    echo "myDataNames.push(\"$name\");\n";
    echo "</script>\n";
}
printOneThing("time", "time", $result, $nrows);
printOneThing("restarts", "restarts", $result, $nrows);
//printOneThing("simplifications", "simplifications", $result, $nrows);
printOneThing("branchDepth", "branch depth", $result, $nrows);
printOneThing("branchDepthDelta", "branch depth delta", $result, $nrows);
printOneThing("trailDepth", "trail depth", $result, $nrows);
printOneThing("glue", "glue", $result, $nrows);
printOneThing("size", "size", $result, $nrows);
printOneThing("resolutions", "resolutions", $result, $nrows);

function printVars($runID) {
    $name = "vars";
    $fullname = $name."Data";
    echo "<div id=\"$name\" style=\"width:790px; height:200px\"></div>";
    echo "<script type=\"text/javascript\">
    function $fullname() {
    return \"Conflicts,vars replaced,vars set\\n";

    $query="
    SELECT `conflicts`, `replaced`, `set`
    from `vars`
    where `runID` = $runID
    order by `conflicts`";
    $result=mysql_query($query);
    if (!$result) {
        die('Invalid query: ' . mysql_error());
    }
    while($row=mysql_fetch_array($result)) {
        for($i = 0; $i < 3; $i++) {
            echo $row[$i];
            if ($i != 2)
                echo ", ";
        }
        echo "\\n";
    }
    echo ";\"};";
    echo "myDataFuncs.push($fullname);\n";
    echo "myDataNames.push(\"$name\");\n";
    echo "</script>\n";
}
printVars($runID);
?>

<script type="text/javascript">
gs = [];
var blockRedraw = false;
for (var i = 0; i <= myDataNames.length; i++) {
gs.push(
    new Dygraph(
        document.getElementById(myDataNames[i]),
        myDataFuncs[i]
        , {
            rollPeriod: 1,
            drawXAxis: false,
            //title: "valami",
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
    )
);
}

function setRollPeriod(num)
{
    for (var j = 0; j < myDataNames.length; j++) {
        gs[j].updateOptions( {
            rollPeriod: num
        } );
    }
}

<?
echo "gs[0].setAnnotations([";
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

$i=0;
while ($i < $nrows) {
    $conf=mysql_result($result, $i, "confl");
    $b=mysql_result($result, $i, "simpl");

    echo "{series: \"time\"
    , x: \"$conf\"
    , shortText: \"S$b\"
    , text: \"Simplification no. $b\"
    },";
    $i++;
}
echo "]);";

?>
</script>

<script type="text/javascript" src="jquery/jquery.min.js"></script>
<script type="text/javascript" src="jquery/jquery.jqplot.min.js"></script>
<script type="text/javascript" src="jquery/plugins/jqplot.pieRenderer.min.js"></script>
<script type="text/javascript" src="jquery/plugins/jqplot.donutRenderer.min.js"></script>


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
    echo "var learntData = [ ";
    $divplacement = "";
    while ($i < $nrows) {
        $units=mysql_result($result, $i, "units");
        $bins=mysql_result($result, $i, "bins");
        $tris=mysql_result($result, $i, "tris");
        $longs=mysql_result($result, $i, "longs");

        echo "[ ['units', $units],['bins', $bins],['tris', $tris],['longs', $longs] ]\n";

        if ($i != $nrows-1) {
            echo " , ";
        }
        $i++;
    }
    echo "];\n";
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
    echo "var propData = [ ";
    $divplacement = "";
    while ($i < $nrows) {
        $unit=mysql_result($result, $i, "unit");
        $binIrred=mysql_result($result, $i, "binIrred");
        $binRed=mysql_result($result, $i, "binRed");
        $tri=mysql_result($result, $i, "tri");
        $longIrred=mysql_result($result, $i, "longIrred");
        $longRed=mysql_result($result, $i, "longRed");

        echo "[ ['unit', $unit],['bin irred.', $binIrred],['bin red.', $binRed]
        , ['tri', $tri],['long irred.', $longIrred],['long red.', $longRed] ]\n";

        if ($i != $nrows-1) {
            echo " , ";
        }
        $i++;
    }
    echo "];\n";
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
    echo "var conflData = [ ";
    $divplacement = "";
    while ($i < $nrows) {
        $binIrred=mysql_result($result, $i, "binIrred");
        $binRed=mysql_result($result, $i, "binRed");
        $tri=mysql_result($result, $i, "tri");
        $longIrred=mysql_result($result, $i, "longIrred");
        $longRed=mysql_result($result, $i, "longRed");

        echo "[ ['bin irred.', $binIrred] ,['bin red.', $binRed],['tri', $tri]
        ,['long irred.', $longIrred],['long red.', $longRed] ]\n";

        if ($i != $nrows-1) {
            echo " , ";
        }
        $i++;
    }
    echo "];\n";
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
    echo "
    <table border=\"1\">
    <tr>
    <td>simp</td>
    <td>Learnt clauses</td>
    <td>Propagations by</td>
    <td>Conflict by</td>
    </tr>";
    $height = 250;
    $width = 250;
    while ($i < $nrows) {
        echo "
        <tr>
        <td>$i</td>";
        echo "<td><div id=\"learnt$i\" style=\"height:".$height."px; width:".$width."px;\"></div></td>\n";
        echo "<td><div id=\"prop$i\" style=\"height:".$height."px; width:".$width."px;\"></div></td>\n";
        echo "<td><div id=\"confl$i\" style=\"height:".$height."px; width:".$width."px;\"></div></td>\n";
        echo "</tr>";
        $i++;
    }
    echo "</table>";
}
createTable($nrows);
?>

<script type="text/javascript">

//Part chart definitions
function pieChart(name, data, num)
{
    var name = name+num;
    jQuery.jqplot (name, [data[num]],
    {
        //title: "Learnt clauses after simplification number " + num,
        /*axesDefaults: {
            pad: 0
        },*/
        seriesDefaults: {
            // Make this a pie chart.
            renderer: jQuery.jqplot.PieRenderer,
            rendererOptions: {
              showDataLabels: false
              , barPadding: 0
              , padding: 0
            },
            shadow: false
        },
        legend: {
            show: true
            , location: 'e'
        },
        grid: {
            borderWidth: 0
            , shadow: false
            , drawGridLines: false
        }
    });
}

//Go through all pie charts
for(var i = 0; i < learntData.length; i++) {
    pieChart("learnt", learntData, i);
    pieChart("prop", propData, i);
    pieChart("confl", conflData, i);
}
</script>


<?
mysql_close();
?>



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

