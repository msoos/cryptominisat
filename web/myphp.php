<?
$runIDs = array(49);
$maxConfl = 40000;
error_reporting(E_ALL | E_STRICT);
ini_set('display_errors',1);
//display_startup_errors(1);
//error_reporting(E_STRICT);
//asfdasfdsf

$username="presenter";
$password="presenter";
$database="cryptoms";

mysql_connect("localhost", $username, $password);
@mysql_select_db($database) or die( "Unable to select database");

class DataPrinter
{
    protected $numberingScheme;
    protected $data;
    protected $nrows;
    protected $colnum;
    protected $runID;
    protected $maxConfl;

    public function __construct($mycolnum, $myRunID, $myMaxConfl)
    {
        $this->colnum = $mycolnum;
        $this->runID = $myRunID;
        $this->maxConfl = $myMaxConfl;
        $this->numberingScheme = 0;
        $this->runQuery();

        echo "
        <script type=\"text/javascript\">
        if (columnDivs.length <= ".$this->colnum.")
            columnDivs.push(new Array());
        </script>
        ";
    }

    public function maxConflRestart()
    {
        $query="
        SELECT max(conflicts) as mymax FROM `restart`
        where conflicts < ".$this->maxConfl."
        and runID = ".$this->runID.";";
        $result=mysql_query($query);

        if (!$result) {
            die('Invalid query: ' . mysql_error());
        }

        $max = mysql_result($result, 0, "mymax");
        echo "<script type=\"text/javascript\">\n";
        echo "maxConflRestart.push($max);";
        echo "</script>";
    }

