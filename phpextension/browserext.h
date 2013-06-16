#ifndef PHP_BROWSEREXT_H
#define PHP_BROWSEREXT_H

#define PHP_BROWSEREXT_EXTNAME  "browserext"
#define PHP_BROWSEREXT_EXTVER   "0.1"

#ifdef HAVE_CONFIG_H
#include "config.h"
#ifdef PHP_WIN32
#undef HAVE_CONFIG_H
#endif
#endif

extern "C" {
#include "php.h"
#include "zend_exceptions.h"

#ifdef ZTS
#include "TSRM.h"
#endif
}

extern zend_module_entry browserext_module_entry;
#define phpext_browserext_ptr &browserext_module_entry;

#endif