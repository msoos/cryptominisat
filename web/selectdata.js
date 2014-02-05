var columnDivs = new Array();
var myData = new Array();
var clDistrib = new Array();
var simplificationPoints = new Array();
var maxConflRestart = new Array();

function MyAjax() {

    //Ajax value OK?
    var gotResponse = true;
    var http = createRequestObject();

    //Creates xmlHttpObject
    function createRequestObject() {
        var tmpXmlHttpObject;

        //depending on what the browser supports, use the right way to create the XMLHttpRequest object
        if (window.XMLHttpRequest) {
            // Mozilla, Safari would use this method ...
            tmpXmlHttpObject = new XMLHttpRequest();

        } else if (window.ActiveXObject) {
            // IE would use this method ...
            tmpXmlHttpObject = new ActiveXObject("Microsoft.XMLHTTP");
        }

        return tmpXmlHttpObject;
    }

    this.makeGetRequest = function(runID) {
        gotResponse = false;

        //make a connection to the server ... specifying that you intend to make a GET request
        //to the server. Specifiy the page name and the URL parameters to send
        http.open('get', 'getdata.php?id=' + runID);

        //assign a handler for the response
        http.onreadystatechange = processResponse;

        //actually send the request to the server
        http.send(null);
    }

    function processResponse() {
        //check if the response has been received from the server
        if(http.readyState == 4){

            //read and assign the response from the server
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

            //If the server returned an error message like a 404 error, that message would be shown within the div tag!!.
            //So it may be worth doing some basic error before setting the contents of the <div>
        }
    }
}

myajax = new MyAjax();

//while (true) {
//setInterval(function(){myajax.makeGetRequest(500005960);}, 2000);
myajax.makeGetRequest(86533651);
//}
