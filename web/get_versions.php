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
$password="";
$database="cmsat";

mysql_connect("localhost", $username, $password);
@mysql_select_db($database) or die( "Unable to select database");

function fill_versions()
{
    $query = "select `version` from `solverRun` group by `version`;";
    $result = mysql_query($query);
    if (!$result) {
        die('Invalid query: ' . mysql_error());
    }
    echo "<select id='version' onchange='changed_version(this.value);'>\n";
    while($row = mysql_fetch_assoc($result))
    {
        echo "<option value = '".$row['version']."'>".$row['version']."</option>\n";
    }
    echo "</select>\n";
}
fill_versions();
?>

<select id="fname" onchange='selected_runID(this.value);'>
<option value="newsvalue">News</option>
<option value="webmastervalue">Webmaster</option>
<option value="techvalue">Tech</option>
</select>

<script type="text/javascript">
function fill_files_options()
{
    jQuery.getJSON("get_files_for_version.php?version=" + jQuery("#version option:selected").text(),
        function(result){
            jQuery("#fname").replaceOptions(result);
            console.log("updated:", result);
            console.log(jQuery("#fname option:selected").val());
            selected_runID(jQuery("#fname option:selected").val());
        }
    );
};
jQuery( document ).ready(function() {
    fill_files_options();
});
</script>

