#include <iostream>
#include "phpbrowser.h"
#include "proxycheckthread.h"
#include "downloader.h"


void str_utf8_dump(const char *str)
{
	QTextCodec *codec1 = QTextCodec::codecForName("UTF-8");
	QString qstr = codec1->toUnicode(str);
#if defined(WIN32) || defined(_WINDOWS)
	QTextCodec *codec2 = QTextCodec::codecForName("IBM866");
#else
	QTextCodec *codec2 = QTextCodec::codecForLocale();
#endif
	QByteArray qstr2 = codec2->fromUnicode(qstr);
	std::cerr << qstr2.constData() << std::endl;
}


char *strdup_qstring(const QString &str)
{
	QByteArray ba = str.toUtf8();
	int len = ba.length()+1;
	char *ret = new char[len];
	strcpy(ret, ba.constData());
	return ret;
}


PhpBrowser::PhpBrowser(bool canclose)
{
	this->canclose = canclose;
    //std::cerr << "phpbrowser PhpBrowser create" << std::endl;
	resize(800, 500);
	layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
	layout->setContentsMargins(5, 5, 5, 5);
	edit = new QLineEdit(this);
	edit->setReadOnly(true);
	tab = new QTabWidget(this);
	layout->addWidget(edit);
	layout->addWidget(tab);
	ProxyCheckThreads = 5;
	currentProxy = -1;
	isLoadImages = true;
	CookieJar = new QNetworkCookieJar(this);
	connect(tab, SIGNAL(currentChanged(int)), this, SLOT(handleTabChanged(int)));
	connect(edit, SIGNAL(returnPressed()), this, SLOT(handleUrlChanged()));
#if defined(Q_OS_WIN)
	downloadDirectory = "";
#elif defined(Q_OS_LINUX)
	downloadDirectory = ".";
#endif
}


void PhpBrowser::newTab()
{
	PhpWebView *view = new PhpWebView(this);
	PhpWebPage *page = new PhpWebPage(view);

	QNetworkProxy proxy;
	if (getNextProxy(proxy))
		page->networkAccessManager()->setProxy(proxy);

	page->networkAccessManager()->setCookieJar(CookieJar);
	CookieJar->setParent(this);
	page->setForwardUnsupportedContent(true);
	connect(page, SIGNAL(unsupportedContent(QNetworkReply*)), this, SLOT(startDownload(QNetworkReply*)));
	connect(view, SIGNAL(urlChanged(const QUrl &)), this, SLOT(setEdit(const QUrl &)));

	view->setPage(page);
	view->settings()->setAttribute(QWebSettings::AutoLoadImages, isLoadImages);
	QString label = (view->title() == "") ? "Noname" : view->title();
	int pos = tab->addTab(view, label);
	tab->setCurrentIndex(pos);
}


PhpWebView *PhpBrowser::getTab()
{
	if (tab->count() > 0)
		return (PhpWebView *)tab->widget(tab->count()-1);
	else
		return NULL;
}


int PhpBrowser::load(const char* url, bool samewnd, int timeout)
{
	QString qstrurl = QString::fromUtf8(url);

	if (!samewnd) newTab();
	PhpWebView *view = getTab();
	view->load(QUrl(qstrurl));

    QEventLoop loop2;
    QObject::connect(view, SIGNAL(loadFinished(bool)), &loop2, SLOT(quit()));
	if (timeout > 0)
		QTimer::singleShot(timeout*1000, &loop2, SLOT(quit()));
    loop2.exec();
	return view->loadError;
}


void PhpBrowser::back()
{
	if (tab->count() > 1)
	{
		tab->setCurrentIndex(tab->count()-2);
		PhpWebView *w = dynamic_cast<PhpWebView*>(tab->widget(tab->count()-1));
		tab->removeTab(tab->count()-1);
		delete w;
	}
}


void PhpBrowser::show()
{
    QWidget::show();
	tab->show();
}


void PhpBrowser::gettitle(char** ret)
{
	QByteArray title = getTab()->page()->mainFrame()->title().toUtf8();
	int len = title.length()+1;
	*ret = new char[len];
    strcpy(*ret, title.constData());
}


