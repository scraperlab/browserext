#include "phpbrowser.h"
#include "browserext.h"

#ifndef PHP_WIN32
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#else
#include <WinBase.h>
#include <process.h>
#endif


#ifdef PHP_WIN32
HANDLE hThread;
#else
pthread_t thread;
#endif
QMutex mutex2;
bool isGuiThreadCreated = false;


static zend_object_handlers phpbrowser_object_handlers;
static zend_object_handlers phpwebelement_object_handlers;

zend_class_entry *phpbrowser_ce;
zend_class_entry *phpwebelement_ce;

//static Initializer *initobj = NULL;
//static GuiThread *guith = NULL;

struct phpbrowser_object {
    zend_object std;
    PhpBrowser *browser;
};

struct phpwebelement_object {
    zend_object std;
    WebElementTS *webelement;
    zend_object_handle browser_handle;
};



phpwebelement_object* phpwebelement_object_new(zend_class_entry *type TSRMLS_DC);
zend_object_value phpwebelement_register_object(phpwebelement_object* obj TSRMLS_DC);
void phpwebelement_free_storage(void *object TSRMLS_DC);
zend_object_value phpwebelement_create_handler(zend_class_entry *type TSRMLS_DC);
int phpwebelement_compare(zval *object1, zval *object2 TSRMLS_DC);


char *get_script_dir(TSRMLS_D)
{
    const char *filename = zend_get_executed_filename(TSRMLS_C);
	const size_t filename_len = strlen(filename);
	char *dirname = NULL;

	if (!filename) {
		filename = "";
	}

	dirname = estrndup(filename, filename_len);
	zend_dirname(dirname, filename_len);

	if (strcmp(dirname, ".") == 0) {
		dirname = (char *)erealloc(dirname, MAXPATHLEN);
#if HAVE_GETCWD
		VCWD_GETCWD(dirname, MAXPATHLEN);
#elif HAVE_GETWD
		VCWD_GETWD(dirname);
#endif
	}

    return dirname;
}


void phpbrowser_free_storage(void *object TSRMLS_DC)
{
    //php_error(E_WARNING, "phpbrowser free storage");
    phpbrowser_object *obj = (phpbrowser_object *)object;

    QMetaObject::invokeMethod(obj->browser, "deleteLater", Qt::QueuedConnection);

#if PHP_VERSION_ID < 50399
    zend_hash_destroy(obj->std.properties);
    FREE_HASHTABLE(obj->std.properties);
#else
    zend_object_std_dtor(&(obj->std) TSRMLS_CC);
#endif

    efree(obj);
}

zend_object_value phpbrowser_create_handler(zend_class_entry *type TSRMLS_DC)
{
    zval *tmp;
    zend_object_value retval;

    phpbrowser_object *obj = (phpbrowser_object *)emalloc(sizeof(phpbrowser_object));
    memset(obj, 0, sizeof(phpbrowser_object));

#if PHP_VERSION_ID < 50399
    obj->std.ce = type;
    ALLOC_HASHTABLE(obj->std.properties);
    zend_hash_init(obj->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
    zend_hash_copy(obj->std.properties, &type->default_properties,
        (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));
#else
    zend_object_std_init(&(obj->std), type TSRMLS_CC);
    object_properties_init(&(obj->std), type);
#endif
    
    retval.handle = zend_objects_store_put(obj, NULL,
        (zend_objects_free_object_storage_t) phpbrowser_free_storage,
        NULL TSRMLS_CC);
    retval.handlers = &phpbrowser_object_handlers;

    return retval;
}


#if PHP_VERSION_ID < 50399
void phpbrowser_write_property(zval *object, zval *member, zval *value TSRMLS_DC)
#else
void phpbrowser_write_property(zval *object, zval *member, zval *value, const struct _zend_literal *key TSRMLS_DC)
#endif
{
    PhpBrowser *browser;
    char *propname = Z_STRVAL_P(member);

    if (Z_TYPE_P(value) == IS_STRING && !strcmp(propname, "downloadDirectory"))
    {
        phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(object TSRMLS_CC);
        browser = obj->browser;

        char *prop = Z_STRVAL_P(value);
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "setDownloadDir", ct, Q_ARG(const char *, prop));
    }
    
#if PHP_VERSION_ID < 50399
    std_object_handlers.write_property(object, member, value TSRMLS_CC);
#else
    zend_std_write_property(object, member, value, key TSRMLS_CC);
#endif
}


PHP_METHOD(PhpBrowser, __construct)
{
    PhpBrowser *browser = NULL;
    zval *object = getThis();
    char *dir;
    char *fulldir;

/*#ifndef PHP_WIN32
    int pid = (int)getpid();
#else
    int pid = (int)GetCurrentProcessId();
#endif
    php_error(E_WARNING, "phpbrowser __construct start. pid is %d", pid);
*/

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(object TSRMLS_CC);
    
#ifndef PHP_WIN32
    mutex2.lock();
    if (!isGuiThreadCreated)
    {
		QWaitCondition waiter;
		pthread_create(&thread, NULL, guithread, (void *)&waiter);
		isGuiThreadCreated = true;
	
		waiter.wait(&mutex2);
    }
    mutex2.unlock();
#endif

    Initializer *initobj = getInitializer();
    Qt::ConnectionType ct = Qt::BlockingQueuedConnection;

    QMetaObject::invokeMethod(initobj, "createPhpBrowser", ct, Q_RETURN_ARG(PhpBrowser*, browser));
    obj->browser = browser;

    dir = get_script_dir(TSRMLS_C);
    fulldir = (char *) emalloc(strlen(dir)+strlen("download")+10);
    strcpy(fulldir, dir);
    strcat(fulldir, "/");
    strcat(fulldir, "download");
    zend_update_property_string(phpbrowser_ce, object, "downloadDirectory", strlen("downloadDirectory"), fulldir TSRMLS_CC);
    efree(fulldir);
	efree(dir);

    //php_error(E_WARNING, "phpbrowser __construct end initobj=%p", initobj);
}


