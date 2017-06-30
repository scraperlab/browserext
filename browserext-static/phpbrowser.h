#ifndef PHPBROWSER_H
#define PHPBROWSER_H

/* Header file from browserext-static library */

#include <QtWidgets>
#include <QtWebKit>
#include <QtWebKitWidgets/QtWebKitWidgets>
#include <QtNetwork>
#include <QWaitCondition>
#include <QMutex>


typedef QMap<QString,QString> QMapParams;

class PhpBrowser;
class XPathInspector;


class PhpWebPage : public QWebPage
{
	Q_OBJECT

public:
	PhpWebPage(QObject * parent = 0);
	QString console();
	void clearConsole();
	void addToConsole(const QString & str);
	void setLastScroll(QPoint & point);
	QPoint lastScroll();
	void setFilename(const QString & filename);
	void setUserAgent(const QString & ua);

protected Q_SLOTS:
	void handleSslErrors(QNetworkReply* reply, const QList<QSslError> &errors);

protected:
	virtual void javaScriptAlert(QWebFrame * frame, const QString & msg); 
	virtual bool javaScriptConfirm(QWebFrame * frame, const QString & msg);
	virtual void javaScriptConsoleMessage(const QString & message, int lineNumber, const QString & sourceID);
	virtual bool javaScriptPrompt(QWebFrame * frame, const QString & msg, const QString & defaultValue, QString * result);
	virtual bool acceptNavigationRequest(QWebFrame * frame, const QNetworkRequest & request, NavigationType type);
	virtual QString chooseFile(QWebFrame *parentFrame, const QString & suggestedFile);
	virtual QString userAgentForUrl(const QUrl & url) const;

	QString output;
	QPoint last_scroll;
	QString filename;
	QString useragent;
};


struct WebElementStruct {
	QWebElement el;
	int z_index;
	QString position;
	int i;
};


class WebElementBridge : public QObject
{
	Q_OBJECT

public Q_SLOTS:
    void passElement(const QWebElement & elem);
	void clear();
public:
	QList<QWebElement> list;
};

    
class PhpWebView : public QWebView
{
	Q_OBJECT

public Q_SLOTS:
    int click(const QString &xpath, bool samewnd = false);
	int click2(QWebElement elem, bool samewnd = false);

protected Q_SLOTS:
	void showXPath();
	void closeTab();
	void setTargetBlank();
	void setTabName(bool ok);
	void requestFinished(QNetworkReply *reply);
	void handleLoadStarted();

Q_SIGNALS:
	void newViewCreated();

protected:
	virtual void contextMenuEvent(QContextMenuEvent * ev);
	virtual QWebView* createWindow(QWebPage::WebWindowType type);
	WebElementStruct createWebElementStruct(QWebElement elem, int i);
	bool findElementByCoord(QWebElement & root, QPoint & point, QList<WebElementStruct> & list);
	virtual bool event(QEvent *e);

public:
    PhpWebView(PhpBrowser *browser = 0, QWidget *parent = 0);
	void setPage(PhpWebPage *page);
	static QString getxpath(const QWebElement & elem);
	QList<QWebElement> getAllElementsByXPath(const QString & xpath);
    QList<QWebElement> getAllElementsByXPath2(const QString & xpath, QWebElement node);
	QWebElement getElementByXPath(const QString & xpath);
    QWebElement getElementByCoord(QWebElement root, QPoint point);
	void selectElement(QWebElement & elem, bool setselect = true);
	QString console();
	void clearConsole();
	void setUserAgent(const QString & ua);

	friend class PhpWebPage;
	friend class PhpBrowser;
	friend class XPathInspector;

protected:
	QPoint rightClick;
	PhpBrowser *browser;
	bool isNewViewCreated;
	bool isNewViewBegin;
	int viewLoadState;
	WebElementBridge webridge;
	int loadError;
	QList<QWebElement> selectedElements;
};


class WebElementTS : public QObject
{
	Q_OBJECT

public Q_SLOTS:
	char *attribute(const char *name);
	QStringList attributeNames();
	char *tagName();
	char *prop(const char *name);
	WebElementTS *parent();
	char *textAll();
	char *text();
	WebElementTS *nextSibling();
	WebElementTS *prevSibling();
	WebElementTS *firstChild();
	WebElementTS *lastChild();
	bool isNull();
	char* getXPath();
	char* html();

public:
	WebElementTS(const QWebElement & elem);
	WebElementTS(const WebElementTS & elem);
	QWebElement getElement();
	bool operator!=(const WebElementTS & o) const;
	bool operator==(const WebElementTS & o) const;
	WebElementTS & operator=(const WebElementTS & other);

protected:
	QWebElement element;
};