int PhpBrowser::click(const char *xpath, bool samewnd)
{
	if (!getTab()) return 0;

	QApplication::setActiveWindow(this);

	PhpWebView *view = getTab();
	int res = view->click(QString(xpath), samewnd);

	//std::cerr << "test: ";
	//str_utf8_dump(getTab()->title().toUtf8().constData());
	if ((view->isNewViewCreated || samewnd) && getTab()->viewLoadState == 0)
	{
		QEventLoop loop2;
		QObject::connect(getTab(), SIGNAL(loadFinished(bool)), &loop2, SLOT(quit()));
		loop2.exec();
		res = getTab()->loadError;
	}
	return res;
}


int PhpBrowser::click2(WebElementTS *elem, bool samewnd)
{
	QWebElement elem2 = elem->getElement();
	PhpWebView *view = dynamic_cast<PhpWebView*>(elem2.webFrame()->page()->view());
	int res = view->click2(elem2, samewnd);

	if (view->isNewViewCreated || samewnd)
    {
		if (!samewnd && getTab()->viewLoadState == 0)
		{
			QEventLoop loop2;
			QObject::connect(getTab(), SIGNAL(loadFinished(bool)), &loop2, SLOT(quit()));
			loop2.exec();
			res = getTab()->loadError;
		}
		else if (samewnd && view->viewLoadState == 0)
		{
			QEventLoop loop2;
			QObject::connect(view, SIGNAL(loadFinished(bool)), &loop2, SLOT(quit()));
			loop2.exec();
			res = view->loadError;
		}
    }
	return res;
}


int PhpBrowser::fill(const char *xpath, const char *value)
{
	if (!getTab()) return 0;

	QWebElement elem = getTab()->getElementByXPath(QString(xpath));
	//std::cerr << "fill: " << elem.tagName().toLower().toUtf8().constData() << " " << elem.attribute("type").toUtf8().constData() << std::endl;
	if (elem.tagName().toLower() == "input" 
		&& (elem.attribute("type") == "text" || elem.attribute("type") == "password" || elem.attribute("type") == ""))
	{
		elem.setAttribute("value", QString(value));
		elem.evaluateJavaScript("this.value = '"+QString(value)+"'");
		return 1;
	}
	else if (elem.tagName().toLower() == "textarea")
	{
		elem.setPlainText(QString(value));
		elem.evaluateJavaScript("this.value = '"+QString(value)+"'");
		return 1;
	}
	return 0;
}


int PhpBrowser::check(const char *xpath)
{
	if (!getTab()) return 0;

	QWebElement elem = getTab()->getElementByXPath(QString(xpath));
	if (elem.tagName().toLower() == "input" && elem.attribute("type") == "checkbox")
	{
		elem.evaluateJavaScript("this.checked = true");
		return 1;
	}
	return 0;
}


int PhpBrowser::uncheck(const char *xpath)
{
	if (!getTab()) return 0;

	QWebElement elem = getTab()->getElementByXPath(QString(xpath));
	if (elem.tagName().toLower() == "input" && elem.attribute("type") == "checkbox")
	{
		elem.evaluateJavaScript("this.checked = false");
		return 1;
	}
	return 0;
}


int PhpBrowser::radio(const char *xpath)
{
	if (!getTab()) return 0;

	QWebElement elem = getTab()->getElementByXPath(QString(xpath));
	if (elem.tagName().toLower() == "input" && elem.attribute("type") == "radio")
	{
		elem.evaluateJavaScript("this.checked = true");
		return 1;
	}
	return 0;
}


int PhpBrowser::select(const char *xpath)
{
	if (!getTab()) return 0;

	QWebElement elem = getTab()->getElementByXPath(QString(xpath));
	if (elem.tagName().toLower() == "option")
	{
		elem.evaluateJavaScript("this.selected = true");
		return 1;
	}
	return 0;
}


