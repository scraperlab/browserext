<?php

$browser = new PhpBrowser();
$browser->load('http://www.google.com');
echo $browser->title();


$b2 = new PhpBrowser();
$b2->load('http://www.ya.ru');
echo $b2->title();


?>
