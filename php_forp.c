/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2011 The PHP Group                                |
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"

#include "forp_log.h"

#include "forp.h"
#include "php_forp.h"

#include "ext/standard/info.h"
#include "zend_exceptions.h"

/* {{{ forp_functions[]
 *
 * Every user visible function must have an entry in forp_functions[].
 */
const zend_function_entry forp_functions[] = {
    PHP_FE(forp_start, NULL)
    PHP_FE(forp_end, NULL)
    PHP_FE(forp_dump, NULL)
    PHP_FE(forp_print, NULL)
    PHP_FE(forp_info, NULL)
    PHP_FE(forp_enable, NULL)
    PHP_FE(forp_inspect, NULL)
    {NULL,NULL,NULL} /*PHP_FE_END*/
};
/* }}} */

/* {{{ forp_module_entry
 */
zend_module_entry forp_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "forp",
    forp_functions,
    PHP_MINIT(forp), // Main init
    PHP_MSHUTDOWN(forp), // Main shutdown
    PHP_RINIT(forp), // Request init
    PHP_RSHUTDOWN(forp), // Request shutdown
    PHP_MINFO(forp),
#if ZEND_MODULE_API_NO >= 20010901
    FORP_VERSION,
#endif
#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 2) || PHP_MAJOR_VERSION >= 6
    NO_MODULE_GLOBALS,
#endif
    /*PHP_GINIT(forp), PHP_GSHUTDOWN(forp),*/
    ZEND_MODULE_POST_ZEND_DEACTIVATE_N(forp),
    STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_FORP
ZEND_GET_MODULE(forp)
#endif

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("forp.max_nesting_level", "50", PHP_INI_ALL, OnUpdateLong, max_nesting_level, zend_forp_globals, forp_globals)
    STD_PHP_INI_BOOLEAN("forp.no_internals", "0", PHP_INI_ALL, OnUpdateBool, no_internals, zend_forp_globals, forp_globals)
    STD_PHP_INI_ENTRY("forp.inspect_depth_object", "2", PHP_INI_ALL, OnUpdateLong, inspect_depth_object, zend_forp_globals, forp_globals)
    STD_PHP_INI_ENTRY("forp.inspect_depth_array", "2", PHP_INI_ALL, OnUpdateLong, inspect_depth_object, zend_forp_globals, forp_globals)
PHP_INI_END()
/* }}} */

/* {{{ PHP_GINIT_FUNCTION
 */
