#include "proxycheckthread.h"

ProxyCheckThread::ProxyCheckThread(const QNetworkProxy & proxy, int index, const QString & site, const QString & findstr)
{
	this->proxy = proxy;
	result = false;
	this->index = index;
	this->site = site;
	this->findstr = findstr;
}


void ProxyCheckThread::run()
{
	QNetworkAccessManager *manager = new QNetworkAccessManager();
	manager->setProxy(proxy);
	QNetworkRequest request = QNetworkRequest(QUrl(site));
	request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.0; WOW64) AppleWebKit/534.27+ (KHTML, like Gecko) Version/5.0.4 Safari/533.20.27");
	QNetworkReply *reply = manager->get(request);

	QEventLoop loop;
	connect(manager, SIGNAL(finished(QNetworkReply *)), &loop, SLOT(quit()));
	loop.exec();
	QByteArray data = reply->readAll();
	result = data.contains(findstr.toUtf8());
	delete reply;
	delete manager;
}


bool ProxyCheckThread::getResult()
{
	return result;
}


bool ProxyCheckThread::getIndex()
{
	return index;
}


QNetworkProxy ProxyCheckThread::getProxy()
{
	return proxy;
}



ProxyChecker::ProxyChecker(QList<QNetworkProxy> & list, int threadsNumber, const QString & site, const QString & findstr)
{
	ProxyList = list;
	this->threadsNumber = threadsNumber;
	threadsFinishedNumber = 0;
	ThreadsList = new QVector<ProxyCheckThread*>(threadsNumber);
  	this->site = site;
	this->findstr = findstr;
}


ProxyChecker::~ProxyChecker()
{
	delete ThreadsList;
}


QList<QNetworkProxy> ProxyChecker::check()
{
	threadsFinishedNumber = 0;
	CheckedList = ProxyList;

	int i;
	for (i=0, ProxyListIndex=0; i<threadsNumber && ProxyListIndex<ProxyList.count(); i++, ProxyListIndex++)
	{
		ProxyCheckThread *thread = new ProxyCheckThread(ProxyList.at(ProxyListIndex), i, site, findstr);
		connect(thread, SIGNAL(finished()), this, SLOT(threadFinished()));
		(*ThreadsList)[i] = thread;
		//thread->start();
	}

	for (int j=0; j<ProxyListIndex; j++)
		(*ThreadsList)[j]->start();

	QEventLoop loop;
	while (threadsFinishedNumber < ProxyList.count())
		loop.processEvents();

	return CheckedList;
}

void ProxyChecker::threadFinished()
{
	ProxyCheckThread *thread = dynamic_cast<ProxyCheckThread *>(sender());
	if (!thread->getResult())
		CheckedList.removeAll(thread->getProxy());

	int index = thread->getIndex();
	thread->deleteLater();

	if (ProxyListIndex < ProxyList.count())
	{
		ProxyCheckThread *thread = new ProxyCheckThread(ProxyList.at(ProxyListIndex), index, site, findstr);
		connect(thread, SIGNAL(finished()), this, SLOT(threadFinished()));
		(*ThreadsList)[index] = thread;
		thread->start();
	}
	threadsFinishedNumber++;
}