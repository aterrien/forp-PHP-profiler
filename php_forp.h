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

#define FORP_VERSION    		"0.0.1"
#define FORP_SKIP			"|forp_dump|forp_print|"
#define FORP_DUMP_ASSOC_FILE		"file"
#define FORP_DUMP_ASSOC_CLASS		"class"
#define FORP_DUMP_ASSOC_FUNCTION	"function"
#define FORP_DUMP_ASSOC_LINENO          "lineno"
#define FORP_DUMP_ASSOC_CPU		"usec"
#define FORP_DUMP_ASSOC_MEMORY		"bytes"
#define FORP_DUMP_ASSOC_LEVEL		"level"
#define FORP_DUMP_ASSOC_PARENT		"parent"
#define FORP_CPU	        	1
#define FORP_MEMORY		        2

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

typedef struct _forp_function {
    char *filename;
    char *class;
    char *function;
    int lineno;
    int type;
} forp_function;

typedef struct _forp_node {
    int key;
    forp_function function;
    int level;
    char *include_filename;
    struct _forp_node *parent;

    // Memory
    signed long mem;
    signed long mem_begin;
    signed long mem_end;

    // CPU
    double time;
    double time_begin;
    double time_end;
} forp_node;


#endif	/* PHP_FORP_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