static void php_forp_init_globals(zend_forp_globals *forp_globals)
{
    forp_globals->started = 0;
    forp_globals->flags = FORP_FLAG_TIME | FORP_FLAG_MEMORY | FORP_FLAG_ANNOTATIONS | FORP_FLAG_CPU;
    forp_globals->max_nesting_level = 50;
    forp_globals->no_internals = 0;
    forp_globals->stack_len = 0;
    forp_globals->nesting_level = 0;
    forp_globals->dump = NULL;
    forp_globals->stack = NULL;
    forp_globals->main = NULL;
    forp_globals->current_node = NULL;
    forp_globals->utime = 0;
    forp_globals->stime = 0;
    forp_globals->inspect = NULL;
    forp_globals->inspect_len = 0;
    forp_globals->inspect_depth_array = 2;
    forp_globals->inspect_depth_object = 2;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(forp) {
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(forp) {
    forp_info(TSRMLS_C);
    DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ forp_info
 */
ZEND_FUNCTION(forp_info) {
    php_info_print_style(TSRMLS_C);
    forp_info(TSRMLS_C);
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(forp) {

    ZEND_INIT_MODULE_GLOBALS(forp, php_forp_init_globals, NULL);

    REGISTER_INI_ENTRIES();

    REGISTER_LONG_CONSTANT("FORP_FLAG_MEMORY", FORP_FLAG_MEMORY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FORP_FLAG_TIME", FORP_FLAG_TIME, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FORP_FLAG_ANNOTATIONS", FORP_FLAG_ANNOTATIONS, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FORP_FLAG_CPU", FORP_FLAG_CPU, CONST_CS | CONST_PERSISTENT);

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(forp) {

    FORP_G(started) = 0;
    FORP_G(flags) = FORP_FLAG_TIME | FORP_FLAG_MEMORY | FORP_FLAG_ANNOTATIONS | FORP_FLAG_CPU;
    FORP_G(stack_len) = 0;
    FORP_G(nesting_level) = 0;
    FORP_G(dump) = NULL;
    FORP_G(stack) = NULL;
    FORP_G(main) = NULL;
    FORP_G(current_node) = NULL;
    FORP_G(utime) = 0;
    FORP_G(stime) = 0;
    FORP_G(inspect) = NULL;
    FORP_G(inspect_len) = 0;

    // globals
    //FORP_G(max_nesting_level) = 50;
    //FORP_G(no_internals) = 0;
    //FORP_G(inspect_depth_array) = 2;
    //FORP_G(inspect_depth_object) = 2;

    return SUCCESS;
}

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(forp) {

    if(FORP_G(started)) {
        // Restores zend api methods
        if (old_execute) {
            zend_execute = old_execute;
        }
        if (!FORP_G(no_internals)) {
            zend_execute_internal = old_execute_internal;
        }
    }

    return SUCCESS;
}
/* }}} */

/* {{{ ZEND_MODULE_POST_ZEND_DEACTIVATE_D
 */
ZEND_MODULE_POST_ZEND_DEACTIVATE_D(forp) {
    int i, j;

    TSRMLS_FETCH();

    // TODO track not terminated node

    FORP_G(started) = 0;
    FORP_G(nesting_level) = 0;
    FORP_G(current_node) = NULL;
    FORP_G(utime) = 0;
    FORP_G(stime) = 0;

    // stack dtor
    if (FORP_G(stack) != NULL) {
        for (i = 0; i < FORP_G(stack_len); ++i) {
            if(FORP_G(stack)[i]->function.groups) {
                free(FORP_G(stack)[i]->function.groups);
            }
            free(FORP_G(stack)[i]);
        }
        if (i) free(FORP_G(stack));
    }
    FORP_G(stack_len) = 0;
    FORP_G(stack) = NULL;

    // dump dtor
    if (FORP_G(dump) != NULL) zval_ptr_dtor(&FORP_G(dump));
    FORP_G(dump) = NULL;

    // inspect
    if (FORP_G(inspect) != NULL) free(FORP_G(inspect));
    FORP_G(inspect) = NULL;

    return SUCCESS;
}
/* }}} */

/* {{{ forp_enable
 */
ZEND_FUNCTION(forp_enable) {

    long opt = -1;
    php_error_docref(
                NULL TSRMLS_CC,
                E_USER_DEPRECATED,
                "forp_enable() is deprecated, use forp_start()."
                );

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &opt) == FAILURE) {
        return;
    }
    if(opt >= 0) FORP_G(flags) = opt;
    forp_start(TSRMLS_C);
}
/* }}} */

/* {{{ forp_start
 */
ZEND_FUNCTION(forp_start) {
    long opt = -1;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &opt) == FAILURE) {
        return;
    }
    if(opt >= 0) FORP_G(flags) = opt;

    forp_start(TSRMLS_C);
}
/* }}} */

/* {{{ forp_end
 */
ZEND_FUNCTION(forp_end) {
    forp_end(TSRMLS_C);
}
/* }}} */

/* {{{ forp_dump
 */
ZEND_FUNCTION(forp_dump) {
    if (FORP_G(started)) {
        forp_end(TSRMLS_C);
    }
    if (!FORP_G(dump)) {
        forp_stack_dump(TSRMLS_C);
    }
    RETURN_ZVAL(FORP_G(dump), 1, 0);
}
/* }}} */

/* {{{ forp_print
 */
ZEND_FUNCTION(forp_print) {
    if (FORP_G(started)) {
        forp_end(TSRMLS_C);
    }
    forp_stack_dump_cli(TSRMLS_C);
}
/* }}} */

/* {{{ forp_inspect
 */
ZEND_FUNCTION(forp_inspect) {
    char *name;
    int name_len;
    zval *expr;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &name, &name_len, &expr) == FAILURE) {
        return;
    }

    forp_inspect_zval(name, expr TSRMLS_CC);

    RETURN_TRUE;
}
/* }}} */