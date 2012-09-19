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

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#elif defined(PHP_WIN32)
#include "win32/time.h"
#include <math.h>
static inline double round(double val) {
    return floor(val + 0.5);
}
#endif

#include <stdio.h>
#include <sys/resource.h>

/* {{{ forp_populate_function
 */
static void forp_populate_function(
    forp_function_t *function,
    zend_execute_data *edata,
    zend_op_array *op_array TSRMLS_DC
) {

    // Inits function struct
    function->class = NULL;
    function->function = NULL;
    function->filename = NULL;
    function->lineno = 0;

    if (edata) {

        // Retrieves filename
        if (op_array) {
            function->filename = strdup(op_array->filename);
        } else {
            if (edata->op_array) {
                function->filename = strdup(edata->op_array->filename);
            } else {
                function->filename = "";
            }
        }

        // Retrieves call lineno
        if(edata->opline) {
            function->lineno = edata->opline->lineno;
        }
        // func proto lineno
        // if (edata->function_state.function->common.type!=ZEND_INTERNAL_FUNCTION) {
        //    function->lineno = edata->function_state.function->op_array.opcodes->lineno - 1;
        // }

        // Retrieves class and function names
        if (edata->function_state.function->common.function_name) {
            if (edata->object) {
                if (edata->function_state.function->common.scope) {
                    function->class = strdup(edata->function_state.function->common.scope->name);
                }
            } else if (
                    EG(scope)
                    && edata->function_state.function->common.scope
                    && edata->function_state.function->common.scope->name
                    ) {
                function->class = strdup(edata->function_state.function->common.scope->name);
            }

            function->function = strdup(edata->function_state.function->common.function_name);
        } else {
#if PHP_VERSION_ID >= 50399
            switch (function->type = edata->opline->extended_value) {
#else
            switch (function->type = edata->opline->op2.u.constant.value.lval) {
#endif
                case ZEND_EVAL:
                    function->function = "eval";
                    break;
                case ZEND_INCLUDE:
                    function->function = "include";
                    break;
                case ZEND_REQUIRE:
                    function->function = "require";
                    break;
                case ZEND_INCLUDE_ONCE:
                    function->function = "include_once";
                    break;
                case ZEND_REQUIRE_ONCE:
                    function->function = "require_once";
                    break;
                default:
                    function->function = "unknown";
                    break;
            }
        }
    } else {

        // Root node
        edata = EG(current_execute_data);
        function->class = NULL;
        function->function = "{main}";
        function->filename = edata->op_array->filename;
    }
}
/* }}} */

/* {{{ forp_begin
 */
forp_node_t *forp_begin(zend_execute_data *edata, zend_op_array *op_array TSRMLS_DC) {
    struct timeval tv;
    forp_node_t *pn;
    int key;

    pn = emalloc(sizeof (forp_node_t));
    pn->level = FORP_G(nesting_level)++;
    pn->parent = FORP_G(current_node);

    forp_populate_function(&(pn->function), edata, op_array TSRMLS_CC);

    FORP_G(current_node) = pn;
    key = FORP_G(stack_len);
    pn->key = key;
    FORP_G(stack_len)++;
    FORP_G(stack) = erealloc(
            FORP_G(stack),
            FORP_G(stack_len) * sizeof (forp_node_t)
            );
    FORP_G(stack)[key] = pn;

    gettimeofday(&tv, NULL);
    pn->time_begin = tv.tv_sec * 1000000.0 + tv.tv_usec;

    pn->mem_begin = zend_memory_usage(0 TSRMLS_CC);

    return pn;
}
/* }}} */

/* {{{ forp_info
 */
void forp_info() {
    php_info_print_table_start();
    php_info_print_table_row(2, "Version", FORP_VERSION);
    php_info_print_table_end();
}
/* }}} */


/* {{{ forp_compile_file
 */
zend_op_array *forp_compile_file(zend_file_handle *file_handle, int type TSRMLS_DC) {
    return old_compile_file(file_handle, type TSRMLS_CC);
}
/* }}} */

/* {{{ forp_end
 */
void forp_end(forp_node_t *pn TSRMLS_DC) {
    struct timeval tv;

    // dump memory before next steps
    pn->mem_end = zend_memory_usage(0 TSRMLS_CC);
    pn->mem = pn->mem_end - pn->mem_begin;

    gettimeofday(&tv, NULL);
    pn->time_end = tv.tv_sec * 1000000.0 + tv.tv_usec;
    pn->time = pn->time_end - pn->time_begin;

    FORP_G(current_node) = pn->parent;
    FORP_G(nesting_level)--;
}
/* }}} */

/* {{{ forp_execute
 */
void forp_execute(zend_op_array *op_array TSRMLS_DC) {
    forp_node_t *pn;

    if (FORP_G(nesting_level) > FORP_G(max_nesting_level)) {
        old_execute(op_array TSRMLS_CC);
    } else {
        pn = forp_begin(EG(current_execute_data), op_array TSRMLS_CC);
        old_execute(op_array TSRMLS_CC);
        forp_end(pn TSRMLS_CC);
    }
}
/* }}} */

/* {{{ forp_execute_internal
 */
void forp_execute_internal(zend_execute_data *current_execute_data, int ret TSRMLS_DC) {
    forp_node_t *pn;

    if (FORP_G(nesting_level) > FORP_G(max_nesting_level)) {
        execute_internal(current_execute_data, ret TSRMLS_CC);
    } else {
        pn = forp_begin(EG(current_execute_data), NULL TSRMLS_CC);
        if (old_execute_internal) {
            old_execute_internal(current_execute_data, ret TSRMLS_CC);
        } else {
            execute_internal(current_execute_data, ret TSRMLS_CC);
        }
        forp_end(pn TSRMLS_CC);
    }
}
/* }}} */

/* {{{ forp_stack_dump
 */
void forp_stack_dump(TSRMLS_D) {
    int i;
    zval *t;

    MAKE_STD_ZVAL(FORP_G(dump));
    array_init(FORP_G(dump));

    for (i = 0; i < FORP_G(stack_len); ++i) {
        forp_node_t *pn;

        pn = FORP_G(stack)[i];

        if (strstr(FORP_SKIP, pn->function.function)) {
            continue;
        }

        // stack entry
        MAKE_STD_ZVAL(t);
        array_init(t);

        if (pn->function.filename)
            add_assoc_string(t, FORP_DUMP_ASSOC_FILE, pn->function.filename, 1);

        if (pn->function.class)
            add_assoc_string(t, FORP_DUMP_ASSOC_CLASS, pn->function.class, 1);

        if (pn->function.function)
            add_assoc_string(t, FORP_DUMP_ASSOC_FUNCTION, pn->function.function, 1);

        if (pn->function.lineno)
            add_assoc_long(t, FORP_DUMP_ASSOC_LINENO, pn->function.lineno);

        zval *time;
        MAKE_STD_ZVAL(time);
        ZVAL_DOUBLE(time, round(pn->time * 1000000.0) / 1000000.0);
        add_assoc_zval(t, FORP_DUMP_ASSOC_CPU, time);
        add_assoc_long(t, FORP_DUMP_ASSOC_MEMORY, pn->mem);
        add_assoc_long(t, FORP_DUMP_ASSOC_LEVEL, pn->level);

        // {main} don't have parent
        if (pn->parent) add_assoc_long(t, FORP_DUMP_ASSOC_PARENT, pn->parent->key);

        if (zend_hash_next_index_insert(Z_ARRVAL_P(FORP_G(dump)), (void *) &t, sizeof (zval *), NULL) == FAILURE) {
            return;
        }
    }
}
/* }}} */

/* {{{ forp_stack_dump_cli_node
 */
void forp_stack_dump_cli_node(forp_node_t *node TSRMLS_DC) {
    int j;

    if (strstr(FORP_SKIP, node->function.function)) {
        return;
    }

    php_printf("[time:%09.0f] [memory:%09d] ", node->time, node->mem);
    for (j = 0; j < node->level; ++j) {
        if (j == node->level - 1) php_printf(" └── ");
        else php_printf(" |   ");
    }
    if (node->function.class) php_printf("%s::", node->function.class);
    php_printf("%s (%s)%s", node->function.function, node->function.filename, PHP_EOL);
}
/* }}} */

/* {{{ forp_stack_dump_cli
 */
void forp_stack_dump_cli(TSRMLS_D) {
    int i;
    php_printf("-----------------------------------------------------------------------------------------------------------%s", PHP_EOL);
    for (i = 0; i < FORP_G(stack_len); ++i) {
        forp_stack_dump_cli_node(FORP_G(stack)[i] TSRMLS_CC);
    }
    php_printf("-----------------------------------------------------------------------------------------------------------%s", PHP_EOL);
}
/* }}} */