PHP_METHOD(PhpBrowser, load)
{
    PhpBrowser *browser;
    char *url;
    int url_len;
    int res;
    zend_bool zsamewnd = 0;
	long timeout = 0;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|bl",
                        &url, &url_len, &zsamewnd, &timeout) == FAILURE) {
        RETURN_LONG(0);
    }

    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        bool samewnd = zsamewnd != 0;
		int timeoutint = timeout;
        QMetaObject::invokeMethod(browser, "load", ct, Q_RETURN_ARG(int, res), Q_ARG(const char*, url), Q_ARG(bool, samewnd), Q_ARG(int, timeoutint));
        if (res != 0) {
            RETURN_TRUE;
        }
        else {
            RETURN_FALSE;
        }
    }
    else {
        RETURN_FALSE;
    }
}

PHP_METHOD(PhpBrowser, title)
{
    PhpBrowser *browser;
    char *ret;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "gettitle", ct, Q_ARG(char**, &ret));
        RETVAL_STRING(ret, 1);
        delete ret;
    }
    else {
        RETURN_NULL();
    }
}


PHP_METHOD(PhpBrowser, click)
{
    PhpBrowser *browser;
    char *str;
    int str_len;
    zend_bool zsamewnd = 0;
    bool samewnd;
    int res;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|b",
                        &str, &str_len, &zsamewnd) == FAILURE) {
        RETURN_NULL();
    }
        
    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        samewnd = zsamewnd != 0;
        QMetaObject::invokeMethod(browser, "click", ct, Q_RETURN_ARG(int, res), Q_ARG(const char*, str), Q_ARG(bool, samewnd));
        if (res != 0) {
            RETURN_TRUE;
        }
        else {
            RETURN_FALSE;
        }
    }
    else {
        RETURN_NULL();
    }
}


PHP_METHOD(PhpBrowser, elements)
{
    zval *zobj;
    PhpBrowser *browser;
    QList<WebElementTS*> qlist;
    char *str;
    int str_len;
    zval *zelem;

    zobj = getThis();
    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(zobj TSRMLS_CC);
    browser = obj->browser;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                        &str, &str_len) == FAILURE) {
        RETURN_NULL();
    }
        
    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "getelements", ct, Q_RETURN_ARG(QList<WebElementTS*>, qlist), Q_ARG(const char*, str));

        array_init(return_value);

        for (int i=0; i<qlist.count(); i++)
        {
            phpwebelement_object *newwe = phpwebelement_object_new(phpwebelement_ce TSRMLS_CC);
            newwe->webelement = qlist.at(i);
            newwe->browser_handle = Z_OBJ_HANDLE(*zobj);
            zend_objects_store_add_ref_by_handle(newwe->browser_handle TSRMLS_CC);

            MAKE_STD_ZVAL(zelem);
            zelem->type = IS_OBJECT;
        	zelem->value.obj = phpwebelement_register_object(newwe TSRMLS_CC);
            add_next_index_zval(return_value, zelem);
        }
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpBrowser, download)
{
    PhpBrowser *browser = NULL;
    char *str_link = NULL;
    int str_link_len = 0;
    char *str_dest = NULL;
    int str_dest_len = 0;
    int result = 0;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
                        &str_link, &str_link_len, &str_dest, &str_dest_len) == FAILURE) {
        RETURN_NULL();
    }
        
    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "download", ct, Q_RETURN_ARG(int, result), Q_ARG(const char*, str_link), Q_ARG(const char*, str_dest));
        RETURN_LONG(result);
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpBrowser, back)
{
    PhpBrowser *browser = NULL;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;

    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "back", ct);
        RETURN_TRUE;
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpBrowser, wait)
{
    PhpBrowser *browser = NULL;
    long sec = 0;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &sec) == FAILURE) {
        RETURN_NULL();
    }

    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
		int secint = sec;
        QMetaObject::invokeMethod(browser, "wait", ct, Q_ARG(int, secint));
        RETURN_TRUE;
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpBrowser, scroll)
{
    PhpBrowser *browser = NULL;
    long screen = 0;
    int ret = 0;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &screen) == FAILURE) {
        RETURN_NULL();
    }

    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
		int screenint = screen;
        QMetaObject::invokeMethod(browser, "scroll", ct, Q_RETURN_ARG(int, ret), Q_ARG(int, screenint));
        if (ret != 0) {
            RETURN_TRUE;
        }
        else {
            RETURN_FALSE;
        }
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpBrowser, fill)
{
    PhpBrowser *browser;
    char *str;
    int str_len;
    char *str2;
    int str_len2;
    int res;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
                        &str, &str_len, &str2, &str_len2) == FAILURE) {
        RETURN_NULL();
    }
        
    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "fill", ct, Q_RETURN_ARG(int, res), Q_ARG(const char*, str), Q_ARG(const char*, str2));
        if (res != 0) {
            RETURN_TRUE;
        }
        else {
            RETURN_FALSE;
        }
    }
    else {
        RETURN_NULL();
    }
}


PHP_METHOD(PhpBrowser, fill2)
{
    PhpBrowser *browser;
    char *str;
    int str_len;
    char *str2;
    int str_len2;
    int res;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
                        &str, &str_len, &str2, &str_len2) == FAILURE) {
        RETURN_NULL();
    }
        
    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "fill2", ct, Q_RETURN_ARG(int, res), Q_ARG(const char*, str), Q_ARG(const char*, str2));
        if (res != 0) {
            RETURN_TRUE;
        }
        else {
            RETURN_FALSE;
        }
    }
    else {
        RETURN_NULL();
    }
}


PHP_METHOD(PhpBrowser, check)
{
    PhpBrowser *browser;
    char *str;
    int str_len;
    int res;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                        &str, &str_len) == FAILURE) {
        RETURN_NULL();
    }
        
    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "check", ct, Q_RETURN_ARG(int, res), Q_ARG(const char*, str));
        if (res != 0) {
            RETURN_TRUE;
        }
        else {
            RETURN_FALSE;
        }
    }
    else {
        RETURN_NULL();
    }
}