    protected function printOneThing(
        $datanames
        , $nicedatanames
        , $doSlideAvg = 0
    ) {
        $fullname = "toplot_".$this->numberingScheme."_".$this->colnum;

        echo "<script type=\"text/javascript\">\n";
        echo "tmp = [";

        //Start with an empty one
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

        //Now go through it all
        $i=0;
        $total_sum = 0.0;
        $last_confl = 0.0;
        while ($i < $this->nrows) {
            //Print conflicts
            $confl=mysql_result($this->data, $i, "conflicts");
            echo "[$confl";

            //Calc local sum
            $local_sum = 0;
            foreach ($datanames as $dataname) {
                $local_sum += mysql_result($this->data, $i, $dataname);
            }

            //Print for each
            foreach ($datanames as $dataname) {
                $tmp = mysql_result($this->data, $i, $dataname);
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
            if ($i < $this->nrows) {
                echo ",";
            }
        }
        echo "];\n";

        //Add name & data
        echo "myData.push({data: tmp ";

        //Calculate labels
        echo ", labels: [\"Conflicts\"";
        foreach ($nicedatanames as $dataname) {
            echo ", \"(".$this->colnum.") $dataname\"";
        }
        if (sizeof($datanames) == 1) {
            echo ", \"".$fullname.$this->colnum." (sliding avg.)\"";
        }
        echo "]";

        //Rest of the info
        echo ", stacked: ".(int)(sizeof($datanames) > 1);
        echo ", colnum: \"".$this->colnum."\"";

        //DIVs
        $blockDivID = "graphBlock".$this->numberingScheme."AT".$this->colnum;
        $dataDivID = $fullname."_datadiv";
        $labelDivID = $fullname."_labeldiv";
        echo ", blockDivID:  '$blockDivID'";
        echo ", dataDivID:  '$dataDivID'";
        echo ", labelDivID: '$labelDivID'";
        echo " });\n";

        //Put into columnDivs
        echo "tmp = {blockDivID:  '$blockDivID'};";
        echo "columnDivs[".$this->colnum."].push(tmp);";


        echo "</script>\n";
        $this->numberingScheme++;
    }

    public function runQuery()
    {
        $query="
        SELECT *
        FROM `restart`
        where `runID` = ".$this->runID."
        and conflicts < ".$this->maxConfl."
        order by `conflicts`";
        //echo "query was: $query";

        $this->data = mysql_query($query);
        if (!$this->data) {
            die('Invalid query: ' . mysql_error());
        }

        $this->nrows = mysql_numrows($this->data);
    }

    public function printOneSolve()
    {
        $this->printOneThing(array("time")
            , array("time"));

        $this->printOneThing(array("restarts")
            , array("restart no."));

        /*printOneThing(array("propsPerDec")
            , array("avg. no. propagations per decision"));*/

        $this->printOneThing(array("branchDepth")
            , array("avg. branch depth"));

        $this->printOneThing(array("branchDepthDelta")
            , array("avg. no. of levels backjumped"));

        $this->printOneThing(array("trailDepth")
            , array("avg. trail depth"));

        $this->printOneThing(array("trailDepthDelta")
            , array("avg. trail depth delta"));

        $this->printOneThing(array("glue")
            , array("newly learnt clauses avg. glue"));

        $this->printOneThing(array("size")
            , array("newly learnt clauses avg. size"));

        $this->printOneThing(array("resolutions")
            , array("avg. no. resolutions for 1UIP"));

        $this->printOneThing(array("agility")
            , array("avg. agility"));

        /*printOneThing(array("flippedPercent")
            , array("var polarity flipped %"));*/

        $this->printOneThing(array("conflAfterConfl")
            , array("conflict after conflict %"));

        /*printOneThing("conflAfterConflSD", array("conflAfterConfl")
            , array("conflict after conflict std dev %"));*/

        $this->printOneThing(array("watchListSizeTraversed")
            , array("avg. traversed watchlist size"));

        /*printOneThing(array("watchListSizeTraversedSD")
            , array("avg. traversed watchlist size std dev"));*/

        /*printOneThing("litPropagatedSomething", array("litPropagatedSomething")
            , array("literal propagated something with binary clauses %"));*/

        $this->printOneThing(array("replaced")
            , array("vars replaced"));

        $this->printOneThing(array("set")
            , array("vars set"));

        $this->printOneThing(array(
            "varSetPos"
            , "varSetNeg"
            )
            , array(
            "propagated polar pos %"
            , "propagated polar neg %")
        );

        $this->printOneThing(array(
            "learntUnits"
            , "learntBins"
            , "learntTris"
            , "learntLongs"
            )
            ,array(
            "new learnts unit %"
            , "new learnts bin %"
            , "new learnts tri %"
            , "new learnts long %")
        );

        $this->printOneThing(array(
            "propBinIrred", "propBinRed"
            , "propTriIrred", "propTriRed"
            , "propLongIrred", "propLongRed"
            )
            ,array("prop by bin irred %", "prop by bin red %"
            , "prop by tri irred %", "prop by tri red %"
            , "prop by long irred %", "prop by long red %"
            )
        );

        $this->printOneThing(array(
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
        );

        /*printOneThing("branchDepthSD", array("branchDepthSD")
            , array("branch depth std dev"));

        printOneThing("branchDepthDeltaSD", array("branchDepthDeltaSD")
            , array("branch depth delta std dev"));

        printOneThing("trailDepthSD", array("trailDepthSD")
            , array("trail depth std dev"));

        printOneThing("trailDepthDeltaSD", array("trailDepthDeltaSD")
            , array("trail depth delta std dev"));

        printOneThing("glueSD", array("glueSD")
            , array("newly learnt clause glue std dev"));

        printOneThing("sizeSD", array("sizeSD")
            , array("newly learnt clause size std dev"));

        printOneThing("resolutionsSD", array("resolutionsSD")
            , array("std dev no. resolutions for 1UIP"));*/

        return $this->numberingScheme;
    }
}

for($i = 0; $i < count($runIDs); $i++) {
    $printer = new DataPrinter($i, $runIDs[$i], $maxConfl);
    $printer->maxConflRestart();
    $orderNum = $printer->printOneSolve();
}


class Simplifications
{
    protected $runIDs;

    public function __construct($myRunIDs)
    {
        $this->runIDs = $myRunIDs;
    }

    protected function fillSimplificationPoints($thisRunID)
    {
        $query="
        SELECT max(conflicts) as confl, simplifications as simpl
        FROM restart
        where runID = ".$thisRunID."
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

    public function fill()
    {
        echo '<script type="text/javascript">';
        foreach ($this->runIDs as $thisRunID) {
            $this->fillSimplificationPoints($thisRunID);
        }

        echo '</script>';
    }
}

$simps = new Simplifications($runIDs);
$simps->fill();

class ClauseSizeDistrib
{
    protected $colnum;
    protected $runID;
    protected $maxConfl;

    public function __construct($mycolnum, $myRunID, $myMaxConfl)
    {
        $this->colnum = $mycolnum;
        $this->runID = $myRunID;
        $this->maxConfl = $myMaxConfl;
    }

    public function printHTML()
    {

        /*echo "<div class=\"block\" id=\"$blockDivName\">
        <table id=\"plot-table-a\">
        <tr>
        <td>
            <div id=\"$dataDiv\" class=\"myPlotData\">
            <canvas id=\"MYdrawingPad".$this->colnum."\" width=\"420\" height=\"100\">
            no support for canvas</canvas>
            </div>";
        echo"
        </td>
        <td>'
            <div id=\"$labelDiv\" class=\"draghandle\"><b>
            (".$this->colnum.") Newly learnt clause size distribution.
            Bottom: unitary clause. Top: largest clause.
            Black: Many learnt. White: None learnt.
            Horizontal resolution: 1000 conflicts.
            Vertical resolution: 1 literal
            </b></div>
        </td>
        </tr>
        </table>
        </div>";*/

        /*echo "
        <script type=\"text/javascript\">
        tmp = {fullDiv:  '$blockDivName'
            ,  dataDiv:  '$dataDiv'
            ,  labelDiv: '$labelDiv'
            ,  canvasID: '$canvas'
            };
        columnDivs[".$this->colnum."].push(tmp);
        </script>
        ";*/
    }

    public function fillClauseDistrib()
    {
        echo '<script type="text/javascript">';
        echo "tmpArray = new Array();\n";

        $query = "
        SELECT conflicts, num FROM clauseSizeDistrib
        where runID = ".$this->runID."
        and conflicts < ".$this->maxConfl."
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
            echo "tmp = {conflStart: $lastConfl, conflEnd: $confl, darkness: [";
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
            echo "tmpArray.push(tmp);\n";
        }

        echo "clDistrib.push({data: tmpArray";

        $blockDivID = "distBlock".$this->colnum;
        $dataDivID = "drawingPad".$this->colnum."Parent";
        $canvasID = "drawingPad".$this->colnum;
        $labelDivID = "$blockDivID"."_labeldiv";
        echo ", blockDivID:  '$blockDivID'";
        echo ", dataDivID:  '$dataDivID'";
        echo ", canvasID: '$canvasID'";
        echo ", labelDivID: '$labelDivID'";
        echo "});\n";

        //Put into columnDivs
        echo "tmp = {blockDivID:  '$blockDivID'};";
        echo "columnDivs[".$this->colnum."].push(tmp);";

        echo '</script>';
    }
}

for($i = 0; $i < count($runIDs); $i++) {
    $myDist = new ClauseSizeDistrib($i, $runIDs[$i], $maxConfl);
    $myDist->fillClauseDistrib();
    $myDist->printHTML();
}

?>