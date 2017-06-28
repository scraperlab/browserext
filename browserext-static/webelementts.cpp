#include "phpbrowser.h"

WebElementTS::WebElementTS(const QWebElement & elem)
{
	element = elem;
}


WebElementTS::WebElementTS(const WebElementTS & elem)
{
	element = elem.element;
}


QWebElement WebElementTS::getElement()
{
	return element;
}


char *WebElementTS::attribute(const char *name)
{
	QString qname = name;
	QString attr = element.attribute(qname);
	char *ret = strdup_qstring(attr);
	return ret;
}


QStringList WebElementTS::attributeNames()
{
	return element.attributeNames();
}


char *WebElementTS::tagName()
{
	QString tag = element.tagName();
	char *ret = strdup_qstring(tag);
	return ret;
}


char *WebElementTS::prop(const char *name)
{
	QString result = element.evaluateJavaScript("this."+QString(name)).toString();
	char *ret = strdup_qstring(result);
	return ret;
}


WebElementTS *WebElementTS::parent()
{
	WebElementTS *ret = new WebElementTS(element.parent());
	return ret;
}


char *WebElementTS::textAll()
{
	QString text = element.toPlainText();
	char *ret = strdup_qstring(text);
	return ret;
}


char *WebElementTS::text()
{
	QString js = "var str = '';\n";
    js +=        "for (var i=0; i<this.childNodes.length; i++) \n";
    js +=        "   if (this.childNodes[i].nodeType == 3) \n";
    js +=        "      str += ' '+this.childNodes[i].nodeValue; \n";
    js +=        "str.substring(1); ";
    QString text = element.evaluateJavaScript(js).toString();
	char *ret = strdup_qstring(text);
	return ret;
}


WebElementTS *WebElementTS::nextSibling()
{
	WebElementTS *ret = new WebElementTS(element.nextSibling());
	return ret;
}


WebElementTS *WebElementTS::prevSibling()
{
	WebElementTS *ret = new WebElementTS(element.previousSibling());
	return ret;
}


WebElementTS *WebElementTS::firstChild()
{
	WebElementTS *ret = new WebElementTS(element.firstChild());
	return ret;
}


WebElementTS *WebElementTS::lastChild()
{
	WebElementTS *ret = new WebElementTS(element.lastChild());
	return ret;
}


bool WebElementTS::isNull()
{
	return element.isNull();
}


char* WebElementTS::getXPath()
{
	QString path = PhpBrowser::getxpath(element);
	char *ret = strdup_qstring(path);
	return ret;
}


char* WebElementTS::html()
{
	QString html = element.toOuterXml();
	char *ret = strdup_qstring(html);
	return ret;
}


bool WebElementTS::operator!=(const WebElementTS & o) const
{
	return element != o.element;
}


bool WebElementTS::operator==(const WebElementTS & o) const
{
	return element == o.element;
}


WebElementTS & WebElementTS::operator=(const WebElementTS & other)
{
	element = other.element;
	return *this;
}
