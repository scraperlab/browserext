#include <iostream>
#include <QtGui>
#include "phpbrowser.h"

QApplication *pApp = NULL;

Initializer::Initializer()
{
    static int argc = 1;
    static char *argv[1] = {"phpbrowser"};
    pApp = new QApplication(argc, argv);
    pApp->setQuitOnLastWindowClosed(false);

    qRegisterMetaType<PhpBrowser*>("PhpBrowser*");
    qRegisterMetaType<char*>("char*");
    qRegisterMetaType<const char*>("const char*");
    qRegisterMetaType<char**>("char**");
	qRegisterMetaType<QStringList>("QStringList");
	qRegisterMetaType<WebElementTS*>("WebElementTS*");
    qRegisterMetaType<QList<WebElementTS*> >("QList<WebElementTS*>");
	qRegisterMetaType<QWebElement>("QWebElement");
	
    isMainLoopRunnig = false;
    //QTimer::singleShot(0, this, SLOT(createApp()));
}

Initializer::~Initializer()
{
    if (pApp)
    {
        pApp->quit();
        //if (mainwindow) delete mainwindow;
        delete pApp;
    }
}

void Initializer::createApp()
{
    std::cerr << "timer fired" << std::endl;
    isMainLoopRunnig = true;
    pApp->exec();
}

QApplication* Initializer::getApp()
{
    return pApp;
}

void Initializer::setActiveBrowser(PhpBrowser *browser)
{
    if (pApp)
        pApp->setActiveWindow(browser);
}

QThread* Initializer::getAppThread()
{
    return pApp->instance()->thread();
}

PhpBrowser* Initializer::createPhpBrowser()
{
    PhpBrowser *ret = new PhpBrowser();
    pApp->setActiveWindow(ret);
    ret->show();
    return ret;
}

bool Initializer::is_main_loop_runnig()
{
    return isMainLoopRunnig;
}

void Initializer::deleteWebElement(QWebElement *elem)
{
	delete elem;
}
