<?php
include "mysql_connect.php";

$version = $_GET["version"];
$unfinished = $_GET["unfinish"] == "true";
$sat = $_GET["sat"] == "true";
$unsat = $_GET["unsat"] == "true";
// $version = 'dc65769cfcba9c2bb52ee81dbea39830ad28bb1b';
// $unfinished = True;
// $sat = True;
// $unsat = True;

$json = array();
function get_files_for_version($sat, $unsat, $unfinished)
{
    global $sql, $version, $json;

    $toadd = "(";
    $num = 0;
    if ($unfinished) {
        $num++;
        $toadd .= "finishup.status is NULL";
    }
    if ($sat) {
        if ($num >0 ) $toadd .= " or ";
        $num++;
        $toadd .= "finishup.status = 'l_True'";
    }
    if ($unsat) {
        if ($num >0 ) $toadd .= " or ";
        $num++;
        $toadd .= "finishup.status = 'l_False'";
    }
    $toadd .= ")";

    $query = "
    select solverRun.runID as runID, tags.tag as fname
    from tags, solverRun left join finishup on (finishup.runID = solverRun.runID)
    where solverRun.version = ?
    and solverRun.runID = tags.runID
    and tags.tagname='filename'
    and $toadd
    order by tags.tag;";

    $stmt = $sql->prepare($query);
    if (!$stmt) {
        print "Error:".$sql->error;
        die("Cannot prepare statement");
    }
    $stmt->bind_param('s', $version);
    $stmt->execute();
    $stmt->bind_result($runID, $fname);

    $numfiles = 0;
    while($stmt->fetch())
    {
        $numfiles++;
        //echo "{text: '".$row['tag']."', value: '".$row['runID']."'},";
        $data = array(
            'text' => basename($fname),
            'value' => $runID
        );
        array_push($json, $data);
    }
    $stmt->close();

    return $numfiles;
}

$numfiles = get_files_for_version($sat, $unsat, $unfinished);

$ret = array(
    'filelist' => $json,
    'numfiles' => $numfiles
);
$jsonstring = json_encode($ret);
echo $jsonstring;
$sql->close();
?>