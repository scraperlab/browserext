#include <iostream>
#include <QtAlgorithms>
#include "xpathinspector.h"
#include "phpbrowser.h"


void WebElementBridge::passElement(const QWebElement & elem)
{
	list.append(elem);
}


void WebElementBridge::clear()
{
	list.clear();
}



PhpWebView::PhpWebView(PhpBrowser *browser, QWidget *parent)
	: QWebView(parent)
{
	this->browser = browser;
	viewLoadState = 0;
	connect(this, SIGNAL(loadFinished(bool)), this, SLOT(setTargetBlank()));
	connect(this, SIGNAL(loadFinished(bool)), this, SLOT(setTabName(bool)));
	connect(this, SIGNAL(loadStarted()), this, SLOT(handleLoadStarted()));
}


int PhpWebView::click(const QString & xpath, bool samewnd)
{
	QWebElement elem = getElementByXPath(QString(xpath));
	
	if (elem.isNull())
		return 0;

	elem.setFocus();

	if (samewnd)
		elem.removeAttribute("target");

	QString js = "var node = this; var x = node.offsetLeft; var y = node.offsetTop; ";
	js += "var w = node.offsetWidth/2; ";
	js += "var h = node.offsetHeight/2; ";
	js += "while (node.offsetParent != null) { ";
	js += "   node = node.offsetParent; ";
	js += "   x += node.offsetLeft; ";
	js += "   y += node.offsetTop; ";
	js += "} ";
	js += "[x+w, y+h]; ";
	QList<QVariant> vlist = elem.evaluateJavaScript(js).toList();
	QPoint point;
	point.setX(vlist.at(0).toInt());
	point.setY(vlist.at(1).toInt());

	QRect elGeom = elem.geometry();
	QPoint elPoint = elGeom.center();
	int elX = point.x(); //elPoint.x();
	int elY = point.y(); //elPoint.y();
	int webWidth = width();
	int webHeight = height();
	int pixelsToScrollRight=0;
	int pixelsToScrollDown=0;
	if (elX > webWidth)
		pixelsToScrollRight = elX-webWidth+elGeom.width()/2+50; //the +10 part if for the page to scroll a bit further
	if (elY > webHeight)
		pixelsToScrollDown = elY-webHeight+elGeom.height()/2+50; //the +10 part if for the page to scroll a bit further
	
	/*pixelsToScrollRight = elX-elGeom.width()/2-50;
	pixelsToScrollDown = elY-elGeom.height()/2-50;

	if (pixelsToScrollRight < 0)
		pixelsToScrollRight = 0;
	if (pixelsToScrollRight > page()->mainFrame()->scrollBarMaximum(Qt::Horizontal))
		pixelsToScrollRight = page()->mainFrame()->scrollBarMaximum(Qt::Horizontal);
	if (pixelsToScrollDown < 0)
		pixelsToScrollDown = 0;
	if (pixelsToScrollDown > page()->mainFrame()->scrollBarMaximum(Qt::Vertical))
		pixelsToScrollDown = page()->mainFrame()->scrollBarMaximum(Qt::Vertical);*/

	int oldHoriz = page()->mainFrame()->scrollBarValue(Qt::Horizontal);
	int oldVert = page()->mainFrame()->scrollBarValue(Qt::Vertical);

	page()->mainFrame()->setScrollBarValue(Qt::Horizontal, pixelsToScrollRight);
	page()->mainFrame()->setScrollBarValue(Qt::Vertical, pixelsToScrollDown);
	QPoint pointToClick(elX-pixelsToScrollRight, elY-pixelsToScrollDown);

	QEventLoop loop;
	isNewViewCreated = false;
	isNewViewBegin = false;
	
	//QMouseEvent *pressEvent = new QMouseEvent(QMouseEvent::MouseButtonPress, pointToClick, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
	//QApplication::postEvent(browser, pressEvent);
	//QApplication::processEvents();

	//QMouseEvent releaseEvent(QMouseEvent::MouseButtonRelease,pointToClick,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
	//QApplication::sendEvent(browser, &releaseEvent);

	QString js2 = "var e = document.createEvent('MouseEvents');";
    //js2 += "e.initEvent( 'click', true, true );";
	//js2 += "e.initMouseEvent('click', true, true, window, 0, 0, 0, "+QString::number(elX)+", "+QString::number(elY)+", false, false, false, false, 0, null);";
	js2 += "e.initMouseEvent('click', true, true, window, 0, 0, 0, 0, 0, false, false, false, false, 0, null);";
    js2 += "this.dispatchEvent(e);";
	elem.evaluateJavaScript(js2);
	
	QTimer::singleShot(1000, &loop, SLOT(quit()));
	loop.exec();
	
	while (browser && isNewViewBegin && !isNewViewCreated)
		loop.processEvents();

	//page()->mainFrame()->setScrollBarValue(Qt::Horizontal, oldHoriz);
	//page()->mainFrame()->setScrollBarValue(Qt::Vertical, oldVert);
	return 1;
}


