<?php
$username="cmsat_presenter";
$password="neud21kahgsdk";
$database="cmsat";

$sql = new mysqli("localhost", $username, $password, $database);
if (mysqli_connect_errno()) {
    printf("Connect failed: %s\n", mysqli_connect_error());
    die();
}

function fill_versions($sql)
{
    $query = "select `version` from `solverRun` group by `version`;";
    $stmt = $sql->prepare($query);
    if (!$stmt) {
        die("Cannot prepare statement");
    }
    $stmt->execute();
    $stmt->bind_result($version);

    $json = array();
    while($stmt->fetch())
    {
         $data = array(
            'text' => $version,
            'value' => $version
        );
        array_push($json, $data);
    }
    $jsonstring = json_encode($json);
    echo $jsonstring;
    $stmt->close();
}
fill_versions($sql);
?>