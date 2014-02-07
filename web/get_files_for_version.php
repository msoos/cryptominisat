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
#$version = "aaaa3aaaa72c7bac1ebe38bdf939ac438d7e9f37";

function get_files_for_version($sql, $version)
{
    $query = "
    select solverRun.runID as runID, tags.tag as fname
    from solverRun, tags
    where solverRun.runID = tags.runID
    and solverRun.version = ?
    and tagname = 'filename'
    order by tags.tag";
    $stmt = $sql->prepare($query);
    if (!$stmt) {
        die("Cannot prepare statement");
    }
    $stmt->bind_param('s', $version);
    $stmt->execute();
    $stmt->bind_result($runID, $fname);

    $json = array();
    while($stmt->fetch())
    {
        //echo "{text: '".$row['tag']."', value: '".$row['runID']."'},";
        $data = array(
            'text' => basename($fname),
            'value' => $runID
        );
        array_push($json, $data);
    }
    $jsonstring = json_encode($json);
    echo $jsonstring;
    $stmt->close();
}
get_files_for_version($sql, $version);
?>