Description of the classes of the BrowserExt php extension
==========================================================

class PhpBrowser
----------------

### Property downloadDirectory

`public $downloadDirectory;`

Sets the directory where files will be downloaded automatically, if
the browser receives download request. This happens, for example,
when you click on the link to the file. The default is:

    __DIR__.'/download'


### Property proxyCheckThreads

`public $proxyCheckThreads = 5;`

Sets the number of threads to test proxy servers. The default is 5.


### Method __construct()

`public __construct();`

Constructor.


### Method load

`public bool load(string $url[, bool $samewnd=false]);`

Loads the page by the url. $samewnd determines whether
the page will be loaded in the same tab or in a new one. 
If successful load returns true, otherwise false.


### Method title

`public string title();`

Returns the title of the current page.


### Method click

`public bool click(string $xpath[, bool $samewnd=false]);`

Clicks on the element specified by xpath. If the new page is loaded,
it will load in a new tab. To load in the same tab, set the 
$samewnd = true. If successful load returns true, otherwise false.


### Method elements

`public array elements(string $xpath);`

Fetches the document elements by xpath. Returns an array of objects
of class PhpWebElement. If no element is found returns an empty array.


### Method download

`public int download(string $url, string $dest);`

Downloads a file from url. $dest - the saving path, including
the file name. Returns 0 if an error occurred and 1 if the download
was successful.


### Method back

`public back();`

Moves to the previous tab. The current tab is closed.


### Method wait

`public wait(int $seconds);`

Waiting for a time specified by the parameter $seconds.


### Method scroll

`public bool scroll(int $screens);`

Scrolls vertically on the number of screens specified by $screens
parameter. The screen size is determined by the size of the browser window.
If scroll successful, returns true. If you reach the end of the page,
it returns false.

This method may be used for pages that dynamically loads the contents
at the end of a page. The algorithm may be as follows:

    $res = true;
    for ($i=0; $i<500 && $res; $i++)
    {
        $res = $browser->scroll(1);
        if (!$res)
        {
            $browser->wait(3);
            $res = $browser->scroll(1);
        }
    }


### Method fill

`public bool fill(string $xpath, string $value);`

Fills the input element of type text or textarea with $value.
The element is selected by xpath. If xpath returns more
elements is taken only the first.

If the element is found returns true, otherwise false.


### Method check

`public bool check(string $xpath);`

Checks the checkbox, given by xpath expression.
If the element is found returns true, otherwise false.


### Method uncheck

`public bool uncheck(string $xpath);`

Resets the checkbox, given by xpath expression.
If the element is found returns true, otherwise false.


### Method radio

`public bool radio(string $xpath);`

Sets the radio element, given by xpath expression.
If the element is found returns true, otherwise false.


### Method select

`public bool select(string $xpath);`

Selects the select element. $xpath parameter sets the element
with option tag, which is selected.

If the element is found returns true, otherwise false.


### Method selectByText

`public bool selectByText(string $xpath, string $text);`

Sets the select element, given by xpath expression.
$text specifies the text to be selected.

If the element is found returns true, otherwise false.


### Method selectByValue

`public bool selectByValue(string $xpath, string $value);`

Sets the select element, given by xpath expression.
$value specifies the value attribute of the option element
to be selected.

If the element is found returns true, otherwise false.


### Method fillfile

`public bool fillfile(string $xpath, string $value);`

Fills the file input element with $value.
The element is selected by xpath. If xpath returns more
elements is taken only the first.

If the element is found returns true, otherwise false.


### Method setProxyList

`public int setProxyList(array $proxies[, bool ischeck=true]);`


Sets a list of proxies to be used when loading pages and files.
$ischeck parameter determines whether the proxies will be tested.
Proxy server list is given an array of strings, each row identifies
a separate server, for example

    $proxies = array('192.168.0.2:3128', 'user:psw@example.com:8080');

The number of threads to test the proxy server is set by property
proxyCheckThreads.


### Method proxyList

`public array proxyList();`

Returns an array of proxies used in the browser. When proxies were tested,
returning only working servers.


### Method html

`public string html();`

Returns the html of the current page.


### Method show

`public show();`

Shows the browser window.


### Method hide

`public hide();`

Hides the browser window.


### Method url

`public string url();`

Returns the url of the current browser page.


### Method requestedUrl

`public string requestedUrl();`

Returns the requested url of the current browser page.


### Method setHtml

`public setHtml(string $html);`

Sets the html of the current page.
                                  


class PhpWebElement
-------------------

### Method attr

`public string attr(string $name);`

Returns the value of attribute by name.
If the attribute is not found, returns an empty string.


### Method attrAll

`public array attrAll();`

Returns an associative array of name=>value pairs of all
attributes of the element.


### Method tagName

`public string tagName();`    

Returns the element tagname.


### Method prop

`public string prop(string $name);`

Returns the property value of the element.
Member properties are specified by the DOM model.


### Method textAll

`public string textAll();`

Returns the value of text of all child element nodes.
For example, if a $el1 has the following html:

    <div>abc<p>123</p>345</div>

then textAll() returns

    abc 123 345


### Method text

`public string text();`

Returns the text value of only those text nodes
who are the direct child of the element.
For example, if a $el1 has the following html:

    <div>abc<p>123</p>345</div>

then text() returns

    abc 345


### Method html

`public string html();`

Returns the inner html of the element.


### Method getXPath

`public string getXPath();`

Returns the xpath of the element.


### Method parent

`public PhpWebElement parent();`

Returns the parent element. If the parent element is not present,
then the method isNull of the returning element
returns true.

 
### Method nextSibling

`public PhpWebElement nextSibling();`

Returns the next sibling element. If the element is last, the method isNull
of the returning element returns true.

    while (!$el->isNull())
        $el = $el->nextSibling();


### Method prevSibling

`public PhpWebElement prevSibling();`

Returns the previous sibling element. If the element is first,
the method isNull of the returning element returns true.

    while (!$el->isNull())
        $el = $el->prevSibling();


### Method firstChild

`public PhpWebElement firstChild();`

Returns the first child in the child list elements.
If the child is not present, then the method isNull of the
returning element returns true.


### Method lastChild

`public PhpWebElement lastChild();`

Returns the last child in the child list elements.
If the child is not present, then the method isNull of the
returning element returns true.


### Method isNull

`public bool isNull();`

Returns false if it is a normal element, or true if the element is NULL,
i.e. element is missing. See the description of the methods parent,
nextSibling, prevSibling, firstChild, lastChild.


### Method elements

`public array elements(string $xpath);`

Performs xpath with context of this element and returns
an array of objects of class PhpWebElement. If the elements are not found
returns an empty array. For example, to select all the links
inside the $el1 you can write:

    $el1->elements('.//a');

The point specifies the current context.


### Method click

`public bool click();`

Clicks on the element. Returns true if successful, for example,
when a new page is loaded, otherwise provides false.


