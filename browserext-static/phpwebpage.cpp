#include "phpbrowser.h"


PhpWebPage::PhpWebPage(QObject *parent)
	: QWebPage(parent)
{
	output = "";
	useragent = "";
	connect(networkAccessManager(), SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(handleSslErrors(QNetworkReply*, const QList<QSslError> & )));
}


void PhpWebPage::javaScriptAlert(QWebFrame * frame, const QString & msg)
{
	output += "[Alert] "+frame->title()+", "+msg+"\n";
}


bool PhpWebPage::javaScriptConfirm(QWebFrame * frame, const QString & msg)
{
	output += "[Confirm] "+frame->title()+", "+msg+", YES\n";
	return false;
}


void PhpWebPage::javaScriptConsoleMessage(const QString & message, int lineNumber, const QString & sourceID)
{
	output += "[Console] "+sourceID+":"+lineNumber+", "+message+"\n";
}


bool PhpWebPage::javaScriptPrompt(QWebFrame * frame, const QString & msg, const QString & defaultValue, QString * result)
{
	output += "[Prompt] "+frame->title()+", "+msg+"\n";
	return false;
}


QString PhpWebPage::console()
{
	return output;
}


void PhpWebPage::clearConsole()
{
	output = "";
}


void PhpWebPage::addToConsole(const QString & str)
{
	output += str;
}


bool PhpWebPage::acceptNavigationRequest(QWebFrame * frame, const QNetworkRequest & request, NavigationType type)
{
	if (!frame)
		dynamic_cast<PhpWebView*>(view())->isNewViewBegin = true;
	return true;
}


void PhpWebPage::setLastScroll(QPoint & point)
{
	last_scroll = point;
}


QPoint PhpWebPage::lastScroll()
{
	return last_scroll;
}


QString PhpWebPage::chooseFile(QWebFrame *parentFrame, const QString & suggestedFile)
{
	return filename;
}


void PhpWebPage::setFilename(const QString & filename)
{
	this->filename = filename;
}


void PhpWebPage::handleSslErrors(QNetworkReply* reply, const QList<QSslError> &errors)
{
    qDebug() << "handleSslErrors: ";
    foreach (QSslError e, errors)
    {
        qDebug() << "ssl error: " << e;
    }

    reply->ignoreSslErrors();
}


QString PhpWebPage::userAgentForUrl(const QUrl & url) const
{
	if (useragent == "") return QWebPage::userAgentForUrl(url);
	return useragent;
}


void PhpWebPage::setUserAgent(const QString & ua)
{
	useragent = ua;
}