int PhpBrowser::selectbytext(const char *xpath, const char *text)
{
	if (!getTab()) return 0;
	QWebElement elem = getTab()->getElementByXPath(QString(xpath));
	if (elem.tagName().toLower() == "select")
	{
		QString str(text);
		str.replace("\"", "\\\"");
		QString js = "for (var i=0; i<this.options.length; i++) ";
		js +=        "	if (this.options[i].text == \""+str+"\")";
		js +=        "      this.options[i].selected = true;";
		elem.evaluateJavaScript(js);
		return 1;
	}
	return 0;
}


int PhpBrowser::selectbyvalue(const char *xpath, const char *value)
{
	if (!getTab()) return 0;
	QWebElement elem = getTab()->getElementByXPath(QString(xpath));
	if (elem.tagName().toLower() == "select")
	{
		QString str(value);
		str.replace("\"", "\\\"");
		QString js = "for (var i=0; i<this.options.length; i++)";
		js +=        "	if (this.options[i].value == \""+str+"\")";
		js +=        "      this.options[i].selected = true;";
		elem.evaluateJavaScript(js);
		return 1;
	}
	return 0;
}


int PhpBrowser::fillfile(const char *xpath, const char *filename)
{
	if (!getTab()) return 0;
	QWebElement elem = getTab()->getElementByXPath(QString(xpath));	
	if (!elem.isNull())
	{
		dynamic_cast<PhpWebPage*>(getTab()->page())->setFilename(QString(filename));
		elem.setFocus();
		QKeyEvent ev(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
		getTab()->event(&ev);
		return 1;
	}
	return 0;
}


QStringList PhpBrowser::getimglink(const char *xpath)
{
	if (!getTab()) return QStringList();
	QList<QWebElement> list = getTab()->getAllElementsByXPath(QString(xpath));

	/*if (list.size() == 0)
		return NULL;

	char **ret = new char*[list.count()];*/
	QStringList ret;
	for (int i=0; i<list.count(); i++)
	{
		QWebElement elem = list[i];
		//QString str = elem.toPlainText();
		QString str = elem.evaluateJavaScript("this.src").toString();
		//ret[i] = new char[str.length()+1];
		//strcpy_s(ret[i], str.length()+1, str.toUtf8().constData());
		ret.append(str);
	}
	return ret;
}


QList<WebElementTS*> PhpBrowser::getelements(const char *xpath)
{
	if (!getTab()) return QList<WebElementTS*>();
	QList<WebElementTS*> ret;
	QList<QWebElement> list = getTab()->getAllElementsByXPath(QString(xpath));
	for (int i=0; i<list.count(); i++)
	{
		WebElementTS *elts = new WebElementTS(list[i]);
		ret.append(elts);
	}
	return ret;
}


QList<WebElementTS*> PhpBrowser::getelements2(const char *xpath, WebElementTS *node)
{
	if (!getTab()) return QList<WebElementTS*>();
	QList<WebElementTS*> ret;
	QList<QWebElement> list = getTab()->getAllElementsByXPath2(QString(xpath), node->getElement());
	for (int i=0; i<list.count(); i++)
	{
		WebElementTS *elts = new WebElementTS(list[i]);
		ret.append(elts);
	}
	return ret;
}


int PhpBrowser::download(const char *link, const char *dest)
{
    QString desttmp(dest);
    int end1 = desttmp.lastIndexOf('/');
	int end2 = desttmp.lastIndexOf('\\');
	int end = (end1 > end2) ? end1 : end2;
    desttmp = desttmp.remove(end, desttmp.length()-end);
    QDir dir(desttmp);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
	
    Downloader *dl = new Downloader(QString(link), QString(dest));
	QNetworkAccessManager *m = dl->networkAccessManager();
    //dl->setNetworkAccessManager(getTab()->page()->networkAccessManager());
    m->setCookieJar(CookieJar);
    CookieJar->setParent(this);

	QNetworkProxy proxy;
	if (getNextProxy(proxy))
		m->setProxy(proxy);

	int res = dl->download();
    if (res != 0 && getTab())
		dynamic_cast<PhpWebPage*>(getTab()->page())->addToConsole(dl->errorString());

	delete dl;
	return (res == 0) ? 1 : 0;
}


QString PhpBrowser::getxpath(const QWebElement & elem)
{
	QString path = "";
	QWebElement current = elem;
	while (!current.isNull())
	{
		QWebElement parent = current.parent();
		QString No = "";

		/*if (current.hasAttribute("id"))
		{
			No = "[@id=\"" + current.attribute("id") + "\"]";
		}
		else*/ if (current.hasAttribute("class"))
		{
			QString classname = current.attribute("class");
			int pos = 1;
			int total = 0;
			bool isfind = false;
			QWebElement child = parent.firstChild();
			while (!child.isNull())
			{
				if (child == current)
					isfind = true;
				if (child.hasAttribute("class") && child.tagName() == current.tagName() 
						&& child.attribute("class") == classname)
				{
					if (!isfind) pos++;
					total++;
				}
				child = child.nextSibling();
			}
			if (total > 1)
				No = "["+QString::number(pos)+"]";
			No = "[@class=\"" + classname + "\"]" + No;
		}
		else if (!parent.isNull())
		{
			int pos = 1;
			int total = 0;
			bool isfind = false;
			QWebElement child = parent.firstChild();
			while (!child.isNull())
			{
				if (child == current)
					isfind = true;
				if (child.tagName() == current.tagName())
				{
					if (!isfind) pos++;
					total++;
				}
				child = child.nextSibling();
			}
			if (total > 1)
				No = "["+QString::number(pos)+"]";
		}

		path = "/" + current.tagName().toLower() + No + path;
		current = current.parent();
	}
	return path;
}


char *PhpBrowser::console()
{
	QString out;
	if (getTab())
		out = getTab()->console();
	else
		out = "";
	return strdup_qstring(out);
}


void PhpBrowser::wait(int seconds)
{
	if (seconds > 0)
	{
		QEventLoop loop;
		QTimer::singleShot(seconds*1000, &loop, SLOT(quit()));
		loop.exec();
	}
}


int PhpBrowser::scroll(int numscreen)
{
	if (getTab() && numscreen != 0)
	{
		QWebFrame *frame = getTab()->page()->mainFrame();
		QPoint curp = dynamic_cast<PhpWebPage*>(getTab()->page())->lastScroll();
		int h = getTab()->height();
		int bottom = frame->scrollBarMaximum(Qt::Vertical);
		int top = 0;
		
		int ret = 1;
		int newy = curp.y()+h*numscreen;

		if (newy > bottom)
		{
			newy = bottom;
			ret = 0;
		}
		if (newy < top)
		{
			newy = top;
			ret = 0;
		}
		curp.setY(newy);
		frame->setScrollPosition(curp);
		dynamic_cast<PhpWebPage*>(getTab()->page())->setLastScroll(curp);
		return ret;
	}
	return 0;
}


int PhpBrowser::setproxylist(QStringList & strlist, bool ischeck)
{
	ProxyList.clear();
	for (int i=0; i<strlist.count(); i++)
	{
		QString proxystr = strlist.at(i).trimmed();
		QRegExp rx("((\\w*)\\:(\\w*)\\@)?([^:]+):(\\d+)");
		if (rx.indexIn(proxystr, 0) != -1)
		{
			QString user = rx.cap(2);
			QString pass = rx.cap(3);
			QString host = rx.cap(4);
			QString port = rx.cap(5);
			QNetworkProxy proxy(QNetworkProxy::HttpProxy);
			proxy.setHostName(host);
			proxy.setPort(port.toUInt());
			if (user != "")
				proxy.setUser(user);
			if (pass != "")
				proxy.setPassword(pass);
			ProxyList.append(proxy);
		}
	}

	if (ischeck)
	{
		ProxyChecker *checker = new ProxyChecker(ProxyList, ProxyCheckThreads);
		ProxyList = checker->check();
		delete checker;
	}

	if (ProxyList.count() > 0)
		currentProxy = 0;
	else
		currentProxy = -1;

	return ProxyList.count();
}


QStringList PhpBrowser::proxylist()
{
	QStringList strlist;
	for (int i=0; i<ProxyList.count(); i++)
	{
		QNetworkProxy proxy = ProxyList.at(i);
		QString str;
		if (proxy.password() != "")
			str = proxy.user()+":"+proxy.password()+"@";
		strlist.append(str+proxy.hostName()+":"+QString::number(proxy.port()));
	}
	return strlist;
}


void PhpBrowser::closeEvent(QCloseEvent *event)
{
	if (!canclose)
		event->ignore();
}


bool PhpBrowser::getNextProxy(QNetworkProxy & proxy)
{
	if (ProxyList.count() > 0)
	{
		if (++currentProxy >= ProxyList.count())
			currentProxy = 0;
		proxy = ProxyList.at(currentProxy);
		return true;
	}
	return false;
}


char *PhpBrowser::html()
{
	char *ret;
	if (getTab())
	{
		QString html = getTab()->page()->mainFrame()->toHtml();
		ret = strdup_qstring(html);
		return ret;
	}

	ret = new char[10];
    strcpy(ret, "");
	return ret;
}


void PhpBrowser::setEdit(const QUrl & url)
{
	edit->setText(url.toString());
}


void PhpBrowser::handleTabChanged(int index)
{
	if (index >= 0)
	{
		QString url = dynamic_cast<PhpWebView*>(tab->widget(index))->url().toString();
		if (url != "")
			edit->setText(url);
	}
}


void PhpBrowser::editableUrl(bool editable)
{
	edit->setReadOnly(!editable);
}


void PhpBrowser::handleUrlChanged()
{
	newTab();
	PhpWebView *view = getTab();
	QString str = edit->text();
	view->load(QUrl(str));
}


char* PhpBrowser::url()
{
	QString url;
	if (getTab())
		url = getTab()->page()->mainFrame()->url().toString();
	char *ret = strdup_qstring(url);
	return ret;
}


char* PhpBrowser::requestedurl()
{
	QString url;
	if (getTab())
		url = getTab()->page()->mainFrame()->requestedUrl().toString();
	char *ret = strdup_qstring(url);
	return ret;
}


void PhpBrowser::setDownloadDir(const char *dir)
{
	downloadDirectory = QString::fromUtf8(dir);
}


char* PhpBrowser::getDownloadDir()
{
	char *ret = strdup_qstring(downloadDirectory);
	return ret;
}


void PhpBrowser::startDownload(QNetworkReply *reply)
{
	QString slash;
	if (downloadDirectory.at(downloadDirectory.size()-1) != '/')
		slash = "/";
	QString dest = downloadDirectory+slash+reply->url().host()+reply->url().path();
	QString desttmp = dest;
	int end = desttmp.lastIndexOf('/');
	desttmp = desttmp.remove(end, desttmp.length()-end);

	QDir dir(desttmp);
	if (!dir.exists()) {
		dir.mkpath(".");
	}

	Downloader *dl = new Downloader(reply, dest);
	QNetworkAccessManager *m = dl->networkAccessManager();
	m->setCookieJar(CookieJar);
	CookieJar->setParent(this);

	QNetworkProxy proxy;
	if (getNextProxy(proxy))
		m->setProxy(proxy);

	int res = dl->download();
	if (res != 0)
		dynamic_cast<PhpWebPage*>(getTab()->page())->addToConsole(dl->errorString());

	delete dl;
}


void PhpBrowser::setHtml(const char *html, const char *url)
{
	newTab();
	QTextCodec *codec = QTextCodec::codecForName("UTF-8");
	QTextCodec *codec2 = QTextCodec::codecForHtml(QByteArray(html), codec);
	QString str = codec2->toUnicode(html);
	getTab()->page()->mainFrame()->setHtml(str, QUrl(QString(url)));
}


void PhpBrowser::setImageLoading(bool isload)
{
	isLoadImages = isload;
}


int PhpBrowser::jsexec(WebElementTS *elem, const char *js)
{
	if (!getTab()) return 0;
	QWebElement elem2 = elem->getElement();
	elem2.evaluateJavaScript(QString(js));
	return 1;
}