<h2>CryptoMiniSat commit log</h2>
<?php
$doc = new DOMDocument();
$doc->load('http://gitorious.com/cryptominisat.atom');
$arrFeeds = array();
foreach ($doc->getElementsByTagName('entry') as $node) {
  $itemRSS = array (
    'title' => $node->getElementsByTagName('title')->item(0)->nodeValue,
    'desc' => $node->getElementsByTagName('description')->item(0)->nodeValue,
    'link' => $node->getElementsByTagName('link')->item(0)->nodeValue,
    'content' => $node->getElementsByTagName('content')->item(0)->nodeValue,
    'date' => $node->getElementsByTagName('pubDate')->item(0)->nodeValue
  );
  array_push($arrFeeds, $itemRSS);
}

echo '<table>';
#echo "<th>CryptoMinisat source commit log</th>";
$i = 0; foreach ($arrFeeds as $item) {
  if ($i > 2) break;
  #echo $item['title'];
  echo "<tr><td>";
  //<p id='info'>";
  #echo $item["content"];
  $disp = str_replace("a href=\"", "a href=\"http://gitorious.com", $item['content']);
  echo $disp;
  echo "</td></tr>"; $i++;
}
echo "</table>";
#var_dump($arrFeeds);
?>