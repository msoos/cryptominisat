<?php
error_reporting(E_ALL | E_STRICT);
ini_set('display_errors',1);
$maxConfl = 7000000;
//display_startup_errors(1);
//error_reporting(E_STRICT);

echo "<script type=\"text/javascript\">";
$username="cmsat_presenter";
$password="";
$database="cmsat";

mysql_connect("localhost", $username, $password);
@mysql_select_db($database) or die( "Unable to select database");

#$runIDs = array(mysql_real_escape_string($_GET["id"]));
$runIDs = array(getLatestID());

function getLatestID()
{
    $query = "select runID from `startup` order by startTime desc limit 1;";
    $result = mysql_query($query);

    if (!$result) {
        die('Invalid query: ' . mysql_error());
    }

    $nrows = mysql_numrows($result);
    if ($nrows < 1) {
        return 7401737;
    }

    $runID = mysql_result($result, 0, "runID");

    return $runID;
}

class DataPrinter
{
    protected $numberingScheme;
    protected $data;
    protected $nrows;
    protected $colnum;
    protected $runID;
    protected $max_confl;

    public function __construct($mycolnum, $runID, $maxConfl)
    {
        $this->colnum = $mycolnum;
        $this->runID = $runID;
        $this->numberingScheme = 0;
        $this->max_confl = $this->sql_get_max_restart($maxConfl);

        echo "
        if (columnDivs.length <= ".$this->colnum.")
            columnDivs.push(new Array());
        ";
    }

    private function sql_get_max_restart($maxConfl)
    {

        $query="
        SELECT max(conflicts) as mymax FROM `restart`
        where conflicts < $maxConfl
        and runID = ".$this->runID.";";
        $result=mysql_query($query);

        if (!$result) {
            die('Invalid query: ' . mysql_error());
        }

        $max = mysql_result($result, 0, "mymax");
        return intval($max);
    }

    public function get_max_confl()
    {
        return $this->max_confl;
    }

