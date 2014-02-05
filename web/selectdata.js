var columnDivs = new Array();
var myData = new Array();
var clDistrib = new Array();
var simplificationPoints = new Array();
var maxConflRestart = new Array();

//while (true) {
//setInterval(function(){myajax.makeGetRequest(500005960);}, 2000);
//myajax.makeGetRequest(86533651);
//}


jQuery.ajax({
     url:'getdata.php?id=' + '86533651'
     ,success:function(response){
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
    }
});
