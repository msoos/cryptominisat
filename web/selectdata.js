var columnDivs = new Array();
var graph_data = new Array();
var clDistrib = new Array();
var simplificationPoints = new Array();
var maxConflRestart = new Array();

//while (true) {
//setInterval(function(){myajax.makeGetRequest(500005960);}, 2000);
//myajax.makeGetRequest(86533651);
//}

function selected_runID(runID) {
    jQuery.getJSON(
        "getdata.php?id=" + runID
         ,function(response){
            columnDivs = response["columnDivs"];
            graph_data = response["graph_data"];
            clDistrib = new Array();
            simplificationPoints = response["simplificationPoints"];
            maxConflRestart = response["maxConflRestart"];
            doAll();
        }
    );
}

//selected_runID(15772794);
