<?php
ob_start('ob_gzhandler');
error_reporting(E_ALL | E_STRICT);
ini_set('display_errors',1);
$maxConfl = 50000000000;
//display_startup_errors(1);
//error_reporting(E_STRICT);

$username="cmsat_presenter";
$password="neud21kahgsdk";
$database="cmsat";

$sql = new mysqli("localhost", $username, $password, $database);
if (mysqli_connect_errno()) {
    printf("Connect failed: %s\n", mysqli_connect_error());
    die();
}
//print_r($sql);

$runID = $_GET["id"];
#$runID = 11335226;

class MainDataGetter
{
    protected $numberingScheme;
    protected $data;
    protected $nrows;
    protected $colnum;
    protected $runID;
    protected $max_confl;
    protected $columndivs;
    protected $data_tmp;
    protected $tablename;
    protected $sql;

    public function __construct($runID, $maxConfl, $sql)
    {
        $this->runID = $runID;
        $this->numberingScheme = 0;
        $this->sql = $sql;
        $this->max_confl = $this->sql_get_max_restart($maxConfl);
        $this->columndivs = array();
        $this->data_tmp = array();
    }

    private function sql_get_max_restart($maxConfl)
    {

        $query="
        SELECT max(conflicts) as mymax FROM `restart`
        where conflicts < ?
        and runID = ?";

        $stmt = $this->sql->prepare($query);
        if (!$stmt) {
            die("Cannot prepare statement $query");
        }
        $stmt->bind_param('ii', $maxConfl, $this->runID);
        $stmt->execute();
        $stmt->bind_result($max);
        $stmt->fetch();
        $stmt->close();
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
            if ($confl -$last_confl_for_everyn < $everyn && $i < $this->nrows-1) {
                $i++;
                continue;
            }
            $last_confl_for_everyn = $confl;
            $json_subarray = array();
            array_push($json_subarray, $confl);

            //Calc local sum
            $local_sum = 0;
            foreach ($datanames as $dataname) {
                $local_sum += $row[$dataname];
            }

            //Print for each
            foreach ($datanames as $dataname) {
                $tmp = $row[$dataname];
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
        $blockDivID = "graphBlock".$this->numberingScheme;
        $fullname = "toplot_".$this->numberingScheme;

        //put into $this->data_tmp
        $json_data_tmp = array();
        $json_data_tmp['data'] = $json_data;
        $json_data_tmp['labels'] = $json_labels_tmp;
        $json_data_tmp['stacked'] = (int)(sizeof($datanames) > 1);
        $json_data_tmp['blockDivID'] = $blockDivID;
        $json_data_tmp['dataDivID'] = $fullname."_datadiv";
        $json_data_tmp['labelDivID'] = $fullname."_labeldiv";
        $json_data_tmp['max_confl'] = $this->max_confl;
        $json_data_tmp['title'] = $title;
        $json_data_tmp['tablename'] = $this->tablename;
        $json_data_tmp['simple_line'] = count($datanames) == 1;
        array_push($this->data_tmp, $json_data_tmp);

        //put into $this->columndivs
        $json_columndivs_tmp = array();
        $json_columndivs_tmp['blockDivID'] = $blockDivID;
        array_push($this->columndivs, $json_columndivs_tmp);

        $this->numberingScheme++;
    }

    function runQuery($table)
    {
        $this->tablename = $table;

        #NOT controllable by attacker, but sanitize it anyway
        $table = $this->sql->real_escape_string($table);

        $query="
        SELECT *
        FROM `$table`
        where `runID` = ?
        and conflicts <= ?
        order by `conflicts`";

        $stmt = $this->sql->prepare($query);
        if (!$stmt) {
            die("Cannot prepare statement $query");
        }
        $stmt->bind_param("ii", $this->runID, $this->max_confl);
        $stmt->execute();
        $this->data = $stmt->get_result();
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

//         $this->print_one_graph(
//             array("var polarity flipped")
//             , array("flipped")
//             , array("")
//         );

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


class Simplifications
{
    protected $runID;
    protected $sql;

    public function __construct($runID, $sql)
    {
        $this->runID = $runID;
        $this->sql = $sql;
    }

    public function fillSimplificationPoints()
    {
        $query="
        SELECT max(conflicts) as confl, simplifications as simpl
        FROM restart
        where runID = ?
        group by simplifications
        order by simplifications";

        $stmt = $this->sql->prepare($query);
        if (!$stmt) {
            die("Cannot prepare statement $query");
        }
        $stmt->bind_param("i", $this->runID);
        $stmt->execute();
        $result = $stmt->get_result();

        $json_tmp = array();
        array_push($json_tmp, 0);
        $i=0;
        while ($row = $result->fetch_assoc()) {
            $confl = (int)$row["confl"];
            array_push($json_tmp, $confl);
            $i++;
        }
        return $json_tmp;
    }
}

class ClauseDistrib
{
    protected $rownum;
    protected $runID;
    protected $tablename;
    protected $lookAt;
    protected $maxConfl;
    protected $columndivs;

    public function __construct($mycolnum, $myrownum, $runID, $maxConfl, $tablename, $lookAt, $columndivs)
    {
        $this->rownum = $myrownum;
        $this->runID = $runID;
        $this->tablename = $tablename;
        $this->lookAt = $lookAt;
        $this->maxConfl = $maxConfl;
        $this->columndivs = $columndivs;
    }

    public function fillClauseDistrib()
    {
        $json_tmparray = array();
        $lookAt = $this->sql->real_escape_string($this->lookat);

        $query = "
        SELECT conflicts, num FROM ".$this->tablename."
        where runID = ?
        and conflicts <= ?
        and `$lookAt` > 0
        order by `conflicts`, `$lookAt`";

        $stmt = $this->sql->prepare($query);
        if (!$stmt) {
            die("Cannot prepare statement $query");
        }
        $stmt->bind_param("ii", $this->runID, $this->maxConfl);
        $stmt->execute();
        $result = $stmt->get_result();
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

        $blockDivID = "distBlock".$this->rownum;
        $dataDivID = "drawingPad".$this->rownum."-Parent";
        $canvasID = "drawingPad".$this->rownum;
        $labelDivID = "$blockDivID"."_labeldiv";

        //Put into data
        $json_onedata = array();
        $json_onedata['data'] = $json_tmparray;
        $json_onedata['blockDivID'] = $blockDivID;
        $json_onedata['dataDivID'] = $dataDivID;
        $json_onedata['canvasID'] = $canvasID;
        $json_onedata['labelDivID'] = $labelDivID;
        $json_onedata['lookAt'] = $this->lookAt;

        //Put into columnDivs
        $json_tmp = array();
        $json_tmp['blockDivID'] = $blockDivID;
        array_push($this->columndivs, $json_tmp);

        return array($json_onedata, $this->columndivs);
    }
}

function get_metadata($sql, $runID)
{
    $query="
    SELECT `startTime`
    FROM `startup`
    where runID = ?";

    $stmt = $sql->prepare($query);
    if (!$stmt) {
        print "Error:".$sql->error;
        die("Cannot prepare statement $query");
    }
    $stmt->bind_param("i", $runID);
    $stmt->execute();
    $stmt->bind_result($starttime);
    $stmt->fetch();
    $stmt->close();

    $query="
    SELECT `endTime`, `status`
    FROM `finishup`
    where runID = ?";

    $stmt = $sql->prepare($query);
    if (!$stmt) {
        print "Error:".$sql->error;
        die("Cannot prepare statement $query");
    }
    $stmt->bind_param("i", $runID);
    $stmt->execute();
    $stmt->bind_result($endtime, $status);
    $ok = $stmt->fetch();
    $stmt->close();

    if ($ok) {
    $query="
        SELECT UNIX_TIMESTAMP(`endTime`)-UNIX_TIMESTAMP(`startTime`)
        FROM `finishup`, `startup`
        where startup.runID = finishup.runID
        and startup.runID = ?";

        $stmt = $sql->prepare($query);
        if (!$stmt) {
            print "Error:".$sql->error;
            die("Cannot prepare statement $query");
        }
        $stmt->bind_param("i", $runID);
        $stmt->execute();
        $stmt->bind_result($difftime);
        $stmt->fetch();
        $stmt->close();
    } else {
        $difftime = 0;
    }

    $json_ret = array();
    $json_ret["startTime"] = $starttime;
    $json_ret["endTime"] = $endtime;
    $json_ret["difftime"] = $difftime;
    $json_ret["status"] = $status;

    return $json_ret;
}

///Main Data
$main_data_getter = new MainDataGetter($runID, $maxConfl, $sql);
list($json_columndivs, $json_graph_data, $orderNum) = $main_data_getter->fill_data_tmp();
$json_maxconflrestart = $main_data_getter->get_max_confl();

///Simplification points
$simps = new Simplifications($runID, $sql);
$json_simplificationpoints = $simps->fillSimplificationPoints();

///Clause distributions
//$myDist = new ClauseDistrib($i, 0, $runID, $maxConfl, "clauseGlueDistrib", "glue", $json_columndivs);
//list($json_cldistrib, $json_columndivs) = $myDist->fillClauseDistrib();
$json_cldistrib = array();

///Metadata
$metadata = get_metadata($sql, $runID);

$final_json = array();
$final_json["metadata"] = $metadata;
$final_json["columnDivs"] = $json_columndivs;
$final_json["graph_data"] = $json_graph_data;
$final_json["clDistrib"] = $json_cldistrib;
$final_json["simplificationPoints"] = $json_simplificationpoints;
$final_json["maxConflRestart"] = $json_maxconflrestart;
$jsonstring = json_encode($final_json);
echo $jsonstring;
?>