class PhpBrowser : public QWidget
{
    Q_OBJECT

public Q_SLOTS:
    void show();
    int load(const char* url, bool samewnd = false, int timeout = 0);
	void back();
    void gettitle(char** ret);
	int click(const char* xpath, bool samewnd = false);
	int click2(WebElementTS *elem, bool samewnd = false);
	int fill(const char *xpath, const char *value);
	int fill2(const char *xpath, const char *value);
	int check(const char *xpath);
	int uncheck(const char *xpath);
	int radio(const char *xpath);
	int select(const char *xpath);
	int selectbytext(const char *xpath, const char *text);
	int selectbyvalue(const char *xpath, const char *value);
	int fillfile(const char *xpath, const char *filename);
	int download(const char *link, const char *dest);
	QStringList getimglink(const char *xpath);
	QStringList gettext(const char *xpath);
	QStringList getlink(const char *xpath);
	QStringList getattr(const char *xpath, const char *attr);
	QList<WebElementTS*> getelements(const char *xpath);
	QList<WebElementTS*> getelements2(const char *xpath, WebElementTS *node);
	char *console();
	void clearConsole();
	void wait(int seconds);
	int scroll(int numscreen);
	int setproxylist(QStringList & strlist, bool ischeck = true, QString site = "http://www.google.com", QString findstr = "google");
	int setproxylist2(QStringList & strlist, bool ischeck = true);
	QStringList proxylist();
	char *html();
	char *url();
	char *requestedurl();
	void setDownloadDir(const char *dir);
	char* getDownloadDir();
	void setHtml(const char *html, const char *url = "");
	void setImageLoading(bool isload);
	int jsexec(WebElementTS *elem, const char *js);
	int submitform(const char *xpath, QMapParams params, bool samewnd = false, QString action2 = "", QString method = "POST", int timeout = 20, bool nobrowser = false, QString wrapper = "{%content%}");
	QMapParams getCookies();
	void clearCookies();
	QString getCurrentProxy();
	char *getCurrentProxy2();
	void sendEvent(const QString& type, const QVariant& arg1 = QVariant(), const QVariant& arg2 = QVariant(), const QString& mouseButton = QString(), const QVariant& modifierArg = QVariant());
	void setUserAgent(const char *ua);
	void setCookiesForUrl(const char* url, QMapParams & qmap);

protected Q_SLOTS:
	void setEdit(const QUrl & url);
	void handleTabChanged(int index);
	void handleUrlChanged();
	void startDownload(QNetworkReply *reply);
	void handleSslErrors(QNetworkReply* reply, const QList<QSslError> &errors);

protected:
	void newTab();
	PhpWebView *getTab();
	virtual void closeEvent(QCloseEvent *event);

public:
    PhpBrowser(bool canclose = false);
	void editableUrl(bool editable);
	static QString getxpath(const QWebElement & elem);
	bool getNextProxy(QNetworkProxy & proxy);

	friend class PhpWebView;

protected:
	QTabWidget *tab;
	QLineEdit *edit;
	QBoxLayout *layout;
	QList<QNetworkProxy> ProxyList;
public:
	QNetworkCookieJar *CookieJar;
	QNetworkDiskCache *diskCache;
protected:
	int currentProxy;
	bool canclose;
	bool isLoadImages;
	QString useragent;
public:
	int ProxyCheckThreads;
	QString downloadDirectory;
	bool isUseCache;
};


class Initializer : public QObject
{
    Q_OBJECT

    QWidget *mainwindow;
    bool isMainLoopRunnig;

public Q_SLOTS:
    void createApp();
    void setActiveBrowser(PhpBrowser* browser);
    QApplication* getApp();
    QThread* getAppThread();
    PhpBrowser* createPhpBrowser();
	void deleteWebElement(QWebElement *elem);

public:
    Initializer();
    ~Initializer();
    bool is_main_loop_runnig();
};



class GuiThread : public QThread
{
	Q_OBJECT

	Initializer *init;
    QWaitCondition *wc;

public:
    GuiThread(QWaitCondition *wc = 0);
	void run();
	Initializer *getInitObj();
};


#ifdef Q_OS_WIN
	unsigned __stdcall guithread(void *arg);
#elif defined(Q_OS_LINUX)
	void *guithread(void *arg);
#endif

Initializer *getInitializer();


void str_utf8_dump(const char *str);
char *strdup_qstring(const QString &str);

#endif // PHPBROWSER_H
