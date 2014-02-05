<?php
error_reporting(E_ALL | E_STRICT);
ini_set('display_errors',1);
$maxConfl = 5000000;
//display_startup_errors(1);
//error_reporting(E_STRICT);

$username="cmsat_presenter";
$password="";
$database="cmsat";

$sql = new mysqli("localhost", $username, $password, $database);
if (mysqli_connect_errno()) {
    printf("Connect failed: %s\n", mysqli_connect_error());
    die();
}
//print_r($sql);

#$runIDs = array(mysql_real_escape_string($_GET["id"]));
#$runIDs = array(getLatestID());
#$runIDs = array(4890500, 15772794, 15772794);
#$runIDs = array(4890500);
#$runIDs = array(15772794);
$runIDs = array(12962808);

function getLatestID()
{
    $query = "select runID from `startup` order by startTime desc limit 1;";
    $result = $sql->query($query);
    if (!$result) {
        printf("Error: %s\n", $sql->error());
        die();
    }

    if ($result->num_rows < 1) {
        return 0;
    }
    $row = $result->fetch_assoc();
    $runID = (int)$row["runID"];
    return $runID;
}

class DataPrinter
{
    protected $numberingScheme;
    protected $data;
    protected $nrows;
    protected $colnum;
    protected $runID;
    protected $num_runIDs;
    protected $max_confl;
    protected $columndivs;
    protected $data_tmp;
    protected $tablename;
    protected $sql;

    public function __construct($mycolnum, $runID, $maxConfl, $num_RunIDs, $json_columndivs, $sql)
    {
        $this->colnum = $mycolnum;
        $this->runID = $runID;
        $this->numberingScheme = 0;
        $this->sql = $sql;
        $this->max_confl = $this->sql_get_max_restart($maxConfl);
        $this->num_runIDs = $num_RunIDs;
        $this->columndivs = $json_columndivs;
        $this->data_tmp = array();
    }

    private function sql_get_max_restart($maxConfl)
    {

        $query="
        SELECT max(conflicts) as mymax FROM `restart`
        where conflicts < $maxConfl
        and runID = ".$this->runID.";";
        $result = $this->sql->query($query);
        if (!$result) {
            printf("Error: %s\n", $this->sql->error());
            die();
        }

        $row = $result->fetch_assoc();
        $max = (int)$row["mymax"];
        return $max;
    }

    public function get_max_confl()
    {
        return $this->max_confl;
    }

    protected function print_one_graph(
        $title
        , $datanames
        , $nicedatanames
        , $everyn = 1000
    ) {
        $json_data = array();

        //Start with an empty one
        $json_subarray = array();
        array_push($json_subarray, 0);
        $i = 0;
        while($i < sizeof($datanames)) {
            array_push($json_subarray, NULL);
            $i++;
        }
        array_push($json_data, $json_subarray);

        //Now go through it all
        $i=0;
        $total_sum = 0.0;
        $last_confl = 0.0;
        $last_confl_for_everyn = 0.0;
        $time_start = microtime();
        $this->data->data_seek(0);
        while ($i < $this->nrows) {
            $row = $this->data->fetch_assoc();
            $confl = (int)$row["conflicts"];
            if ($confl -$last_confl_for_everyn < $everyn) {
                $i++;
                continue;
            }
            $last_confl_for_everyn = $confl;
            $json_subarray = array();
            array_push($json_subarray, $confl);

            //Calc local sum
            $local_sum = 0;
            foreach ($datanames as $dataname) {
                $local_sum += (int)$row[$dataname];
            }

            //Print for each
            foreach ($datanames as $dataname) {
                $tmp = (int)$row[$dataname];
                if (sizeof($datanames) > 1) {
                    if ($local_sum != 0) {
                        $tmp /= $local_sum;
                        $tmp *= 100.0;
                    }
                    array_push($json_subarray, $tmp);
                } else {
                    $total_sum += $tmp*($confl-$last_confl);
                    $last_confl = $confl;
                    array_push($json_subarray, $tmp);
                }
            }
            array_push($json_data, $json_subarray);
            $i++;
        }
        $time = microtime() - $time_start;
        //echo "Took $time seconds\n";

        //Calculate labels
        $json_labels_tmp = array();
        array_push($json_labels_tmp, "Conflicts");
        foreach ($nicedatanames as $dataname) {
            array_push($json_labels_tmp, $dataname);
        }

        //Calculate blockDivID
        $blockDivID = "graphBlock".$this->numberingScheme."AT".$this->colnum;
        $fullname = "toplot_".$this->numberingScheme."_".$this->colnum;

        //put into $this->data_tmp
        $json_data_tmp = array();
        $json_data_tmp['data'] = $json_data;
        $json_data_tmp['labels'] = $json_labels_tmp;
        $json_data_tmp['stacked'] = (int)(sizeof($datanames) > 1);
        $json_data_tmp['colnum'] = $this->colnum;
        $json_data_tmp['blockDivID'] = $blockDivID;
        $json_data_tmp['dataDivID'] = $fullname."_datadiv";
        $json_data_tmp['labelDivID'] = $fullname."_labeldiv";
        $json_data_tmp['max_confl'] = $this->max_confl;
        $json_data_tmp['title'] = $title;
        $json_data_tmp['tablename'] = $this->tablename;
        array_push($this->data_tmp, $json_data_tmp);

        //put into $this->columndivs
        $json_columndivs_tmp = array();
        $json_columndivs_tmp['blockDivID'] = $blockDivID;
        if (!array_key_exists($this->colnum, $this->columndivs)) {
            $this->columndivs[$this->colnum] = array();
        }
        array_push($this->columndivs[$this->colnum], $json_columndivs_tmp);

        $this->numberingScheme++;
    }

