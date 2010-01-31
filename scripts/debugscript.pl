#!/usr/bin/perl

$var1 = 59;
$var2 = 61;
$vars = "($var1|$var2)";
$group = 645454;
$valami = '(((var)|(lit)) \b-?'.$vars.'\b)|(Replacing.*\b'.$vars.'\b)|(Statistics)|(Extending.*\b'.$vars.':)|( \| )|(after replacing.*\b-?'.$vars.'\b)|(after)|(after.*'.$vars.')|(Final.*\b-?'.$vars.'\b)|(group: '.$group.')|(doCalc.*\b-?'.$vars.'\b)|(^Var:.*'.$vars.')';
$myexec= 'grep -E "'.$valami.'" output';
exec $myexec
