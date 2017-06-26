<?php

$br = new PhpBrowser();
$br->load('http://scraperlab.com/en/docs');
echo $br->title()."\n";

$elems = $br->elements('//div[@class="contents"]//a');
foreach ($elems as $el)
{
    //$br->load($el->prop('href'));
    $br->click($el->getXPath());
    $title = $br->elements('//h1');
    echo $title[0]->text()."\n";
    $br->back();
}



?>