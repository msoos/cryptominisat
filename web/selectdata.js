var columnDivs = new Array();
var myData = new Array();
var clDistrib = new Array();
var simplificationPoints = new Array();
var maxConflRestart = new Array();

function MyAjax() {
    var gotResponse = true;
    var http = createRequestObject();

    function createRequestObject() {
        var tmpXmlHttpObject;

        //Browser dependent
        if (window.XMLHttpRequest) {
            //Mozilla, Safari
            tmpXmlHttpObject = new XMLHttpRequest();

        } else if (window.ActiveXObject) {
            //IE
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
            //Got it

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

myajax = new MyAjax();

//while (true) {
//setInterval(function(){myajax.makeGetRequest(500005960);}, 2000);
myajax.makeGetRequest(86533651);
//}