int PhpWebView::click2(QWebElement elem, bool samewnd)
{
	if (elem.isNull())
		return 0;

	if (samewnd)
		elem.removeAttribute("target");

	QEventLoop loop;
	isNewViewCreated = false;
	isNewViewBegin = false;
	
	QString js = "var e = document.createEvent('MouseEvents');";
	//js += "e.initEvent( 'click', true, true );";
    js += "e.initMouseEvent( 'click', true, true, window, 0, 0, 0, 0, 0, false, false, false, false, 0, null);";
    js += "this.dispatchEvent(e);";
	elem.evaluateJavaScript(js);
	
	QTimer::singleShot(1000, &loop, SLOT(quit()));
	loop.exec();
	
	while (browser && isNewViewBegin && !isNewViewCreated)
		loop.processEvents();

	return 1;
}


void PhpWebView::contextMenuEvent(QContextMenuEvent * ev)
{
	QMenu *menu = this->page()->createStandardContextMenu();
	rightClick = ev->pos();
	menu->addSeparator();
	menu->addAction("Get XPath", this, SLOT(showXPath()));
	if (browser)
		menu->addAction("Close tab", this, SLOT(closeTab()));
	menu->exec(ev->globalPos());
	delete menu;
}


QList<QWebElement> PhpWebView::getAllElementsByXPath(const QString & xpath)
{
	QWebElement doc = page()->mainFrame()->documentElement();

    QString js = "var result = document.evaluate('"+xpath+"', document, null, XPathResult.ORDERED_NODE_ITERATOR_TYPE, null); ";
	js += "WebElementBridge.clear(); ";
	js += "var node = result.iterateNext(); ";
    js += "while (node) { ";
    js += " 	WebElementBridge.passElement(node); ";
    js += " 	node = result.iterateNext(); ";
    js += "} ";
	doc.evaluateJavaScript(js);
	
	QList<QWebElement> coll = webridge.list;
	return coll;
}


QList<QWebElement> PhpWebView::getAllElementsByXPath2(const QString & xpath, QWebElement node)
{
	QString js = "var result = document.evaluate('"+xpath+"', this, null, XPathResult.ORDERED_NODE_ITERATOR_TYPE, null); ";
	js += "WebElementBridge.clear(); ";
	js += "var node = result.iterateNext(); ";
    js += "while (node) { ";
    js += " 	WebElementBridge.passElement(node); ";
    js += " 	node = result.iterateNext(); ";
    js += "} ";
	node.evaluateJavaScript(js);

	QList<QWebElement> coll = webridge.list;
	return coll;
}


QWebElement PhpWebView::getElementByXPath(const QString & xpath)
{
    QList<QWebElement> coll = getAllElementsByXPath(xpath);
	if (coll.count() > 0)
		return coll.at(0);
	else
		return QWebElement();
}