    function runQuery($table, $extra = "")
    {
        $this->tablename = $table;

        $query="
        SELECT *
        FROM `$table`
        where `runID` = ".$this->runID."
        and conflicts <= ".$this->max_confl." $extra
        order by `conflicts`";

        $this->data = $this->sql->query($query);
        if (!$this->data) {
            printf("Error: %s\n", $this->sql->error());
            die();
        }
        $this->nrows = $this->data->num_rows;
    }

    public function fill_data_tmp()
    {
        $this->runQuery("restart");

//         $this->print_one_graph(
//             "No. of restarts"
//             , array("restarts")
//             , array("")
//         );

        /*print_one_graph(array("propsPerDec")
            , array("avg. no. propagations per decision"));*/

        $this->print_one_graph(
            "Distribution of clause types %"
            , array(
            "set"
            , "numIrredBins"
            , "numRedBins"
            , "numIrredTris"
            , "numRedTris"
            , "numIrredLongs"
            , "numRedLongs"
            )
            ,array(
            "unit cls"
            , "irred bin"
            , "red bin"
            , "irred tri"
            , "red tri"
            , "irred long"
            , "ired long"
            )
        );

        $this->print_one_graph(
            "Avg. branch depth"
            , array("branchDepth")
            , array("")
        );

        $this->print_one_graph(
            "Avg. branch depth delta"
            , array("branchDepthDelta")
            , array(""));

        $this->print_one_graph(
            "Avg. trail depth"
            , array("trailDepth")
            , array(""));

        $this->print_one_graph(
            "Avg. trail depth delt"
            , array("trailDepthDelta")
            , array("")
        );

        $this->print_one_graph(
            "Avg. glue of newly learnt clauses"
            , array("glue")
            , array("")
        );

        $this->print_one_graph(
            "Avg. size of newly learnt clauses"
            , array("size")
            , array("")
        );

        $this->print_one_graph(
            "Avg. no. of resolutions carried out for 1st UIP"
            , array("resolutions")
            , array("")
        );

        $this->print_one_graph(
            "Avg. agility"
            , array("agility")
            , array("avg. agility")
        );

        /*print_one_graph(array("flippedPercent")
            , array("var polarity flipped %"));*/

        $this->print_one_graph(
            "Conflicts immediately following a conflict %"
            , array("conflAfterConfl")
            , array("")
        );

        /*print_one_graph("conflAfterConflSD", array("conflAfterConfl")
            , array("conflict after conflict std dev %"));*/

//         $this->print_one_graph(
//             "Avg. traversed watchlist size"
//             , array("watchListSizeTraversed")
//             , array("")
//         );

        /*print_one_graph(array("watchListSizeTraversedSD")
            , array("avg. traversed watchlist size std dev"));*/

        /*print_one_graph("litPropagatedSomething", array("litPropagatedSomething")
            , array("literal propagated something with binary clauses %"));*/

        $this->print_one_graph(
            "No. of variables replaced"
            , array("replaced")
            , array("")
        );

        $this->print_one_graph(
            "No. of variables\' values set"
            , array("set")
            , array("")
        );

        $this->print_one_graph(
            "Propagated polarity %"
            , array(
            "varSetPos"
            , "varSetNeg"
            )
            , array(
            "positive"
            , "negative")
        );

        $this->print_one_graph(
            "Newly learnt clause type %"
            , array(
            "learntUnits"
            , "learntBins"
            , "learntTris"
            , "learntLongs"
            )
            ,array(
            "unit"
            , "binary"
            , "tertiary"
            , "long")
        );

        $this->print_one_graph(
            "Propagation by %"
            , array(
            "propBinIrred", "propBinRed"
            , "propTriIrred", "propTriRed"
            , "propLongIrred", "propLongRed"
            )
            ,array("bin irred", "bin red"
            , "tri irred", "tri red"
            , "long irred", "long red"
            )
        );

        $this->print_one_graph(
            "Conflict caused by clause type %"
            , array(
            "conflBinIrred"
            , "conflBinRed"
            , "conflTriIrred"
            , "conflTriRed"
            , "conflLongIrred"
            , "conflLongRed"
            )
            ,array(
            "bin irred"
            , "bin red"
            , "tri irred"
            , "tri red"
            , "long irred"
            , "long red"
            )
        );

        $this->print_one_graph(
            "Resolutions used clause types %"
            , array(
              "resolBin"
            , "resolTri"
            , "resolLIrred"
            , "resolLRed"
            )
            ,array(
              "bin"
            , "tri"
            , "long irred"
            , "long red"
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

        $this->print_one_graph(
            "Visited literals while propagating %"
            , array(
                "irredLitsVisited"
                , "redLitsVisited"
            )
            ,array(
                "irredundant"
                , "redundant"
            )
        );

//         $this->print_one_graph(
//             "Clause-cleaning pre-removed clauses with resolutions"
//             , array(
//                   "preRemovedResolBin"
//                 , "preRemovedResolTri"
//                 , "preRemovedResolLIrred"
//                 , "preRemovedResolLRed"
//             )
//             ,array(
//                   "bin"
//                 , "tri"
//                 , "long irred"
//                 , "long red"
//             )
//         );

        $this->print_one_graph(
            "Cleaning removed learnt cls with resolutions %"
            , array(
                  "removedResolBin"
                , "removedResolTri"
                , "removedResolLIrred"
                , "removedResolLRed"
            )
            ,array(
                  "bin"
                , "tri"
                , "long irred"
                , "long red"
            )
            , 1
        );

        $this->print_one_graph(
            "After cleaning remaining learnt cls with resolutions %"
            , array(
                  "remainResolBin"
                , "remainResolTri"
                , "remainResolLIrred"
                , "remainResolLRed"
            )
            ,array(
                  "bin"
                , "tri"
                , "long irred"
                , "long red"
            )
        );

        return array($this->columndivs, $this->data_tmp, $this->numberingScheme);
    }
}

$json_graph_data = array();
$json_maxconflrestart = array();
$json_columndivs = array();
for($i = 0; $i < count($runIDs); $i++) {
    $printer = new DataPrinter($i, $runIDs[$i], $maxConfl, count($runIDs), $json_columndivs, $sql);

    list($json_columndivs, $data_tmp, $orderNum) = $printer->fill_data_tmp();
    array_push($json_maxconflrestart, $printer->get_max_confl());
    array_push($json_graph_data, $data_tmp);
}


class Simplifications
{
    protected $runIDs;
    protected $points = array();
    protected $sql;

    public function __construct($runIDs, $sql)
    {
        $this->runIDs = $runIDs;
        $this->sql = $sql;
    }

    protected function fillSimplificationPoints($thisRunID)
    {
        $query="
        SELECT max(conflicts) as confl, simplifications as simpl
        FROM restart
        where runID = ".$thisRunID."
        group by simplifications
        order by simplifications";
        $result = $this->sql->query($query);
        if (!$result) {
            printf("Error: %s\n", $this->sql->error());
            die();
        }

        $json_tmp = array();
        array_push($json_tmp, 0);
        $i=0;
        while ($row = $result->fetch_assoc()) {
            $confl = (int)$row["confl"];
            array_push($json_tmp, $confl);
            $i++;
        }
        array_push($this->points, $json_tmp);
    }

    public function fill()
    {
        foreach ($this->runIDs as $thisRunID) {
            $this->fillSimplificationPoints($thisRunID);
        }

        return $this->points;
    }
}

$simps = new Simplifications($runIDs, $sql);
$json_simplificationpoints = $simps->fill();

class ClauseDistrib
{
    protected $colnum;
    protected $rownum;
    protected $runID;
    protected $tablename;
    protected $lookAt;
    protected $maxConfl;
    protected $cldistrib;
    protected $columndivs;

    public function __construct($mycolnum, $myrownum, $runID, $maxConfl, $tablename, $lookAt, $cldistrib, $columndivs)
    {
        $this->colnum = $mycolnum;
        $this->rownum = $myrownum;
        $this->runID = $runID;
        $this->tablename = $tablename;
        $this->lookAt = $lookAt;
        $this->maxConfl = $maxConfl;
        $this->cldistrib = $cldistrib;
        $this->columndivs = $columndivs;
    }

    public function fillClauseDistrib()
    {
        $json_tmparray = array();

        $query = "
        SELECT conflicts, num FROM ".$this->tablename."
        where runID = ".$this->runID."
        and conflicts <= ".$this->maxConfl."
        and ".$this->lookAt." > 0
        order by conflicts, ".$this->lookAt;

        $result = $this->sql->query($query);
        if (!$result) {
            printf("Error: %s\n", $this->sql->error());
            die();
        }
        $nrows = $result->num_rows;
        $result = $this->sql->store_result();

        $rownum = 0;
        $lastConfl = 0;
        while($rownum < $nrows) {
            $this->data->data_seek($rownum);
            $row = $this->data->fetch_assoc();
            $confl = (int)$row["conflicts"];

            $json_tmp = array();
            $json_tmp['conflStart'] = $lastConfl;
            $json_tmp['conflEnd'] = $confl;
            $json_darkness = array();
            $lastConfl = $confl;
            while($rownum < $nrows) {
                $this->data->data_seek($rownum);
                $row = $this->data->fetch_assoc();
                $numberOfCl = (int)$row["num"];
                array_push($json_darkness, $numberOfCl);

                //More in this bracket?
                $rownum++;
                $this->data->data_seek($rownum);
                $row = $this->data->fetch_assoc();
                if ($rownum >= $nrows
                    || $row["conflicts"] != $confl
                ) {
                    break;
                }
            }
            $json_tmp['darkness'] = $json_darkness;
            array_push($json_tmparray, $json_tmp);
        }

        $blockDivID = "distBlock".$this->colnum."-".$this->rownum;
        $dataDivID = "drawingPad".$this->colnum."-".$this->rownum."-Parent";
        $canvasID = "drawingPad".$this->colnum."-".$this->rownum;
        $labelDivID = "$blockDivID"."_labeldiv";

        //Put into data
        $json_onedata = array();
        $json_onedata['data'] = $json_tmparray;
        $json_onedata['blockDivID'] = $blockDivID;
        $json_onedata['dataDivID'] = $dataDivID;
        $json_onedata['canvasID'] = $canvasID;
        $json_onedata['labelDivID'] = $labelDivID;
        $json_onedata['lookAt'] = $this->lookAt;
        array_push($this->cldistrib[$this->colnum], $json_onedata);

        //Put into columnDivs
        $json_tmp = array();
        $json_tmp['blockDivID'] = $blockDivID;
        array_push($this->columndivs[$this->colnum], $json_tmp);

        return array($this->cldistrib, $this->columndivs);
    }
}

$json_cldistrib = array();
/*for($i = 0; $i < count($runIDs); $i++) {
    array_push($json_cldistrib, array());
    $myDist = new ClauseDistrib($i, 0, $runIDs[$i], $maxConfl, "clauseGlueDistrib", "glue", $json_cldistrib, $json_columndivs);
    list($json_cldistrib, $json_columndivs) = $myDist->fillClauseDistrib();

    $myDist = new ClauseDistrib($i, 1, $runIDs[$i], $maxConfl, "clauseSizeDistrib", "size", $json_cldistrib, $json_columndivs);
    list($json_cldistrib, $json_columndivs) = $myDist->fillClauseDistrib();
}*/

$final_json = array();
$final_json["columnDivs"] = $json_columndivs;
$final_json["graph_data"] = $json_graph_data;
$final_json["clDistrib"] = $json_cldistrib;
$final_json["simplificationPoints"] = $json_simplificationpoints;
$final_json["maxConflRestart"] = $json_maxconflrestart;
$jsonstring = json_encode($final_json);
echo $jsonstring;
?>
