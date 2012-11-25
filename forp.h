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
#ifdef ZTS
#include "TSRM.h"
#endif

#define FORP_NODE_TYPE_FUNCTION		0x0001
#define FORP_NODE_TYPE_GROUP    	0x0002

typedef struct forp_function_t {
    char *filename;
    char *class;
    char *function;
    int lineno;
    int type;
    char *group;
} forp_function_t;


//typedef struct forp_group_t {
//    char *name;
//} forp_group_t;

typedef struct forp_node_t {
    int key;
    //int type;
    int level;
    struct forp_node_t *parent;
    char *caption;

    // Type node specific
    //forp_group_t group;
    forp_function_t function;

    // Memory
    signed long mem;
    signed long mem_begin;
    signed long mem_end;

    // Duration
    double time;
    double time_begin;
    double time_end;
} forp_node_t;

/* proxy */
zend_op_array* (*old_compile_file)(zend_file_handle* file_handle, int type TSRMLS_DC);
zend_op_array* forp_compile_file(zend_file_handle*, int TSRMLS_DC);

void (*old_execute)(zend_op_array *op_array TSRMLS_DC);
void forp_execute(zend_op_array *op_array TSRMLS_DC);

void (*old_execute_internal)(zend_execute_data *current_execute_data, int return_value_used TSRMLS_DC);
void forp_execute_internal(zend_execute_data *current_execute_data, int return_value_used TSRMLS_DC);

/* public functions */
static void forp_populate_function(forp_function_t *function, zend_execute_data *edata, zend_op_array *op_array TSRMLS_DC);

forp_node_t *forp_begin(zend_execute_data *edata, zend_op_array *op_array TSRMLS_DC);

void forp_info();

zend_op_array *forp_compile_file(zend_file_handle *file_handle, int type TSRMLS_DC);

void forp_end(forp_node_t *pn TSRMLS_DC);

void forp_execute(zend_op_array *op_array TSRMLS_DC);

void forp_execute_internal(zend_execute_data *current_execute_data, int ret TSRMLS_DC);

void forp_stack_dump(TSRMLS_D);

void forp_stack_dump_cli_node(forp_node_t *node TSRMLS_DC);

void forp_stack_dump_cli(TSRMLS_D);

#ifndef POSIX
char* forp_strndup(const char* s, size_t n) {
    size_t slen = (size_t)strlen(s);
    char* copy;
    if (slen < n) {
        n = slen;
    }
    copy = malloc(n+1);
    if (copy) {
        memcpy(copy, s, n);
        copy[n] = 0;
    }
    return copy;
}
char *forp_strndup(const char *s, size_t n);
#define strndup(s,n) forp_strndup(s, n)
#endif*/

#endif  /* FORP_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */