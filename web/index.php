<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
    <meta charset="utf-8">
    <title>Cryptominisat 3 visualization</title>

    <link rel="stylesheet" type="text/css" href="jquery.jqplot.css" />
    <script type="text/javascript" src="jquery/jquery.js"></script>
    <script type="text/javascript" src="dygraphs/dygraph-combined.js"></script>
    <script type="text/javascript" src="scriptaculous-js-1.9.0/lib/prototype.js"></script>
    <script type="text/javascript" src="scriptaculous-js-1.9.0/src/scriptaculous.js"></script>
    <script type="text/javascript" src="dragdrop/js/portal.js"></script>
    <script type="text/javascript" src="selectdata.js"></script>
    <script type="text/javascript" src="drawgraphs.js"></script>

    <style>
    @import url(style.css);
    </style>

    <!-- Start of StatCounter Code -->
    <!--<script type="text/javascript">
    var sc_project=6140803;
    var sc_invisible=1;
    var sc_security="26273f9f";
    </script>

    <script type="text/javascript"
    src="http://www.statcounter.com/counter/counter.js">
    </script>

    <noscript><div class="statcounter"><a title="tumblr page counter"
    href="http://statcounter.com/tumblr/" target="_blank">
    <img class="statcounter" src="http://c.statcounter.com/6140803/0/26273f9f/1/"
    alt="tumblr page counter"></a></div>
    </noscript>-->
    <!-- End of StatCounter Code -->

</head>

<body>
<h1>Cryptominisat 3</h1>

<h3>Replacing wordy authority with visible certainty</h4>
<p>This webpage shows the partial solving of two SAT instances, visually.
I was amazed by <a href="http://www.edwardtufte.com/tufte/">Edward Tufte</a>'s
work (hence the subtitle) and this came out of it. Tufte would probably not
approve, as some of the layout is terrible. However, it might allow you to
understand SAT better, and may offer inspiration... or, rather, <i>vision</i>.
Enjoy.
</p>


<h2>Search restart statistics</h2>
<p>Below you will find conflicts in the X axis and several interesting data
on the Y axis. There are two columns, each selectable what to show.
Every datapoint corresponds to a restart. You may zoom in by
clicking on an interesting point and dragging the cursor along the X axis.
Double-click to unzoom. You can rearrange the order and layout by dragging the
labels on the right. Blue vertical lines indicate the positions of
<i>simplification sessions</i>. Between the blue lines are <i>search
sessions</i>. The angle of the "time" graph indicates conflicts/second.
Simplification sessions are not detailed. However, time jumps during
simplifcaition, and the solver behaviour changes afterwards. The angle
of the "restart no." graph indicates how often restarts were made. You can
find a full list of terms below.
</p>

<div id="columns"></div>
<div id="myajaxStuff"></div>
<div id="datagraphs"></div>

<!-- clear out column layout -->
<p style="clear:both"></p>

<h2>Terms used</h2>

