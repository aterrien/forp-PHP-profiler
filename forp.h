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

#ifndef FORP_H
#define FORP_H

#include "php.h"
#include "forp_string.h"
#include "forp_log.h"

#ifdef ZTS
#include "TSRM.h"
#endif

#define FORP_DUMP_ASSOC_FILE            "file"
#define FORP_DUMP_ASSOC_CLASS           "class"
#define FORP_DUMP_ASSOC_FUNCTION        "function"
#define FORP_DUMP_ASSOC_LINENO          "lineno"
#define FORP_DUMP_ASSOC_DURATION        "usec"
#define FORP_DUMP_ASSOC_PROFILERTIME    "pusec"
#define FORP_DUMP_ASSOC_MEMORY          "bytes"
#define FORP_DUMP_ASSOC_LEVEL           "level"
#define FORP_DUMP_ASSOC_PARENT          "parent"
#define FORP_DUMP_ASSOC_GROUPS          "groups"
#define FORP_DUMP_ASSOC_CAPTION         "caption"
#define FORP_HIGHLIGHT_BEGIN            "<div style='position: relative; border: 1px solid #555; margin: 2px; padding: 20px 2px 2px 2px'>"
#define FORP_HIGHLIGHT_END              "<div style='position: absolute; top: 0px; right: 0px; left: 0px; height: 14px; background: #555; color: #eee; padding: 2px 5px; font-family: Consolas, monaco, monospace; font-size: 10px; text-align: right'>%s [%.03f ms, %d b, level %d]</div></div>"
#define FORP_STACK_REALLOC              1000

typedef struct forp_function_t {
    char *filename;
    char *class;
    char *function;
    int type;
    char **groups;
    int groups_len;
    char *highlight;
} forp_function_t;

typedef struct forp_node_t {
    int key;
    int state;

    char *filename;
    int lineno;
    int level;
    struct forp_node_t *parent;
    char *caption;
    char *alias;

    forp_function_t function;

    // Memory
    signed long mem;
    signed long mem_begin;
    signed long mem_end;

    // Duration
    double time;
    double time_begin;
    double time_end;

    // Self cost
    double profiler_duration;
} forp_node_t;


/* Zend API proxies */
#if PHP_VERSION_ID < 50500
void (*old_execute)(zend_op_array *op_array TSRMLS_DC);
void forp_execute(zend_op_array *op_array TSRMLS_DC);
void (*old_execute_internal)(zend_execute_data *current_execute_data, int return_value_used TSRMLS_DC);
void forp_execute_internal(zend_execute_data *current_execute_data, int return_value_used TSRMLS_DC);
#else
void (*old_execute_ex)(zend_execute_data *execute_data TSRMLS_DC);
void forp_execute_ex(zend_execute_data *execute_data TSRMLS_DC);
void (*old_execute_internal)(zend_execute_data *current_execute_data, struct _zend_fcall_info *fci, int return_value_used TSRMLS_DC);
void forp_execute_internal(zend_execute_data *current_execute_data, struct _zend_fcall_info *fci, int return_value_used TSRMLS_DC);
#endif


static void forp_populate_function(forp_function_t *function, zend_execute_data *edata, zend_op_array *op_array TSRMLS_DC);

void forp_info(TSRMLS_D);

void forp_start(TSRMLS_D);

void forp_end(TSRMLS_D);

forp_node_t *forp_open_node(zend_execute_data *edata, zend_op_array *op_array TSRMLS_DC);

void forp_close_node(forp_node_t *pn TSRMLS_DC);

zval *forp_stack_dump_var(forp_var_t *var TSRMLS_DC);

void forp_stack_dump(TSRMLS_D);

void forp_stack_dump_cli_node(forp_node_t *node TSRMLS_DC);

void forp_stack_dump_cli_var(forp_var_t *var, int depth TSRMLS_DC);

void forp_stack_dump_cli(TSRMLS_D);

int forp_is_profiling_function(forp_node_t *n TSRMLS_DC);

#endif  /* FORP_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