PHP_METHOD(PhpBrowser, uncheck)
{
    PhpBrowser *browser;
    char *str;
    int str_len;
    int res;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                        &str, &str_len) == FAILURE) {
        RETURN_NULL();
    }
        
    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "uncheck", ct, Q_RETURN_ARG(int, res), Q_ARG(const char*, str));
        if (res != 0) {
            RETURN_TRUE;
        }
        else {
            RETURN_FALSE;
        }
    }
    else {
        RETURN_NULL();
    }
}


PHP_METHOD(PhpBrowser, radio)
{
    PhpBrowser *browser;
    char *str;
    int str_len;
    int res;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                        &str, &str_len) == FAILURE) {
        RETURN_NULL();
    }
        
    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "radio", ct, Q_RETURN_ARG(int, res), Q_ARG(const char*, str));
        if (res != 0) {
            RETURN_TRUE;
        }
        else {
            RETURN_FALSE;
        }
    }
    else {
        RETURN_NULL();
    }
}


PHP_METHOD(PhpBrowser, select)
{
    PhpBrowser *browser;
    char *str;
    int str_len;
    int res;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                        &str, &str_len) == FAILURE) {
        RETURN_NULL();
    }
        
    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "select", ct, Q_RETURN_ARG(int, res), Q_ARG(const char*, str));
        if (res != 0) {
            RETURN_TRUE;
        }
        else {
            RETURN_FALSE;
        }
    }
    else {
        RETURN_NULL();
    }
}


PHP_METHOD(PhpBrowser, selectByText)
{
    PhpBrowser *browser;
    char *str;
    int str_len;
    char *str2;
    int str_len2;
    int res;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
                        &str, &str_len, &str2, &str_len2) == FAILURE) {
        RETURN_NULL();
    }
        
    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "selectbytext", ct, Q_RETURN_ARG(int, res), Q_ARG(const char*, str), Q_ARG(const char*, str2));
        if (res != 0) {
            RETURN_TRUE;
        }
        else {
            RETURN_FALSE;
        }
    }
    else {
        RETURN_NULL();
    }
}


PHP_METHOD(PhpBrowser, selectByValue)
{
    PhpBrowser *browser;
    char *str;
    int str_len;
    char *str2;
    int str_len2;
    int res;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
                        &str, &str_len, &str2, &str_len2) == FAILURE) {
        RETURN_NULL();
    }
        
    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "selectbyvalue", ct, Q_RETURN_ARG(int, res), Q_ARG(const char*, str), Q_ARG(const char*, str2));
        if (res != 0) {
            RETURN_TRUE;
        }
        else {
            RETURN_FALSE;
        }
    }
    else {
        RETURN_NULL();
    }
}


PHP_METHOD(PhpBrowser, fillfile)
{
    PhpBrowser *browser;
    char *str;
    int str_len;
    char *str2;
    int str_len2;
    int res;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
                        &str, &str_len, &str2, &str_len2) == FAILURE) {
        RETURN_NULL();
    }
        
    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "fillfile", ct, Q_RETURN_ARG(int, res), Q_ARG(const char*, str), Q_ARG(const char*, str2));
        if (res != 0) {
            RETURN_TRUE;
        }
        else {
            RETURN_FALSE;
        }
    }
    else {
        RETURN_NULL();
    }
}


PHP_METHOD(PhpBrowser, setProxyList)
{
    PhpBrowser *browser;
    zval *arr;
    zend_bool zcheck = 1;
    bool ischeck;
    int res;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a|b",
                        &arr, &zcheck) == FAILURE) {
        RETURN_NULL();
    }
        
    if (browser != NULL)
    {
        HashTable *arrht = Z_ARRVAL_P(arr);
        QStringList strlist;        

        for(zend_hash_internal_pointer_reset(arrht);
            zend_hash_has_more_elements(arrht) == SUCCESS;
            zend_hash_move_forward(arrht))
        {
            zval **ppzval;

            if (zend_hash_get_current_data(arrht, (void**)&ppzval) == FAILURE) {
                continue;
            }

            char *str = estrdup(Z_STRVAL_PP(ppzval));
            strlist.append(QString(str));
            efree(str);
        }

        zval *zconn = zend_read_property(phpbrowser_ce, getThis(), "proxyCheckThreads", strlen("proxyCheckThreads"), 0 TSRMLS_CC);
        int connNumber = Z_LVAL_P(zconn);
        if (connNumber < 1)  connNumber = 5;
        browser->ProxyCheckThreads = connNumber;

        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        ischeck = zcheck != 0;
        QMetaObject::invokeMethod(browser, "setproxylist2", ct, Q_RETURN_ARG(int, res), Q_ARG(QStringList&, strlist), Q_ARG(bool, ischeck));
		long res2 = res;
        RETURN_LONG(res2);
    }
    else {
        RETURN_NULL();
    }
}


PHP_METHOD(PhpBrowser, proxyList)
{
    zval *zobj;
    PhpBrowser *browser;
    QStringList qlist;

    zobj = getThis();
    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(zobj TSRMLS_CC);
    browser = obj->browser;

    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "proxylist", ct, Q_RETURN_ARG(QStringList, qlist));

        array_init(return_value);

        for (int i=0; i<qlist.count(); i++)
        {
            char *str = strdup_qstring(qlist.at(i));
            add_next_index_string(return_value, str, 1);
            delete str;
        }
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpBrowser, html)
{
    PhpBrowser *browser = NULL;
    char *str;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;

    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "html", ct, Q_RETURN_ARG(char *, str));
        RETVAL_STRING(str, 1);
        delete str;
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpBrowser, show)
{
    PhpBrowser *browser = NULL;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;

    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "show", ct);
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpBrowser, hide)
{
    PhpBrowser *browser = NULL;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;

    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "hide", ct);
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpBrowser, url)
{
    PhpBrowser *browser = NULL;
    char *str;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;

    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "url", ct, Q_RETURN_ARG(char *, str));
        RETVAL_STRING(str, 1);
        delete str;
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpBrowser, requestedUrl)
{
    PhpBrowser *browser = NULL;
    char *str;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;

    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "requestedurl", ct, Q_RETURN_ARG(char *, str));
        RETVAL_STRING(str, 1);
        delete str;
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpBrowser, setHtml)
{
    PhpBrowser *browser;
    char *str;
    int str_len;
    char *str2 = "";
    int str_len2;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s",
                        &str, &str_len, &str2, &str_len2) == FAILURE) {
        RETURN_NULL();
    }
        
    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "setHtml", ct, Q_ARG(const char*, str), Q_ARG(const char*, str2));
    }
    else {
        RETURN_NULL();
    }
}


