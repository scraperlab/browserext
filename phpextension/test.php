<?php

$browser = new PhpBrowser();
$browser->load('http://www.google.com');
echo $browser->gettitle();


$b2 = new PhpBrowser();
$b2->load('http://www.ya.ru');
echo $b2->gettitle();


?>