<table class="gridtable">
<tr><th>Abbreviation</th><th>Explanation</th></tr>
<tr><td>red.</td><td>reducible, also called learnt</td></tr>
<tr><td>irred.</td><td>irreducible, also called non-learnt</td></tr>
<tr><td>confl</td><td>conflict reached by the solver</td></tr>
<tr><td>learnt</td><td>clause learnt during 1UIP conflict analysis</td></tr>
<tr><td>trail depth</td><td>depth of serach i.e. the number of variables set when the solver reached a conflict</td></tr>
<tr><td>brach depth</td><td>the number of branches made before conflict was encountered</td></tr>
<tr><td>trail depth delta</td><td>the number of variables we jumped back when doing conflict-directed backjumping</td></tr>
<tr><td>branch depth delta</td><td>the number of branches jumped back during conflict-directed backjumping</td></tr>
<tr><td>propagations/decision</td><td>number of variables set due to the
propagation of a decision (note that there is always at least one, the variable itself)
</td></tr>
<tr><td>vars replaced</td><td>the number or variables replaced due to equivalent literal simplfication</td></tr>
<tr><td>polarity flipped</td><td>polarities of variables are
<a href="http://dx.doi.org/10.1007/978-3-540-72788-0_28">saved</a> and then used
if branching is needed, but if propagation takes place, they are sometimes flipped
</td></tr>
<tr><td>std dev</td><td>standard deviation, the square root of variance</td></tr>
<tr><td>confl by</td><td>the clause that caused the conflict</td></tr>
<tr><td>agility</td><td>See
<a href="http://www.inf.ucv.cl/~bcrawford/PapersAutonomousSearch_julio2008/BRODERICK_CRAWFORD_AGO_01_X.pdf">
here</a>.</td></tr>
<tr><td>glue</td><td>the number of different decision levels of the literals found
in newly learnt clauses. See <a href = "http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.150.1911">
here</a></td></tr>
<tr><td>conflict after conflict %</td><td>How often does it happen that a conflict
, after backtracking and propagating immeediately (i.e. without branching) leads
to a conflict. This is displayed because it's extremely high percentage relative
to what most would expect. Thanks to
<a href="http://www.cril.univ-artois.fr/~jabbour/">Said Jabbour</a> for this.
</td></tr>
</table>

<h2>Why did I do this?</h2>
<p>There has been some
<a href="http://www-sr.informatik.uni-tuebingen.de/~sinz/DPvis/">
past work</a> on statically visualizing SAT problems by
<a href="http://www.carstensinz.de/">Carsten Sinz</a>, but not much on
dynamic solving visualization - in fact, nothing comes to my mind that
is comparable to what is above. However, the point of this excercise was
not only to visually display dynamic solver behaviour. Rather, I
think we could do dynamic analysis and heuristic adjustment instead
of the static analysis and static heuristic selection as done by current
<a href="http://www.jair.org/media/2490/live-2490-3923-jair.pdf">
portifolio solvers</a>. Accordingly, CryptoMiniSat 3 has an extremely
large set of options - e.g. swithcing between cleaning using glues,
activities, clause sizes, or number of propagations+conflicts made by
a clause is only a matter of setting a variable, and can be done on-the-fly.
Problems tend to evolve as simplication and solving steps are made,
so search heuristics should evolve with the problem.
</p>

<h2>The End</h2>
<p>If you enjoyed this visualization, there are three things you can do.
First, tell me about your impressions  <a href="http://www.msoos.org/">
here</a> and send the link to a friend. Second, you can
<a href="http://www.srlabs.de">contact my employer</a>, and he will be happy
to find a way for us to help you with your SAT problems. Third, you can improve
this system by cloning my
<a href="https://github.com/msoos/cryptominisat">GIT</a> repository. It includes
everything i.e. SQL, PHP, HTML, CSS and more.
</p>

<h2>Acknowledgements</h2>
<p>I would like to thank my employer for letting me play with SAT,
my collegue <a href="http://www.flickr.com/photos/lucamelette/">
Luca Melette</a> for helping me with ideas and coding,
<a href="http://folk.uio.no/vegardno/">Vegard Nossum</a> for the many
discussions we had about visualization,
<a href="http://www.inra.fr/mia/T/katsirelos/">George Katsirelos</a> for
improvement ideas, <a href="http://www.cril.univ-artois.fr/~jabbour/">
Said Jabbour</a> for further improvement ideas,
<a href="http://dygraphs.com/">Dygraphs</a> for the visually pleasing graphs,
<a href="http://www.michelhiemstra.nl/blog/igoogle-like-drag-drop-portal-v20/">
Portal</a> for the drag-and-drop feature and Edward Tufte for all his wonderful
<a href="http://www.edwardtufte.com/tufte/books_vdqi">books</a>.
</p>

<br/>
<p><small>Copyright <a href="http://www.msoos.org">Mate Soos</a>, 2012.
Licensed under <a href="http://creativecommons.org/licenses/by-nc-sa/2.5/">
CC BY-NC-SA 2.5</a></small>
</p>

</body>
</html>