PHP_METHOD(PhpBrowser, setImageLoading)
{
    PhpBrowser *browser;
    zend_bool zload = 1;
    bool isload;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "b",
                        &zload) == FAILURE) {
        RETURN_NULL();
    }
        
    if (browser != NULL) {
        isload = zload != 0;
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "setImageLoading", ct, Q_ARG(bool, isload));
    }
    else {
        RETURN_NULL();
    }
}



PHP_METHOD(PhpBrowser, gettext)
{
    zval *zobj;
    PhpBrowser *browser;
    QStringList qlist;
	char *str;
    int str_len;
    zval *zelem;

    zobj = getThis();
    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(zobj TSRMLS_CC);
    browser = obj->browser;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                        &str, &str_len) == FAILURE) {
        RETURN_NULL();
    }

    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "gettext", ct, Q_RETURN_ARG(QStringList, qlist), Q_ARG(const char*, str));

        array_init(return_value);

        for (int i=0; i<qlist.count(); i++)
        {
            char *str = strdup_qstring(qlist.at(i));
            add_next_index_string(return_value, str, 1);
            delete str;
        }
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpBrowser, getlink)
{
    zval *zobj;
    PhpBrowser *browser;
    QStringList qlist;
	char *str;
    int str_len;
    zval *zelem;

    zobj = getThis();
    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(zobj TSRMLS_CC);
    browser = obj->browser;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                        &str, &str_len) == FAILURE) {
        RETURN_NULL();
    }

    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "getlink", ct, Q_RETURN_ARG(QStringList, qlist), Q_ARG(const char*, str));

        array_init(return_value);

        for (int i=0; i<qlist.count(); i++)
        {
            char *str = strdup_qstring(qlist.at(i));
            add_next_index_string(return_value, str, 1);
            delete str;
        }
    }
    else {
        RETURN_FALSE;
    }
}



PHP_METHOD(PhpBrowser, getimglink)
{
    zval *zobj;
    PhpBrowser *browser;
    QStringList qlist;
	char *str;
    int str_len;
    zval *zelem;

    zobj = getThis();
    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(zobj TSRMLS_CC);
    browser = obj->browser;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                        &str, &str_len) == FAILURE) {
        RETURN_NULL();
    }

    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "getimglink", ct, Q_RETURN_ARG(QStringList, qlist), Q_ARG(const char*, str));

        array_init(return_value);

        for (int i=0; i<qlist.count(); i++)
        {
            char *str = strdup_qstring(qlist.at(i));
            add_next_index_string(return_value, str, 1);
            delete str;
        }
    }
    else {
        RETURN_FALSE;
    }
}



PHP_METHOD(PhpBrowser, getattr)
{
    zval *zobj;
    PhpBrowser *browser;
    QStringList qlist;
	char *str;
    int str_len;
	char *str2;
    int str_len2;
    zval *zelem;

    zobj = getThis();
    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(zobj TSRMLS_CC);
    browser = obj->browser;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
                        &str, &str_len, &str2, &str_len2) == FAILURE) {
        RETURN_NULL();
    }

    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "getattr", ct, Q_RETURN_ARG(QStringList, qlist), Q_ARG(const char*, str), Q_ARG(const char*, str2));

        array_init(return_value);

        for (int i=0; i<qlist.count(); i++)
        {
            char *str = strdup_qstring(qlist.at(i));
            add_next_index_string(return_value, str, 1);
            delete str;
        }
    }
    else {
        RETURN_FALSE;
    }
}



PHP_METHOD(PhpBrowser, console)
{
    PhpBrowser *browser;
    char *ret;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "console", ct, Q_RETURN_ARG(char*, ret));
        RETVAL_STRING(ret, 1);
        delete ret;
    }
    else {
        RETURN_NULL();
    }
}


PHP_METHOD(PhpBrowser, clearConsole)
{
    PhpBrowser *browser;
    char *ret;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "clearConsole", ct);
		RETURN_TRUE;
    }
    else {
        RETURN_FALSE;
    }
}



PHP_METHOD(PhpBrowser, getCookies)
{
    zval *zobj;
    PhpBrowser *browser;
    QMapParams qmap;

    zobj = getThis();
    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(zobj TSRMLS_CC);
    browser = obj->browser;

    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "getCookies", ct, Q_RETURN_ARG(QMapParams, qmap));

        array_init(return_value);

		QMapParams::iterator i;
		for (i = qmap.begin(); i != qmap.end(); ++i)
		{
			char *strkey = strdup_qstring(i.key());
			char *strval = strdup_qstring(i.value());
			add_assoc_string(return_value, strkey, strval, 1);
            delete strkey;
			delete strval;
        }
    }
    else {
        RETURN_FALSE;
    }
}



PHP_METHOD(PhpBrowser, clearCookies)
{
    PhpBrowser *browser;
    char *ret;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "clearCookies", ct);
		RETURN_TRUE;
    }
    else {
        RETURN_FALSE;
    }
}



PHP_METHOD(PhpBrowser, getCurrentProxy)
{
    PhpBrowser *browser;
    char *ret;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "getCurrentProxy2", ct, Q_RETURN_ARG(char*, ret));
        RETVAL_STRING(ret, 1);
        delete ret;
    }
    else {
        RETURN_NULL();
    }
}


