#include "downloader.h"


Downloader::Downloader(const QNetworkRequest & request, const QString & dest, QObject *parent)
	: QObject(parent)
{
	network = new QNetworkAccessManager(this);
	this->request = request;
	this->dest = dest;
	reply = NULL;
	dlFile = new QFile(dest);
	isStarted = false;
}


Downloader::Downloader(const QString & link, const QString & dest, QObject *parent)
	: QObject(parent)
{
	network = new QNetworkAccessManager(this);
	this->request.setUrl(QUrl(link));
	this->dest = dest;
	reply = NULL;
	dlFile = new QFile(dest);
	isStarted = false;
}


Downloader::Downloader(QNetworkReply *reply, const QString & dest, QObject *parent)
	: QObject(parent)
{
	network = new QNetworkAccessManager(this);
	this->dest = dest;
	this->reply = reply;
	dlFile = new QFile(dest);
	isStarted = false;
}


Downloader::~Downloader()
{
	delete dlFile;
}


int Downloader::download(bool wait)
{
	if (!isStarted)
	{
		isStarted = true;
		downloadError = 0;
		
		if (!dlFile->open(QIODevice::WriteOnly))
		{
			errorStr += "can not open file " + dest + "\n";
			return 1;
		}

		if (reply == NULL)
			reply = network->get(request);
		
		QObject::connect(reply, SIGNAL(readyRead()), this, SLOT(downloadedFileWrite()));
		QObject::connect(reply, SIGNAL(finished ()), this, SLOT(handleDownload()));
		QObject::connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(slotError(QNetworkReply::NetworkError)));
		//QObject::connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(slotSslErrors(QList<QSslError>)));

		if (wait)
		{
			QEventLoop loop;
			QObject::connect(this, SIGNAL(finished(int)), &loop, SLOT(quit()));
			loop.exec();
		}
		return downloadError;
	}
	return 0;
}


void Downloader::setNetworkAccessManager(QNetworkAccessManager *manager)
{
	network = manager;
}


QNetworkAccessManager* Downloader::networkAccessManager()
{
	return network;
}


QString Downloader::errorString()
{
	return errorStr;
}


void Downloader::downloadedFileWrite()
{
    dlFile->write(reply->readAll());
}


void Downloader::handleDownload()
{
    dlFile->close();
	downloadError = 0;
	reply->deleteLater();
	emit finished(downloadError);
}


void Downloader::slotError(QNetworkReply::NetworkError error)
{
	errorStr = reply->errorString();
	downloadError = 2;
}

#ifdef QT_OPENSSL
void PhpWebView::slotSslErrors(QList<QSslError> errors)
{
	for (int i=0; i<errors.count(); i++)
		errorStr += errors[i].errorString() + "\n";
	downloadError = 3;
}
#endif
