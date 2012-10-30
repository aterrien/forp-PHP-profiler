/*
  +----------------------------------------------------------------------+
  | Forp	                                                         |
  +----------------------------------------------------------------------+
  | Copyright (c) 2012 Anthony Terrien                                   |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Anthony Terrien <forp@anthonyterrien.com>                    |
  +----------------------------------------------------------------------+
 */
#ifndef PHP_FORP_H
#define PHP_FORP_H

#define FORP_VERSION                "0.0.1"
#define FORP_SKIP                   "|forp_dump|forp_print|"
#define FORP_DUMP_ASSOC_FILE		"file"
#define FORP_DUMP_ASSOC_CLASS		"class"
#define FORP_DUMP_ASSOC_FUNCTION	"function"
#define FORP_DUMP_ASSOC_LINENO      "lineno"
#define FORP_DUMP_ASSOC_CPU         "usec"
#define FORP_DUMP_ASSOC_MEMORY		"bytes"
#define FORP_DUMP_ASSOC_LEVEL		"level"
#define FORP_DUMP_ASSOC_PARENT		"parent"
#define FORP_DUMP_ASSOC_GROUP       "group"
#define FORP_DUMP_ASSOC_CAPTION     "caption"
#define FORP_FLAG_CPU               0x0001
#define FORP_FLAG_MEMORY            0x0002
#define FORP_FLAG_ANNOTATION        0x0003

extern zend_module_entry forp_module_entry;
#define phpext_forp_ptr &forp_module_entry

#ifdef PHP_WIN32
#define PHP_FORP_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#define PHP_FORP_API __attribute__ ((visibility("default")))
#else
#define PHP_FORP_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(forp);
PHP_MSHUTDOWN_FUNCTION(forp);
PHP_RSHUTDOWN_FUNCTION(forp);
PHP_MINFO_FUNCTION(forp);
ZEND_MODULE_POST_ZEND_DEACTIVATE_D(forp);

PHP_FUNCTION(forp_enable);
PHP_FUNCTION(forp_dump);
PHP_FUNCTION(forp_print);
PHP_FUNCTION(forp_info);

/* global variables */
ZEND_BEGIN_MODULE_GLOBALS(forp)
	int enabled;
    long flags;
	long nesting_level;
	forp_node_t *main;
	forp_node_t *current_node;
	int stack_len;
	forp_node_t **stack;
	zval *dump;
	long max_nesting_level;
	int no_internals;
ZEND_END_MODULE_GLOBALS(forp)

ZEND_DECLARE_MODULE_GLOBALS(forp);

#ifdef ZTS
#define FORP_G(v) TSRMG(forp_globals_id, zend_forp_globals *, v)
#else
#define FORP_G(v) (forp_globals.v)
#endif

#endif	/* PHP_FORP_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