WebElementStruct PhpWebView::createWebElementStruct(QWebElement elem, int i)
{
	WebElementStruct we;
	we.el = elem;
	we.position = elem.styleProperty("position", QWebElement::ComputedStyle);
	we.z_index = 0;
	we.i = i;
	
	QWebElement cur = elem;
	while (!cur.isNull())
	{
		QString z = cur.styleProperty("z-index", QWebElement::ComputedStyle);
		bool res;
		int zi = z.toInt(&res);
		if (res)
		{
			we.z_index = zi;
			break;
		}
		cur = cur.parent();
	}

	cur = elem;
	while (!cur.isNull())
	{
		QString pos = cur.styleProperty("position", QWebElement::ComputedStyle);
		if (pos != "inherit")
		{
			we.position = pos;
			break;
		}
		cur = cur.parent();
	}
	
	return we;
}


bool PhpWebView::findElementByCoord(QWebElement & root, QPoint & point, QList<WebElementStruct> & list)
{
	QWebElement child = root.lastChild();
	bool isfind = false;
	while (!child.isNull())
	{
		if (findElementByCoord(child, point, list))
			isfind = true;
		child = child.previousSibling();
	}

	QString display = root.styleProperty("display", QWebElement::ComputedStyle);
	if (display != "none" && root.geometry().contains(point) && !isfind)
	{
		list.append(createWebElementStruct(root, list.count()));
		return true;
	}
	else
		return false;
}


int positionToInt(QString pos)
{
	if (pos == "absolute")
		return 2;
	else if (pos == "fixed")
		return 2;
	else if (pos == "relative")
		return 2;
	else if (pos == "static")
		return 1;
	return 1;
}


bool elementLessThan(const WebElementStruct &el1, const WebElementStruct &el2)
{
	if (el1.z_index == el2.z_index)
	{
		//if (positionToInt(el1.position) == positionToInt(el2.position))
			return el1.i > el2.i;

		//return positionToInt(el1.position) < positionToInt(el2.position);
	}
	else
		return el1.z_index < el2.z_index;
}


QWebElement PhpWebView::getElementByCoord(QWebElement root, QPoint point)
{
	QList<WebElementStruct> list;
	findElementByCoord(root, point, list);
	qSort(list.begin(), list.end(), elementLessThan);

	/*for (int i=list.count()-1; i>=0; i--)
	{
		WebElementStruct el = list.at(i);
		QString tag = el.el.tagName();
		QString id = el.el.attribute("id", "none");
		QString pos = el.position;
		QString z = QString::number(el.z_index);
		std::cerr << tag.toUtf8().constData() << " " << QString::number(el.i).toUtf8().constData() << " [" << id.toUtf8().constData() << "]" << " " << pos.toUtf8().constData() << " " << z.toUtf8().constData() << std::endl;
	}*/
		
	if (list.count() > 0)
		return list.last().el;

	return QWebElement();
}


void PhpWebView::showXPath()
{
	QWebFrame *frame = page()->frameAt(rightClick);
	if (frame)
	{
		XPathInspector *dlg = new XPathInspector(this, this);

		if (selectedElements.count() == 0)
		{
			QPoint point = frame->scrollPosition();
			point += rightClick;
            QWebElement elem = getElementByCoord(frame->documentElement(), point);
			dlg->setActiveElement(elem);
		}
		else
		{
			dlg->setFewActiveElements(selectedElements);
		}
		dlg->show();
	}
}

void PhpWebView::closeTab()
{
	if (browser)
	{
		int ind = browser->tab->indexOf(this);
		browser->tab->removeTab(ind);
		this->deleteLater();
	}
}

QWebView* PhpWebView::createWindow(QWebPage::WebWindowType type)
{
	if (browser)
	{
		browser->newTab();
		isNewViewCreated = true;
		emit newViewCreated();
	    return browser->getTab();
	}
	else
		return NULL;
}


