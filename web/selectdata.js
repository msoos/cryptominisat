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
    clear_everything();
    console.log("getting it");
    jQuery.getJSON(
        "getdata.php?id=" + runID,
         function(response){
            console.log("gotit");
            console.log(response);
            columnDivs = response["columnDivs"];
            graph_data = response["graph_data"];
            clDistrib = new Array();
            simplificationPoints = response["simplificationPoints"];
            maxConflRestart = response["maxConflRestart"];
            print_all_graphs();
        }
    );
}

//selected_runID(15772794);
