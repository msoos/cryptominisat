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
    var v = document.getElementById("fileinfo");
    jQuery.getJSON(
        "getdata.php?id=" + runID,
         function(response)
         {
            var v = document.getElementById("fileinfo");
            v.innerHTML = "Status: ";
            var metad = response["metadata"][0];
            if (metad["status"] == null) {
                v.innerHTML += "Unfinished";
            } else {
                v.innerHTML += metad["status"];
                v.innerHTML += ", time(s): "+ metad["difftime"];
                //v.innerHTML += ", start time: "+ metad["startTime"];
                //v.innerHTML += ", end time: "+ metad["endTime"];
            }
            v.innerHTML += ", run id: "+runID;

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
