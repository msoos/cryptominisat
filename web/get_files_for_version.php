<?php
$username="cmsat_presenter";
$password="neud21kahgsdk";
$database="cmsat";

$sql = new mysqli("localhost", $username, $password, $database);
if (mysqli_connect_errno()) {
    printf("Connect failed: %s\n", mysqli_connect_error());
    die();
}

$version = $_GET["version"];
$unfinished = $_GET["unfinish"] == "true";
$sat = $_GET["sat"] == "true";
$unsat = $_GET["unsat"] == "true";
$numfiles = 0;

$json = array();
function get_files_for_unsat()
{
    $query = "
    select solverRun.runID as runID, tags.tag as fname
    from solverRun, tags, finishup
    where solverRun.runID = tags.runID
    and solverRun.runID = finishup.runID
    and solverRun.version = ?
    and tagname = 'filename'
    and finishup.status = 'l_False'
    order by tags.tag";

    get_files_for_version($query);
}

function get_files_for_sat()
{
    $query = "
    select solverRun.runID as runID, tags.tag as fname
    from solverRun, tags, finishup
    where solverRun.runID = tags.runID
    and solverRun.runID = finishup.runID
    and solverRun.version = ?
    and tagname = 'filename'
    and finishup.status = 'l_True'
    order by tags.tag";

    get_files_for_version($query);
}

function get_files_for_unfinished()
{
    $query = "
    select solverRun.runID as runID, tags.tag as fname
    from tags, solverRun left join finishup on (finishup.runID = solverRun.runID)
    where solverRun.version = ?
    and solverRun.runID = tags.runID
    and tags.tagname='filename'
    and finishup.runID is NULL;";

    get_files_for_version($query);
}

function get_files_for_version($query)
{
    global $sql, $version, $json, $numfiles;
    $stmt = $sql->prepare($query);
    if (!$stmt) {
        print "Error:".$sql->error;
        die("Cannot prepare statement");
    }
    $stmt->bind_param('s', $version);
    $stmt->execute();
    $stmt->bind_result($runID, $fname);

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
}
if ($unsat) {
    get_files_for_unsat();
}
if ($sat) {
    get_files_for_sat();
}
if ($unfinished) {
    get_files_for_unfinished();
}

$ret = array(
    'filelist' => $json,
    'numfiles' => $numfiles
);
$jsonstring = json_encode($ret);
echo $jsonstring;
$sql->close();
?>