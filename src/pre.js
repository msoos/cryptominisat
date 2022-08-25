Module['print'] = function(text) {
    var bottom = at_bottom();
    $('#output').append(text+"\n")
    if (bottom) {
        scroll();
    }
};
Module['printErr'] = function(text) {
    $('#output').append("--error-- " + text+"\n")
    scroll();
};
Module['onRuntimeInitialized'] = function () {
    $().ready(function () {
        $('#runbutton').removeAttr("disabled")
        $('#result').text('Ready');
    })
}