void PhpWebView::setTargetBlank()
{
	QWebElementCollection coll = page()->mainFrame()->documentElement().findAll("a");
	for (int i=0; i<coll.count(); i++)
		coll.at(i).setAttribute("target", "_blank");

	QWebElementCollection coll2 = page()->mainFrame()->documentElement().findAll("form");
	for (int i=0; i<coll2.count(); i++)
		coll2.at(i).setAttribute("target", "_blank");
}


void PhpWebView::setTabName(bool ok)
{
	if (browser)
	{
		int ind = browser->tab->indexOf(this);
		QString t = title();
		if (t != "")
			browser->tab->setTabText(ind, t);
	}
	viewLoadState = 1;  //страница загружена
	if (ok) loadError = 1;
	else loadError = 0;
	page()->mainFrame()->addToJavaScriptWindowObject("WebElementBridge", &webridge);
}


QString PhpWebView::console()
{
	return dynamic_cast<PhpWebPage*>(page())->console();
}


void PhpWebView::clearConsole()
{
	dynamic_cast<PhpWebPage*>(page())->clearConsole();
}


void PhpWebView::setUserAgent(const QString & ua)
{
	dynamic_cast<PhpWebPage*>(page())->setUserAgent(ua);
}


void PhpWebView::setPage(PhpWebPage *page)
{
	//network->setCookieJar(page->networkAccessManager()->cookieJar());
	connect(page->networkAccessManager(), SIGNAL(finished(QNetworkReply *)), this, SLOT(requestFinished(QNetworkReply *)));
	QWebView::setPage(page);
}


void PhpWebView::requestFinished(QNetworkReply *reply)
{
	//std::cerr << "request finished" << std::endl;
	QString url = reply->request().url().toString();
	QString url2 = page()->mainFrame()->url().toString();
	if (url == url2)
	{
		switch (reply->error())
		{
			case QNetworkReply::NoError:
				return;
			default:
				QString status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
				QString url = reply->request().url().toString();
				page()->mainFrame()->setHtml("<html><body><p>"+url+"</p><p style=\"font-size:20pt\">Status: "+status+"</p></body></html>");
		}
	}
}


void PhpWebView::selectElement(QWebElement & elem, bool setselect)
{
	if (!elem.isNull())
	{
		QString js = "(this.mydataSelected) ? true : false;";
		bool isSel = elem.evaluateJavaScript(js).toBool();
		if (!isSel && setselect)
		{
			QString oldstyle = elem.styleProperty("border", QWebElement::ComputedStyle);
			elem.setStyleProperty("border", "2px dashed red");
			js = "this.mydataOldBorder = '"+oldstyle+"'; ";
			js += "this.mydataSelected = 1; ";
			elem.evaluateJavaScript(js);
			selectedElements.append(elem);
		}
		else if (isSel && !setselect)
		{
			QString oldstyle = elem.evaluateJavaScript("this.mydataOldBorder;").toString();
			elem.setStyleProperty("border", oldstyle);
			js += "this.mydataSelected = 0;";
			elem.evaluateJavaScript(js);
			selectedElements.removeAll(elem);
		}
	}
}


bool PhpWebView::event(QEvent *e)
{
	if (e->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent *me = dynamic_cast<QMouseEvent*>(e);
		if (me->modifiers() & Qt::ControlModifier)
		{
			QPoint pos = me->pos();
			QWebFrame *frame = page()->frameAt(pos);
			if (frame)
			{
				QPoint point = frame->scrollPosition();
				point += pos;
				QWebElement elem = getElementByCoord(page()->mainFrame()->documentElement(), point);
				QString js = "(this.mydataSelected) ? true : false;";
				bool isSel = elem.evaluateJavaScript(js).toBool();
				selectElement(elem, !isSel);
				return true;
			}
		}
	}
	return QWebView::event(e);
}


void PhpWebView::handleLoadStarted()
{
	viewLoadState = 0;
}

