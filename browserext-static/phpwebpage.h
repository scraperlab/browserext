#ifndef PHPWEBPAGE_H
#define PHPWEBPAGE_H

#include <QtGui>
#include <QtWebKit>


class PhpWebPage : public QWebPage
{
	Q_OBJECT

public:
	PhpWebPage(QObject * parent = 0);
	QString console();

protected:
	virtual void javaScriptAlert(QWebFrame * frame, const QString & msg); 
	virtual bool javaScriptConfirm(QWebFrame * frame, const QString & msg);
	virtual void javaScriptConsoleMessage(const QString & message, int lineNumber, const QString & sourceID);
	virtual bool javaScriptPrompt(QWebFrame * frame, const QString & msg, const QString & defaultValue, QString * result);
	virtual bool acceptNavigationRequest(QWebFrame * frame, const QNetworkRequest & request, NavigationType type);

	QString output;
};


#endif