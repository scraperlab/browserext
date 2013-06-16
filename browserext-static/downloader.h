#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QtNetwork>
#ifdef QT_OPENSSL
#include <QtNetwork/QSslError>
#endif


class Downloader : public QObject
{
	Q_OBJECT

Q_SIGNALS:
	void finished(int result);

public:
	Downloader(const QNetworkRequest & request, const QString & dest, QObject *parent = 0);
	Downloader(const QString & link, const QString & dest, QObject *parent = 0);
	Downloader(QNetworkReply *reply, const QString & dest, QObject *parent = 0);
	~Downloader();
	int download(bool wait = true);
	void setNetworkAccessManager(QNetworkAccessManager *manager);
	QNetworkAccessManager* networkAccessManager();
	QString errorString();

protected Q_SLOTS:
	void handleDownload();
	void downloadedFileWrite();
	void slotError(QNetworkReply::NetworkError error);
#ifdef QT_OPENSSL
	void slotSslErrors(QList<QSslError> errors);
#endif

protected:
	QNetworkAccessManager *network;
	QNetworkReply *reply;
	QFile *dlFile;
	int downloadError;
	QNetworkRequest request;
	QString dest;
	QString errorStr;
	bool isStarted;
};

#endif