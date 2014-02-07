<!--<html xmlns="http://www.w3.org/1999/xhtml">
<head>
    <meta charset="utf-8">
    <script type="text/javascript" src="jquery/jquery.js"></script>
</head>-->

<script type="text/javascript">
(function($, window) {
  jQuery.fn.replaceOptions = function(options) {
    var self, $option;

    this.empty();
    self = this;

    jQuery.each(options, function(index, option) {
      $option = $("<option></option>")
        .attr("value", option.value)
        .text(option.text);
      self.append($option);
    });
  };
})(jQuery, window);

function changed_version(val) {
    console.log(val);
    new fill_files_options(val);
}
</script>

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
    $result = $sql->query($query);
    if (!$result) {
        die('Invalid query: ' . mysql_error());
    }
    echo "<select id='version' onchange='changed_version(this.value);'>\n";
    while($row = $result->fetch_assoc())
    {
        echo "<option value = '".$row['version']."'>".$row['version']."</option>\n";
    }
    echo "</select>\n";
    $result->close();
}
fill_versions($sql);
?>

<select id="fname">
<option value="newsvalue">News</option>
<option value="webmastervalue">Webmaster</option>
<option value="techvalue">Tech</option>
</select>

<script type="text/javascript">
$('#fname').change(function(){
    selected_runID(jQuery("#fname option:selected").text());
    //alert($(this).val());
    //selected_runID(this.text());
})

function fill_files_options()
{
    jQuery.getJSON("get_files_for_version.php?version=" + jQuery("#version option:selected").text(),
        function(result){
            jQuery("#fname").replaceOptions(result);
            //selected_runID(jQuery("#fname option:selected").val());
            selected_runID(jQuery("#fname option:selected").text());
        }
    );
};
jQuery( document ).ready(function() {
    fill_files_options();
});
</script>

