#include "phpbrowser.h"

GuiThread::GuiThread(QWaitCondition *wc)
{
    this->wc = wc;
	init = NULL;
}

void GuiThread::run()
{
    init = new Initializer();
    if (wc)
        wc->wakeAll();

	init->getApp()->exec();

    delete init;
}

Initializer* GuiThread::getInitObj()
{
	return init;
}