PHP_METHOD(PhpBrowser, setUserAgent)
{
    PhpBrowser *browser;
    char *str;
    int str_len;
    int res;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                        &str, &str_len) == FAILURE) {
        RETURN_NULL();
    }
        
    if (browser != NULL) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "setUserAgent", ct, Q_ARG(const char*, str));
        RETURN_TRUE;
    }
    else {
		RETURN_FALSE;
    }
}



PHP_METHOD(PhpBrowser, setCookiesForUrl)
{
    PhpBrowser *browser;
    zval *arr;
	char *str;
    int str_len;
    bool ischeck;
    int res;

    phpbrowser_object *obj = (phpbrowser_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    browser = obj->browser;
        
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sa",
                        &str, &str_len, &arr) == FAILURE) {
        RETURN_NULL();
    }
        
    if (browser != NULL)
    {
        HashTable *arrht = Z_ARRVAL_P(arr);
        QMapParams qmap;        

        for(zend_hash_internal_pointer_reset(arrht);
            zend_hash_has_more_elements(arrht) == SUCCESS;
            zend_hash_move_forward(arrht))
        {
            zval **ppzval;
			char *key;
			uint klen;
			ulong index;

			if (zend_hash_get_current_key_ex(arrht, &key, &klen, &index, 1, NULL) == HASH_KEY_IS_STRING) 
			{
				if (zend_hash_get_current_data(arrht, (void**)&ppzval) == FAILURE) {
             	   continue;
	            }

				char *val = estrdup(Z_STRVAL_PP(ppzval));
	            qmap[QString(key)] = QString(val);
	            efree(val);
		    }
        }

        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "setCookiesForUrl", ct, Q_ARG(const char *, str), Q_ARG(QMapParams&, qmap));
        RETURN_TRUE;
    }
    else {
        RETURN_FALSE;
    }
}




