#include "phpbrowser.h"

#ifdef Q_OS_WIN
#include <process.h>
#endif

Initializer *init = NULL;

#ifdef Q_OS_WIN
unsigned __stdcall guithread(void *arg)
#elif defined(Q_OS_LINUX)
void *guithread(void *arg)
#endif
{
    init = new Initializer();
    QWaitCondition *wc = static_cast<QWaitCondition*>(arg);
    wc->wakeAll();

    init->getApp()->exec();

    delete init;
#ifdef Q_OS_WIN
	_endthreadex(0);
#endif
    return NULL;
}

Initializer *getInitializer()
{
    return init;
}
