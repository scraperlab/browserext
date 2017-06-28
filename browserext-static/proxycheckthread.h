#ifndef PROXYCHECKTHREAD_H
#define PROXYCHECKTHREAD_H

#include <QThread>
#include <QtNetwork>


class ProxyCheckThread : public QThread
{
	Q_OBJECT

public:
	ProxyCheckThread(const QNetworkProxy & proxy, int index, const QString & site = "http://google.com", const QString & findstr = "<title>Google</title>");
	void run();
	bool getResult();
	bool getIndex();
	QNetworkProxy getProxy();
protected:
	QNetworkProxy proxy;
	bool result;
	int index;
	QString site;
	QString findstr;
};


class ProxyChecker : public QObject
{
	Q_OBJECT

public:
	ProxyChecker(QList<QNetworkProxy> & list, int threadsNumber = 5, const QString & site = "http://google.com", const QString & findstr = "<title>Google</title>");
	~ProxyChecker();
	QList<QNetworkProxy> check();

protected Q_SLOTS:
	void threadFinished();

protected:
	QList<QNetworkProxy> ProxyList;
	QList<QNetworkProxy> CheckedList;
	QVector<ProxyCheckThread*> *ThreadsList;
	int threadsNumber;
	int threadsFinishedNumber;
	int ProxyListIndex;
	QString site;
	QString findstr;
};

#endif