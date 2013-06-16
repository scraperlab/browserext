#ifndef PHPBROWSER_H
#define PHPBROWSER_H

/* Header file from browserext-static library */

#include <QtGui>
#include <QtWebKit>
#include <QtNetwork>
#include <QWaitCondition>
#include <QMutex>


class PhpBrowser;


class PhpWebPage : public QWebPage
{
	Q_OBJECT

public:
	PhpWebPage(QObject * parent = 0);
	QString console();
	void addToConsole(const QString & str);
	void setLastScroll(QPoint & point);
	QPoint lastScroll();
	void setFilename(const QString & filename);

protected:
	virtual void javaScriptAlert(QWebFrame * frame, const QString & msg); 
	virtual bool javaScriptConfirm(QWebFrame * frame, const QString & msg);
	virtual void javaScriptConsoleMessage(const QString & message, int lineNumber, const QString & sourceID);
	virtual bool javaScriptPrompt(QWebFrame * frame, const QString & msg, const QString & defaultValue, QString * result);
	virtual bool acceptNavigationRequest(QWebFrame * frame, const QNetworkRequest & request, NavigationType type);
	virtual QString chooseFile(QWebFrame *parentFrame, const QString & suggestedFile);

	QString output;
	QPoint last_scroll;
	QString filename;
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

	friend class PhpWebPage;
	friend class PhpBrowser;

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
	int check(const char *xpath);
	int uncheck(const char *xpath);
	int radio(const char *xpath);
	int select(const char *xpath);
	int selectbytext(const char *xpath, const char *text);
	int selectbyvalue(const char *xpath, const char *value);
	int fillfile(const char *xpath, const char *filename);
	int download(const char *link, const char *dest);
	QStringList getimglink(const char *xpath);
	QList<WebElementTS*> getelements(const char *xpath);
	QList<WebElementTS*> getelements2(const char *xpath, WebElementTS *node);
	char *console();
	void wait(int seconds);
	int scroll(int numscreen);
	int setproxylist(QStringList & strlist, bool ischeck = true);
	QStringList proxylist();
	char *html();
	char *url();
	char *requestedurl();
	void setDownloadDir(const char *dir);
	char* getDownloadDir();
	void setHtml(const char *html, const char *url = "");

protected Q_SLOTS:
	void setEdit(const QUrl & url);
	void handleTabChanged(int index);
	void handleUrlChanged();
	void startDownload(QNetworkReply *reply);

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
	QNetworkCookieJar *CookieJar;
	int currentProxy;
	bool canclose;
public:
	int ProxyCheckThreads;
	QString downloadDirectory;
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
