var columnDivs = new Array();
var myData = new Array();
var clDistrib = new Array();
var simplificationPoints = new Array();
var maxConflRestart = new Array();

function fill_datapoints() {
    var gotResponse = true;
    var http = createRequestObject();

    function createRequestObject() {
        var tmpXmlHttpObject;
        if (window.XMLHttpRequest) {
            tmpXmlHttpObject = new XMLHttpRequest();
        } else if (window.ActiveXObject) {
            tmpXmlHttpObject = new ActiveXObject("Microsoft.XMLHTTP");
        }
        return tmpXmlHttpObject;
    }

    this.makeGetRequest = function(runID) {
        gotResponse = false;
        http.open('get', 'getdata.php?id=' + runID);
        http.onreadystatechange = processResponse;
        http.send(null);
    }

    function processResponse() {
        if(http.readyState == 4){
            var response = http.responseText;

            //Clear data
            columnDivs = new Array();
            myData = new Array();
            clDistrib = new Array();
            simplificationPoints = new Array();
            maxConflRestart = new Array();

            var div = document.getElementById("myajaxStuff");
            div.innerHTML = response;
            var x = div.getElementsByTagName("script");
            for(var i=0; i < x.length; i++) {
                //console.log(x[i].text);
                //jQuery.parseJSON(x[i].text);
                //json.parse (x[i].text);
                eval(x[i].text);
            }
            doAll();
            gotResponse = true;
        }
    }
}

var myajax = new fill_datapoints();
//while (true) {
//setInterval(function(){myajax.makeGetRequest(500005960);}, 2000);
myajax.makeGetRequest(86533651);
//}