    protected function print_one_graph(
        $datanames
        , $nicedatanames
    ) {
        $fullname = "toplot_".$this->numberingScheme."_".$this->colnum;

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
                    if ($local_sum == 0) {
                        //$tmp = 0;
                    } else {
                        $tmp /= $local_sum;
                        $tmp *= 100.0;
                    }
                    echo ", $tmp";
                } else {
                    $total_sum += $tmp*($confl-$last_confl);
                    $last_confl = $confl;
                    echo ", $tmp";
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
        echo "data_tmp.push({data: tmp ";

        //Calculate labels
        echo ", labels: [\"Conflicts\"";
        foreach ($nicedatanames as $dataname) {
            echo ", \"(".$this->colnum.") $dataname\"";
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
        echo ", max_confl: '".$this->max_confl."'";
        echo " });\n";

        //Put into columnDivs
        echo "tmp = {blockDivID:  '$blockDivID'};";
        echo "columnDivs[".$this->colnum."].push(tmp);";

        $this->numberingScheme++;
    }

    function runQuery($table, $extra = "")
    {
        $query="
        SELECT *
        FROM `$table`
        where `runID` = ".$this->runID."
        and conflicts <= ".$this->max_confl." $extra
        order by `conflicts`";

        $this->data = mysql_query($query);
        if (!$this->data) {
            die('Invalid query: ' . mysql_error());
        }

        $this->nrows = mysql_numrows($this->data);
    }

    public function fill_data_tmp()
    {
        echo "data_tmp = new Array();";
        $this->runQuery("restart");

        $this->print_one_graph(array("time")
            , array("time"));

        $this->print_one_graph(array("restarts")
            , array("restart no."));

        /*print_one_graph(array("propsPerDec")
            , array("avg. no. propagations per decision"));*/

        $this->print_one_graph(array(
            "set"
            , "numIrredBins"
            , "numRedBins"
            , "numIrredTris"
            , "numRedTris"
            , "numIrredLongs"
            , "numRedLongs"
            )
            ,array(
            "unit cls %"
            , "irredBin %"
            , "redBin %"
            , "irredTri %"
            , "redTri  %"
            , "irredLong %"
            , "iredLong %"
            )
        );

        $this->print_one_graph(array("branchDepth")
            , array("avg. branch depth"));

        $this->print_one_graph(array("branchDepthDelta")
            , array("avg. no. of levels backjumped"));

        $this->print_one_graph(array("trailDepth")
            , array("avg. trail depth"));

        $this->print_one_graph(array("trailDepthDelta")
            , array("avg. trail depth delta"));

        $this->print_one_graph(array("glue")
            , array("newly learnt clauses avg. glue"));

        $this->print_one_graph(array("size")
            , array("newly learnt clauses avg. size"));

        $this->print_one_graph(array("resolutions")
            , array("avg. no. resolutions for 1UIP"));

        $this->print_one_graph(array("agility")
            , array("avg. agility"));

        /*print_one_graph(array("flippedPercent")
            , array("var polarity flipped %"));*/

        $this->print_one_graph(array("conflAfterConfl")
            , array("conflict after conflict %"));

        /*print_one_graph("conflAfterConflSD", array("conflAfterConfl")
            , array("conflict after conflict std dev %"));*/

        $this->print_one_graph(array("watchListSizeTraversed")
            , array("avg. traversed watchlist size"));

        /*print_one_graph(array("watchListSizeTraversedSD")
            , array("avg. traversed watchlist size std dev"));*/

        /*print_one_graph("litPropagatedSomething", array("litPropagatedSomething")
            , array("literal propagated something with binary clauses %"));*/

        $this->print_one_graph(array("replaced")
            , array("vars replaced"));

        $this->print_one_graph(array("set")
            , array("vars set"));

        $this->print_one_graph(array(
            "varSetPos"
            , "varSetNeg"
            )
            , array(
            "propagated polar pos %"
            , "propagated polar neg %")
        );

        $this->print_one_graph(array(
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

        $this->print_one_graph(array(
            "propBinIrred", "propBinRed"
            , "propTriIrred", "propTriRed"
            , "propLongIrred", "propLongRed"
            )
            ,array("prop by bin irred %", "prop by bin red %"
            , "prop by tri irred %", "prop by tri red %"
            , "prop by long irred %", "prop by long red %"
            )
        );

        $this->print_one_graph(array(
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

        $this->print_one_graph(array(
              "resolBin"
            , "resolTri"
            , "resolLIrred"
            , "resolLRed"
            )
            ,array(
              "resolv by bin %"
            , "resolv by tri %"
            , "resolv by long irred %"
            , "resolv by long red %"
            )
        );

        /*print_one_graph("branchDepthSD", array("branchDepthSD")
            , array("branch depth std dev"));

        print_one_graph("branchDepthDeltaSD", array("branchDepthDeltaSD")
            , array("branch depth delta std dev"));

        print_one_graph("trailDepthSD", array("trailDepthSD")
            , array("trail depth std dev"));

        print_one_graph("trailDepthDeltaSD", array("trailDepthDeltaSD")
            , array("trail depth delta std dev"));

        print_one_graph("glueSD", array("glueSD")
            , array("newly learnt clause glue std dev"));

        print_one_graph("sizeSD", array("sizeSD")
            , array("newly learnt clause size std dev"));

        print_one_graph("resolutionsSD", array("resolutionsSD")
            , array("std dev no. resolutions for 1UIP"));*/


        $this->runQuery("reduceDB");

        $this->print_one_graph(array(
                "irredLitsVisited"
                , "redLitsVisited"
            )
            ,array(
                "irredLitsVisited % "
                , "redLitsVisited %"
            )
        );

        $this->print_one_graph(array(
                  "preRemovedResolBin"
                , "preRemovedResolTri"
                , "preRemovedResolLIrred"
                , "preRemovedResolLRed"
            )
            ,array(
                  "preRemovedResolBin"
                , "preRemovedResolTri"
                , "preRemovedResolLIrred"
                , "preRemovedResolLRed"
            )
        );

        $this->print_one_graph(array(
                  "removedResolBin"
                , "removedResolTri"
                , "removedResolLIrred"
                , "removedResolLRed"
            )
            ,array(
                  "removedResolBin"
                , "removedResolTri"
                , "removedResolLIrred"
                , "removedResolLRed"
            )
        );

        $this->print_one_graph(array(
                  "remainResolBin"
                , "remainResolTri"
                , "remainResolLIrred"
                , "remainResolLRed"
            )
            ,array(
                  "remainResolBin"
                , "remainResolTri"
                , "remainResolLIrred"
                , "remainResolLRed"
            )
        );

        return $this->numberingScheme;
    }
}

for($i = 0; $i < count($runIDs); $i++) {
    $printer = new DataPrinter($i, $runIDs[$i], $maxConfl);
    echo "maxConflRestart.push(".$printer->get_max_confl().");";
    $orderNum = $printer->fill_data_tmp();
    echo "myData.push(data_tmp);";
}


class Simplifications
{
    protected $runIDs;

    public function __construct($runIDs)
    {
        $this->runIDs = $runIDs;
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
        foreach ($this->runIDs as $thisRunID) {
            $this->fillSimplificationPoints($thisRunID);
        }
    }
}

$simps = new Simplifications($runIDs);
$simps->fill();

class ClauseDistrib
{
    protected $colnum;
    protected $rownum;
    protected $runID;
    protected $tablename;
    protected $lookAt;
    protected $maxConfl;

    public function __construct($mycolnum, $myrownum, $runID, $maxConfl, $tablename, $lookAt)
    {
        $this->colnum = $mycolnum;
        $this->rownum = $myrownum;
        $this->runID = $runID;
        $this->tablename = $tablename;
        $this->lookAt = $lookAt;
        $this->maxConfl = $maxConfl;
    }

    public function fillClauseDistrib()
    {
        echo "tmpArray = new Array();\n";

        $query = "
        SELECT conflicts, num FROM ".$this->tablename."
        where runID = ".$this->runID."
        and conflicts <= ".$this->maxConfl."
        and ".$this->lookAt." > 0
        order by conflicts, ".$this->lookAt;
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

        $blockDivID = "distBlock".$this->colnum."-".$this->rownum;
        $dataDivID = "drawingPad".$this->colnum."-".$this->rownum."-Parent";
        $canvasID = "drawingPad".$this->colnum."-".$this->rownum;
        $labelDivID = "$blockDivID"."_labeldiv";

        //Put into data
        echo "oneData = {data: tmpArray";
        echo ", blockDivID:  '$blockDivID'";
        echo ", dataDivID:  '$dataDivID'";
        echo ", canvasID: '$canvasID'";
        echo ", labelDivID: '$labelDivID'";
        echo ", lookAt: '".$this->lookAt."'";
        echo "};\n";
        echo "clDistrib[".$this->colnum."].push(oneData);";

        //Put into columnDivs
        echo "tmp = {blockDivID:  '$blockDivID'};";
        echo "columnDivs[".$this->colnum."].push(tmp);";
    }
}

for($i = 0; $i < count($runIDs); $i++) {
    echo "clDistrib.push([]);";
    $myDist = new ClauseDistrib($i, 0, $runIDs[$i], $maxConfl, "clauseGlueDistrib", "glue");
    $myDist->fillClauseDistrib();

    $myDist = new ClauseDistrib($i, 1, $runIDs[$i], $maxConfl, "clauseSizeDistrib", "size");
    $myDist->fillClauseDistrib();
}
echo '</script>';

?>