phpwebelement_object* phpwebelement_object_new(zend_class_entry *type TSRMLS_DC)
{
	phpwebelement_object *obj;
    zval *tmp;

    obj = (phpwebelement_object *)emalloc(sizeof(phpwebelement_object));
    memset(obj, 0, sizeof(phpwebelement_object));

#if PHP_VERSION_ID < 50399
    obj->std.ce = type;
    ALLOC_HASHTABLE(obj->std.properties);
    zend_hash_init(obj->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
    zend_hash_copy(obj->std.properties, &type->default_properties,
        (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));
#else
    zend_object_std_init(&(obj->std), type TSRMLS_CC);
    object_properties_init(&(obj->std), type);
#endif

	return obj;
}


zend_object_value phpwebelement_register_object(phpwebelement_object* obj TSRMLS_DC)
{
    zend_object_value retval;

    retval.handle = zend_objects_store_put(obj, NULL,
        (zend_objects_free_object_storage_t) phpwebelement_free_storage,
        NULL TSRMLS_CC);
    retval.handlers = &phpwebelement_object_handlers;

	return retval;
}


void phpwebelement_free_storage(void *object TSRMLS_DC)
{
    phpwebelement_object *obj = (phpwebelement_object *)object;

    /*QString str;
    if (!obj->webelement->isNull())
        str = obj->webelement->tagName();
    char *strz = strdup_qstring(str);
    php_error(E_WARNING, "phpwebelement free storage %s", strz);
    delete strz;
    */

    zend_objects_store_del_ref_by_handle(obj->browser_handle TSRMLS_CC);

    Qt::ConnectionType ct = Qt::QueuedConnection;
    QMetaObject::invokeMethod(obj->webelement, "deleteLater", ct);
 
#if PHP_VERSION_ID < 50399
    zend_hash_destroy(obj->std.properties);
    FREE_HASHTABLE(obj->std.properties);
#else
    zend_object_std_dtor(&(obj->std) TSRMLS_CC);
#endif

    efree(obj);
}


zend_object_value phpwebelement_create_handler(zend_class_entry *type TSRMLS_DC)
{
    phpwebelement_object* obj = phpwebelement_object_new(type TSRMLS_CC);
    return phpwebelement_register_object(obj TSRMLS_CC);
}


int phpwebelement_compare(zval *object1, zval *object2 TSRMLS_DC)
{
    phpwebelement_object *obj1 = (phpwebelement_object *)zend_object_store_get_object(object1 TSRMLS_CC);
    WebElementTS *p1 = obj1->webelement;
    phpwebelement_object *obj2 = (phpwebelement_object *)zend_object_store_get_object(object2 TSRMLS_CC);
    WebElementTS *p2 = obj2->webelement;
    if (*p1 == *p2)
        return 0;
    else
        return 1;
}


PHP_METHOD(PhpWebElement, attr)
{
    WebElementTS *webelement = NULL;
    char *str = NULL;
    int str_len = 0;
    char *ret;

    phpwebelement_object *obj = (phpwebelement_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    webelement = obj->webelement;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                        &str, &str_len) == FAILURE) {
        RETURN_NULL();
    }
        
    if (webelement != NULL)
    {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(webelement, "attribute", ct, Q_RETURN_ARG(char *, ret), Q_ARG(const char *, str));
        RETVAL_STRING(ret, 1);
        delete ret;
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpWebElement, tagName)
{
    WebElementTS *webelement = NULL;
    char *ret;

    phpwebelement_object *obj = (phpwebelement_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    webelement = obj->webelement;

    if (webelement != NULL)
    {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(webelement, "tagName", ct, Q_RETURN_ARG(char *, ret));
        RETVAL_STRING(ret, 1);
        delete ret;
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpWebElement, prop)
{
    WebElementTS *webelement = NULL;
    char *str = NULL;
    int str_len = 0;
    char *ret;

    phpwebelement_object *obj = (phpwebelement_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    webelement = obj->webelement;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                        &str, &str_len) == FAILURE) {
        RETURN_NULL();
    }
        
    if (webelement != NULL)
    {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(webelement, "prop", ct, Q_RETURN_ARG(char *, ret), Q_ARG(const char *, str));
        RETVAL_STRING(ret, 1);
        delete ret;
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpWebElement, parent)
{
    WebElementTS *webelement = NULL;
    WebElementTS *ret;

    phpwebelement_object *obj = (phpwebelement_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    webelement = obj->webelement;

    if (webelement != NULL)
    {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(webelement, "parent", ct, Q_RETURN_ARG(WebElementTS*, ret));
        phpwebelement_object *newwe = phpwebelement_object_new(phpwebelement_ce TSRMLS_CC);
        newwe->webelement = ret;
        newwe->browser_handle = obj->browser_handle;
        return_value->type = IS_OBJECT;
        return_value->value.obj = phpwebelement_register_object(newwe TSRMLS_CC);
    }
    else {
        RETURN_FALSE;
    }
}

PHP_METHOD(PhpWebElement, textAll)
{
    WebElementTS *webelement = NULL;
    char *ret;

    phpwebelement_object *obj = (phpwebelement_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    webelement = obj->webelement;

    if (webelement != NULL)
    {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(webelement, "textAll", ct, Q_RETURN_ARG(char *, ret));
        RETVAL_STRING(ret, 1);
        delete ret;
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpWebElement, text)
{
    WebElementTS *webelement = NULL;
    char *ret;

    phpwebelement_object *obj = (phpwebelement_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    webelement = obj->webelement;

    if (webelement != NULL)
    {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(webelement, "text", ct, Q_RETURN_ARG(char *, ret));
        RETVAL_STRING(ret, 1);
        delete ret;
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpWebElement, nextSibling)
{
    WebElementTS *webelement = NULL;
    WebElementTS *ret;

    phpwebelement_object *obj = (phpwebelement_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    webelement = obj->webelement;

    if (webelement != NULL)
    {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(webelement, "nextSibling", ct, Q_RETURN_ARG(WebElementTS*, ret));

        phpwebelement_object *newwe = phpwebelement_object_new(phpwebelement_ce TSRMLS_CC);
        newwe->webelement = ret;
        newwe->browser_handle = obj->browser_handle;
        return_value->type = IS_OBJECT;
        return_value->value.obj = phpwebelement_register_object(newwe TSRMLS_CC);
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpWebElement, prevSibling)
{
    WebElementTS *webelement = NULL;
    WebElementTS *ret;

    phpwebelement_object *obj = (phpwebelement_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    webelement = obj->webelement;

    if (webelement != NULL)
    {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(webelement, "prevSibling", ct, Q_RETURN_ARG(WebElementTS*, ret));

        phpwebelement_object *newwe = phpwebelement_object_new(phpwebelement_ce TSRMLS_CC);
        newwe->webelement = ret;
        newwe->browser_handle = obj->browser_handle;
        return_value->type = IS_OBJECT;
        return_value->value.obj = phpwebelement_register_object(newwe TSRMLS_CC);
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpWebElement, firstChild)
{
    WebElementTS *webelement = NULL;
    WebElementTS *ret;

    phpwebelement_object *obj = (phpwebelement_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    webelement = obj->webelement;

    if (webelement != NULL)
    {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(webelement, "firstChild", ct, Q_RETURN_ARG(WebElementTS*, ret));

        phpwebelement_object *newwe = phpwebelement_object_new(phpwebelement_ce TSRMLS_CC);
        newwe->webelement = ret;
        newwe->browser_handle = obj->browser_handle;
        return_value->type = IS_OBJECT;
        return_value->value.obj = phpwebelement_register_object(newwe TSRMLS_CC);
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpWebElement, lastChild)
{
    WebElementTS *webelement = NULL;
    WebElementTS *ret;

    phpwebelement_object *obj = (phpwebelement_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    webelement = obj->webelement;

    if (webelement != NULL)
    {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(webelement, "lastChild", ct, Q_RETURN_ARG(WebElementTS*, ret));

        phpwebelement_object *newwe = phpwebelement_object_new(phpwebelement_ce TSRMLS_CC);
        newwe->webelement = ret;
        newwe->browser_handle = obj->browser_handle;
        return_value->type = IS_OBJECT;
        return_value->value.obj = phpwebelement_register_object(newwe TSRMLS_CC);
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpWebElement, isNull)
{
    WebElementTS *webelement = NULL;
    bool ret;

    phpwebelement_object *obj = (phpwebelement_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    webelement = obj->webelement;

    if (webelement != NULL)
    {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(webelement, "isNull", ct, Q_RETURN_ARG(bool, ret));
        if (ret) {
            RETURN_TRUE;
        }
        else {
            RETURN_FALSE;
        }
    }
    else {
        RETURN_FALSE;
    }
}

PHP_METHOD(PhpWebElement, getXPath)
{
    WebElementTS *webelement = NULL;
    char *ret;

    phpwebelement_object *obj = (phpwebelement_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    webelement = obj->webelement;

    if (webelement) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(webelement, "getXPath", ct, Q_RETURN_ARG(char *, ret));
        RETVAL_STRING(ret, 1);
        delete ret;
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpWebElement, html)
{
    WebElementTS *webelement = NULL;
    char *ret;

    phpwebelement_object *obj = (phpwebelement_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    webelement = obj->webelement;

    if (webelement) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(webelement, "html", ct, Q_RETURN_ARG(char *, ret));
        RETVAL_STRING(ret, 1);
        delete ret;
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpWebElement, elements)
{
    PhpBrowser *browser;
    QList<WebElementTS*> qlist;
    char *str;
    int str_len;
    zval *zelem;
    WebElementTS *webelement = NULL;

    phpwebelement_object *obj = (phpwebelement_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    webelement = obj->webelement;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                        &str, &str_len) == FAILURE) {
        RETURN_NULL();
    }

    if (webelement) {
        phpbrowser_object *obj2 = (phpbrowser_object *) zend_object_store_get_object_by_handle(obj->browser_handle TSRMLS_CC);
        browser = obj2->browser;
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "getelements2", ct, Q_RETURN_ARG(QList<WebElementTS*>, qlist), Q_ARG(const char*, str), Q_ARG(WebElementTS*, webelement));

        array_init(return_value);

        for (int i=0; i<qlist.count(); i++)
        {
            phpwebelement_object *newwe = phpwebelement_object_new(phpwebelement_ce TSRMLS_CC);
            newwe->webelement = qlist.at(i);
            newwe->browser_handle = obj->browser_handle;
            zend_objects_store_add_ref_by_handle(newwe->browser_handle TSRMLS_CC);

            MAKE_STD_ZVAL(zelem);
            zelem->type = IS_OBJECT;
        	zelem->value.obj = phpwebelement_register_object(newwe TSRMLS_CC);
            add_next_index_zval(return_value, zelem);
        }
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpWebElement, click)
{
    PhpBrowser *browser;
    WebElementTS *webelement = NULL;
    zend_bool zsamewnd = 0;
    int res;

    phpwebelement_object *obj = (phpwebelement_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    webelement = obj->webelement;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b",
                        &zsamewnd) == FAILURE) {
        RETURN_NULL();
    }

    if (webelement) {
        phpbrowser_object *obj2 = (phpbrowser_object *) zend_object_store_get_object_by_handle(obj->browser_handle TSRMLS_CC);
        browser = obj2->browser;
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        bool samewnd = zsamewnd != 0;
        QMetaObject::invokeMethod(browser, "click2", ct, Q_RETURN_ARG(int, res), Q_ARG(WebElementTS*, webelement), Q_ARG(bool, samewnd));
        if (res != 0) {
            RETURN_TRUE;
        }
        else {
            RETURN_FALSE;
        }
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpWebElement, attrAll)
{
    QStringList qlist;
    char *str;
    char *key;
    WebElementTS *webelement = NULL;

    phpwebelement_object *obj = (phpwebelement_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    webelement = obj->webelement;

    if (webelement) {
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(webelement, "attributeNames", ct, Q_RETURN_ARG(QStringList, qlist));

        array_init(return_value);

        for (int i=0; i<qlist.count(); i++)
        {
            QString name = qlist.at(i);
            QMetaObject::invokeMethod(webelement, "attribute", ct, Q_RETURN_ARG(char *, str), Q_ARG(const char *, name.toUtf8().constData()));
            key = strdup_qstring(name);
            add_assoc_string(return_value, key, str, 1);
            delete key;
            delete str;
        }
    }
    else {
        RETURN_FALSE;
    }
}


PHP_METHOD(PhpWebElement, jsexec)
{
    PhpBrowser *browser;
    WebElementTS *webelement = NULL;
    char *str;
    int str_len;
    int res;

    phpwebelement_object *obj = (phpwebelement_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    webelement = obj->webelement;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                        &str, &str_len) == FAILURE) {
        RETURN_NULL();
    }

    if (webelement) {
        phpbrowser_object *obj2 = (phpbrowser_object *) zend_object_store_get_object_by_handle(obj->browser_handle TSRMLS_CC);
        browser = obj2->browser;
        Qt::ConnectionType ct = Qt::BlockingQueuedConnection;
        QMetaObject::invokeMethod(browser, "jsexec", ct, Q_RETURN_ARG(int, res), Q_ARG(WebElementTS*, webelement), Q_ARG(const char *, str));
        if (res != 0) {
            RETURN_TRUE;
        }
        else {
            RETURN_FALSE;
        }
    }
    else {
        RETURN_FALSE;
    }
}




ZEND_BEGIN_ARG_INFO_EX(arginfo_phpbrowser_void, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phpbrowser_load, 0, 0, 1)
    ZEND_ARG_INFO(0, url)
	ZEND_ARG_INFO(0, samewnd)
	ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phpbrowser_click, 0, 0, 1)
	ZEND_ARG_INFO(0, xpath)
    ZEND_ARG_INFO(0, samewnd)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phpbrowser_xpath, 0, 0, 1)
	ZEND_ARG_INFO(0, xpath)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phpbrowser_download, 0, 0, 2)
	ZEND_ARG_INFO(0, url)
    ZEND_ARG_INFO(0, destination)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phpbrowser_wait, 0, 0, 1)
	ZEND_ARG_INFO(0, seconds)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phpbrowser_scroll, 0, 0, 1)
	ZEND_ARG_INFO(0, numscreens)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phpbrowser_fill, 0, 0, 2)
	ZEND_ARG_INFO(0, xpath)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phpbrowser_setproxylist, 0, 0, 1)
	ZEND_ARG_ARRAY_INFO(0, proxylist, 0)
	ZEND_ARG_INFO(0, check)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phpbrowser_sethtml, 0, 0, 1)
	ZEND_ARG_INFO(0, html)
    ZEND_ARG_INFO(0, url)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phpbrowser_setimageloading, 0, 0, 1)
	ZEND_ARG_INFO(0, isload)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phpbrowser_setcookiesforurl, 0, 0, 2)
	ZEND_ARG_INFO(0, url)
	ZEND_ARG_ARRAY_INFO(0, cookies, 0)
ZEND_END_ARG_INFO()



const zend_function_entry phpbrowser_methods[] = {
    PHP_ME(PhpBrowser,  __construct,         arginfo_phpbrowser_void, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(PhpBrowser,  load,                arginfo_phpbrowser_load, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  title,               arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  click,               arginfo_phpbrowser_click, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  elements,            arginfo_phpbrowser_xpath, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  download,            arginfo_phpbrowser_download, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  back,                arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  wait,                arginfo_phpbrowser_wait, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  scroll,              arginfo_phpbrowser_scroll, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  fill,                arginfo_phpbrowser_fill, ZEND_ACC_PUBLIC)
	PHP_ME(PhpBrowser,  fill2,               arginfo_phpbrowser_fill, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  check,               arginfo_phpbrowser_xpath, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  uncheck,             arginfo_phpbrowser_xpath, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  radio,               arginfo_phpbrowser_xpath, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  select,              arginfo_phpbrowser_xpath, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  selectByText,        arginfo_phpbrowser_fill, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  selectByValue,       arginfo_phpbrowser_fill, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  fillfile,            arginfo_phpbrowser_fill, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  setProxyList,        arginfo_phpbrowser_setproxylist, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  proxyList,           arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  html,                arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  show,                arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  hide,                arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  url,                 arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  requestedUrl,        arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  setHtml,             arginfo_phpbrowser_sethtml, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  setImageLoading,     arginfo_phpbrowser_setimageloading, ZEND_ACC_PUBLIC)
	PHP_ME(PhpBrowser,  gettext,             arginfo_phpbrowser_xpath, ZEND_ACC_PUBLIC)
	PHP_ME(PhpBrowser,  getlink,             arginfo_phpbrowser_xpath, ZEND_ACC_PUBLIC)
	PHP_ME(PhpBrowser,  getimglink,          arginfo_phpbrowser_xpath, ZEND_ACC_PUBLIC)
	PHP_ME(PhpBrowser,  getattr,		     arginfo_phpbrowser_fill, ZEND_ACC_PUBLIC)
	PHP_ME(PhpBrowser,  console,             arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
	PHP_ME(PhpBrowser,  clearConsole,        arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
	PHP_ME(PhpBrowser,  getCookies,          arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
	PHP_ME(PhpBrowser,  clearCookies,        arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
	PHP_ME(PhpBrowser,  getCurrentProxy,     arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
    PHP_ME(PhpBrowser,  setUserAgent,        arginfo_phpbrowser_xpath, ZEND_ACC_PUBLIC)
	PHP_ME(PhpBrowser,  setCookiesForUrl,    arginfo_phpbrowser_setcookiesforurl, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL, 0, 0}
};


ZEND_BEGIN_ARG_INFO_EX(arginfo_phpwebelement_name, 0, 0, 1)
    ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phpwebelement_click, 0, 0, 0)
    ZEND_ARG_INFO(0, samewnd)
ZEND_END_ARG_INFO()

const zend_function_entry phpwebelement_methods[] = {
    PHP_ME(PhpWebElement,  attr,         arginfo_phpwebelement_name, ZEND_ACC_PUBLIC)
    PHP_ME(PhpWebElement,  attrAll,      arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
    PHP_ME(PhpWebElement,  tagName,      arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)    
    PHP_ME(PhpWebElement,  prop,         arginfo_phpwebelement_name, ZEND_ACC_PUBLIC)    
    PHP_ME(PhpWebElement,  parent,       arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
    PHP_ME(PhpWebElement,  textAll,      arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)    
    PHP_ME(PhpWebElement,  text,         arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)    
    PHP_ME(PhpWebElement,  nextSibling,  arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)    
    PHP_ME(PhpWebElement,  prevSibling,  arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
    PHP_ME(PhpWebElement,  firstChild,   arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
    PHP_ME(PhpWebElement,  lastChild,    arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
    PHP_ME(PhpWebElement,  isNull,       arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
    PHP_ME(PhpWebElement,  getXPath,     arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
    PHP_ME(PhpWebElement,  html,         arginfo_phpbrowser_void, ZEND_ACC_PUBLIC)
    PHP_ME(PhpWebElement,  elements,     arginfo_phpbrowser_xpath, ZEND_ACC_PUBLIC)
    PHP_ME(PhpWebElement,  click,        arginfo_phpwebelement_click, ZEND_ACC_PUBLIC)
    PHP_ME(PhpWebElement,  jsexec,       arginfo_phpwebelement_name, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL, 0, 0}
};


PHP_MINIT_FUNCTION(browserext)
{
    zend_class_entry ce;
    zend_class_entry webelement;

    INIT_CLASS_ENTRY(ce, "PhpBrowser", phpbrowser_methods);
    phpbrowser_ce = zend_register_internal_class(&ce TSRMLS_CC);
    zend_declare_property_long(phpbrowser_ce, "proxyCheckThreads", strlen("proxyCheckThreads"), 5, ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_declare_property_string(phpbrowser_ce, "downloadDirectory", strlen("downloadDirectory"), "", ZEND_ACC_PUBLIC TSRMLS_CC);
    phpbrowser_ce->create_object = phpbrowser_create_handler;
    memcpy(&phpbrowser_object_handlers,
        zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    phpbrowser_object_handlers.clone_obj = NULL;
    phpbrowser_object_handlers.write_property = phpbrowser_write_property;

    INIT_CLASS_ENTRY(webelement, "PhpWebElement", phpwebelement_methods);
    phpwebelement_ce = zend_register_internal_class(&webelement TSRMLS_CC);
    phpwebelement_ce->create_object = phpwebelement_create_handler;
    memcpy(&phpwebelement_object_handlers,
        zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    phpwebelement_object_handlers.clone_obj = NULL;
    phpwebelement_object_handlers.compare_objects = phpwebelement_compare;

#ifdef PHP_WIN32
	mutex2.lock();
	QWaitCondition waiter;
	hThread = (HANDLE)_beginthreadex(NULL, 0, &guithread, (void *)&waiter, 0, NULL);
	waiter.wait(&mutex2);
	mutex2.unlock();
    //guith = new GuiThread();
    //guith->start();
#endif

    return SUCCESS;
}


PHP_MSHUTDOWN_FUNCTION(browserext)
{
#ifdef PHP_WIN32
	qApp->quit();
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
#else
    if (isGuiThreadCreated)
    {
		qApp->quit();
		pthread_join(thread, NULL);
	}
#endif

    return SUCCESS;
}


zend_module_entry browserext_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_BROWSEREXT_EXTNAME,
    NULL,                  /* Functions */
    PHP_MINIT(browserext),
    PHP_MSHUTDOWN(browserext),/* MSHUTDOWN */
    NULL,                  /* RINIT */
    NULL,                  /* RSHUTDOWN */
    NULL,                  /* MINFO */
#if ZEND_MODULE_API_NO >= 20010901
    PHP_BROWSEREXT_EXTVER,
#endif
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_BROWSEREXT
extern "C" {
ZEND_GET_MODULE(browserext)
}

#endif
