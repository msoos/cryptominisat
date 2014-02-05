<?php
$username="cmsat_presenter";
$password="";
$database="cmsat";

mysql_connect("localhost", $username, $password);
@mysql_select_db($database) or die( "Unable to select database");

$version = mysql_real_escape_string($_GET["version"]);
#$version = "aaaa3aaaa72c7bac1ebe38bdf939ac438d7e9f37";

function get_files_for_version($version)
{
    $query = '
    select solverRun.runID as runID, tagname,tags.tag
    from solverRun, tags
    where solverRun.runID = tags.runID
    and solverRun.version="'.$version.'"
    and tagname = "filename";';

    $result = mysql_query($query);
    if (!$result) {
        die('Invalid query: ' . mysql_error());
    }

    $json = array();
    while($row = mysql_fetch_assoc($result))
    {
        //echo "{text: '".$row['tag']."', value: '".$row['runID']."'},";
        $data = array(
            'text' => $row['tag'],
            'value' => $row['runID']
        );
        array_push($json, $data);
    }
    $jsonstring = json_encode($json);
    echo $jsonstring;
}
get_files_for_version($version);
?>