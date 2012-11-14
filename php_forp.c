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
#include "forp.h"
#include "php_forp.h"

#include "ext/standard/info.h"
#include "zend_exceptions.h"

/* {{{ forp_functions[]
 *
 * Every user visible function must have an entry in forp_functions[].
 */
const zend_function_entry forp_functions[] = {
    PHP_FE(forp_enable, NULL)
    PHP_FE(forp_dump, NULL)
    PHP_FE(forp_print, NULL)
    PHP_FE(forp_info, NULL)
    PHP_FE_END /* Must be the last line in forp_functions[] */
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
    NULL, // Request init
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
    STD_PHP_INI_ENTRY("forp.max_nesting_level", "20", PHP_INI_ALL, OnUpdateLong, max_nesting_level, zend_forp_globals, forp_globals)
    STD_PHP_INI_BOOLEAN("forp.no_internals", "0", PHP_INI_ALL, OnUpdateBool, no_internals, zend_forp_globals, forp_globals)
PHP_INI_END()
/* }}} */

/* {{{ PHP_GINIT_FUNCTION
 */
static void php_forp_init_globals(zend_forp_globals *forp_globals)
{
    forp_globals->enabled = 0;
    forp_globals->flags = FORP_FLAG_CPU | FORP_FLAG_MEMORY | FORP_FLAG_ANNOTATIONS;
    forp_globals->max_nesting_level = 20;
    forp_globals->no_internals = 0;
    forp_globals->stack_len = 0;
    forp_globals->nesting_level = 0;
    forp_globals->dump = NULL;
    forp_globals->stack = NULL;
    forp_globals->main = NULL;
    forp_globals->current_node = NULL;
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
    forp_info();
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

    //REGISTER_LONG_CONSTANT("FORP_FLAG_MINIMALISTIC", FORP_FLAG_MINIMALISTIC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FORP_FLAG_MEMORY", FORP_FLAG_MEMORY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FORP_FLAG_CPU", FORP_FLAG_CPU, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FORP_FLAG_ANNOTATIONS", FORP_FLAG_ANNOTATIONS, CONST_CS | CONST_PERSISTENT);

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(forp) {

    if(FORP_G(enabled)) {
        // Restores zend api methods
        if (old_execute) {
            zend_execute = old_execute;
        }
        if (!FORP_G(no_internals)) {
            zend_compile_file = old_compile_file;
            zend_execute_internal = old_execute_internal;
        }
    }
    return SUCCESS;
}
/* }}} */

/* {{{ ZEND_MODULE_POST_ZEND_DEACTIVATE_D
 */
ZEND_MODULE_POST_ZEND_DEACTIVATE_D(forp) {
    TSRMLS_FETCH();

    if(FORP_G(enabled)) {
        // TODO track not terminated node

        FORP_G(nesting_level) = 0;
        FORP_G(current_node) = NULL;

        // destruct the stack
        if (FORP_G(stack)) {
            int i;
            for (i = 0; i < FORP_G(stack_len); ++i) {
                if(FORP_G(stack)[i]->function.groups) {
                    efree(FORP_G(stack)[i]->function.groups);
                }
                efree(FORP_G(stack)[i]);
            }
            if (i) efree(FORP_G(stack));
        }
        FORP_G(stack_len) = 0;
        FORP_G(stack) = NULL;

        // destruct the dump
        if (FORP_G(dump)) zval_ptr_dtor(&FORP_G(dump));
        FORP_G(dump) = NULL;
    }
    return SUCCESS;
}
/* }}} */

/* {{{ forp_enable
 */
ZEND_FUNCTION(forp_enable) {

    long opt = -1;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &opt) == FAILURE) {
        return;
    }
    if(opt >= 0) FORP_G(flags) = opt;

    FORP_G(enabled) = 1;

    // Proxying zend api methods
    old_execute = zend_execute;
    zend_execute = forp_execute;

    if (!FORP_G(no_internals)) {
        old_compile_file = zend_compile_file;
        zend_compile_file = forp_compile_file;

        old_execute_internal = zend_execute_internal;
        zend_execute_internal = forp_execute_internal;
    }

    FORP_G(main) = forp_begin(NULL, NULL TSRMLS_CC);
}
/* }}} */

/* {{{ forp_dump
 */
ZEND_FUNCTION(forp_dump) {

    if (FORP_G(enabled)) {
        if (!FORP_G(dump)) {
            forp_end(FORP_G(main) TSRMLS_CC);
            forp_stack_dump(TSRMLS_C);
        }
    } else {
        php_error_docref(
                NULL TSRMLS_CC,
                E_NOTICE,
                "forp_dump() has no effect when forp_enable is turned off."
                );
    }

    RETURN_ZVAL(FORP_G(dump), 1, 0);
}
/* }}} */

/* {{{ forp_print
 */
ZEND_FUNCTION(forp_print) {
    if (FORP_G(enabled)) {
        forp_end(FORP_G(main) TSRMLS_CC);
        forp_stack_dump_cli(TSRMLS_C);
    } else {
        php_error_docref(
                NULL TSRMLS_CC,
                E_NOTICE,
                "forp_print() has no effect when forp_enable is turned off."
                );
    }
}
/* }}} */