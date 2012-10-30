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

/* {{{ forp_annotation
 *
 * Handles annotations in doc_comment : @<tag>("<value>")<\n>
 *
 * @param char* doc_comment
 * @param char* tag
 * @return char*
 */
char *forp_annotation(char *doc_comment, char *tag TSRMLS_DC) {
    char *v = NULL, *v_search = NULL, *t_start = NULL, *tmp = NULL, *eot = NULL;
    unsigned int v_start, v_end;

    v_search = emalloc(sizeof(char*) * (strlen(tag) + 3));
    if(v_search) {
        sprintf(v_search, "@%s(\"", tag);
        t_start = strstr(doc_comment, v_search);
        if (t_start) {
            v_start = t_start - doc_comment + strlen(v_search);
            tmp = strndup(doc_comment + v_start, strlen(doc_comment));
            eot = strstr(tmp, "\")\n");
            v_end = eot - tmp;
            v = strndup(doc_comment + v_start, v_end);
        }
        efree(v_search);
    }
    return v;
}
/* }}} */

/* {{{ forp_substr_replace
 *
 * @param char* subject
 * @param uint start
 * @param uint len
 * @param char* replace
 * @return char*
 */
char *forp_substr_replace(char *subject, char *replace, unsigned int start, unsigned int len)
{
   char *ns = NULL;
   size_t size;
   if (
        subject != NULL && replace != NULL
        && start >= 0 && len > 0
   ) {
      size = strlen(subject);
      ns = emalloc(sizeof(*ns) * (size - len + strlen(replace) + 1));
      if(ns) {
         memcpy(ns, subject, start);
         memcpy(&ns[start], replace, strlen(replace));
         memcpy(&ns[start + strlen(replace)], &subject[start + len], size - len - start + 1);
      }
      subject = strdup(ns);
      efree(ns);
   }
   return subject;
}
/* }}} */

/* {{{ forp_str_replace
 *
 * @param char* search
 * @param char* replace
 * @param char* subject
 * @return char*
 */
char *forp_str_replace(char *search, char *replace, char *subject TSRMLS_DC) {
    char *found = strstr(subject, search);
    if(found) {
        subject = forp_substr_replace(subject, replace, found - subject, strlen(search));
    }
    return subject;
}
/* }}} */

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
    function->caption = NULL;
    function->group = NULL;

    // Retrieves filename
    if (op_array) {
        function->filename = strdup(op_array->filename);

        if((FORP_G(flags) & FORP_FLAG_ANNOTATION) && op_array->doc_comment) {
            function->caption =
                    forp_annotation(op_array->doc_comment, "ProfileCaption" TSRMLS_CC);
            function->group =
                    forp_annotation(op_array->doc_comment, "ProfileGroup" TSRMLS_CC);
        }
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

    // Collects params
    if(function->caption) {
        void **params;
        int params_count;
        int i;

        // Retrieves params in global zend_vm_stack
#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)
        params = EG(argument_stack)->top-1;
#else
        params = EG(argument_stack).top_element-1;
#endif
        params_count = (ulong) *params;

        for(i = 1; i <= params_count; i++) {

            char c[4];
            char *v;

            sprintf(c, "#%d", params_count - i + 1);

            zval *expr;
            expr = *((zval **) (params - i));

            // Uses zend_make_printable_zval
            zval expr_copy;
            int use_copy;
            zend_make_printable_zval(expr, &expr_copy, &use_copy);
            if(use_copy) {
                v = strdup((char*)(expr_copy).value.str.val);
                zval_dtor(&expr_copy);
            } else {
                v = strdup((char*)(*expr).value.str.val);
            }
            /*switch(Z_TYPE_P(expr)) {
                case IS_STRING :
                    v = strdup((char*)(*expr).value.str.val);
                    break;
                case IS_DOUBLE :
                    //convert_to_string
                    break;
                case IS_OBJECT :
                    v = "(Object)";
                    break;
                case IS_ARRAY :
                    v = "(Array)";
                    break;
                case IS_RESOURCE :
                    v = "(Resource)";
                    break;
                default :
                    v = "(not a string)";
            }*/

            function->caption = forp_str_replace(
                c, v,
                function->caption TSRMLS_CC
            );
        }
    }
}
/* }}} */

/* {{{ forp_begin
 */
forp_node_t *forp_begin(zend_execute_data *edata, zend_op_array *op_array TSRMLS_DC) {
    struct timeval tv;
    forp_node_t *n;
    int key;

    n = emalloc(sizeof (forp_node_t));
    n->level = FORP_G(nesting_level)++;
    n->parent = FORP_G(current_node);
    n->time_begin = 0;
    n->time_end = 0;
    n->time = 0;
    n->mem_begin = 0;
    n->mem_end = 0;
    n->mem = 0;

    if(edata) {
        forp_populate_function(&(n->function), edata, op_array TSRMLS_CC);
    } else {
        // Root node
        edata = EG(current_execute_data);
        n->function.class = NULL;
        n->function.function = "{main}";
        n->function.filename = edata->op_array->filename;
        n->function.lineno = 0;
        n->function.caption = NULL;
        n->function.group = NULL;
    }

    FORP_G(current_node) = n;
    key = FORP_G(stack_len);
    n->key = key;
    FORP_G(stack_len)++;
    FORP_G(stack) = erealloc(
            FORP_G(stack),
            FORP_G(stack_len) * sizeof (forp_node_t)
            );
    FORP_G(stack)[key] = n;

    if(FORP_G(flags) & FORP_FLAG_CPU) {
        gettimeofday(&tv, NULL);
        n->time_begin = tv.tv_sec * 1000000.0 + tv.tv_usec;
    }

    if(FORP_G(flags) & FORP_FLAG_MEMORY) {
        n->mem_begin = zend_memory_usage(0 TSRMLS_CC);
    }

    return n;
}
/* }}} */

/* {{{ forp_info
 */
void forp_info(TSRMLS_D) {
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
void forp_end(forp_node_t *n TSRMLS_DC) {
    struct timeval tv;

    // dump memory before next steps
    if(FORP_G(flags) & FORP_FLAG_MEMORY) {
        n->mem_end = zend_memory_usage(0 TSRMLS_CC);
        n->mem = n->mem_end - n->mem_begin;
    }

    if(FORP_G(flags) & FORP_FLAG_CPU) {
        gettimeofday(&tv, NULL);
        n->time_end = tv.tv_sec * 1000000.0 + tv.tv_usec;
        n->time = n->time_end - n->time_begin;
    }

    FORP_G(current_node) = n->parent;
    FORP_G(nesting_level)--;
}
/* }}} */

/* {{{ forp_execute
 */
void forp_execute(zend_op_array *op_array TSRMLS_DC) {
    forp_node_t *n;

    if (FORP_G(nesting_level) > FORP_G(max_nesting_level)) {
        old_execute(op_array TSRMLS_CC);
    } else {
        n = forp_begin(EG(current_execute_data), op_array TSRMLS_CC);
        old_execute(op_array TSRMLS_CC);
        forp_end(n TSRMLS_CC);
    }
}
/* }}} */

/* {{{ forp_execute_internal
 */
void forp_execute_internal(zend_execute_data *current_execute_data, int ret TSRMLS_DC) {
    forp_node_t *n;

    if (FORP_G(nesting_level) > FORP_G(max_nesting_level)) {
        execute_internal(current_execute_data, ret TSRMLS_CC);
    } else {
        n = forp_begin(EG(current_execute_data), NULL TSRMLS_CC);
        if (old_execute_internal) {
            old_execute_internal(current_execute_data, ret TSRMLS_CC);
        } else {
            execute_internal(current_execute_data, ret TSRMLS_CC);
        }
        forp_end(n TSRMLS_CC);
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
        forp_node_t *n;

        n = FORP_G(stack)[i];

        if (strstr(FORP_SKIP, n->function.function)) {
            continue;
        }

        // stack entry
        MAKE_STD_ZVAL(t);
        array_init(t);

        if (n->function.filename)
            add_assoc_string(t, FORP_DUMP_ASSOC_FILE, n->function.filename, 1);

        if (n->function.class)
            add_assoc_string(t, FORP_DUMP_ASSOC_CLASS, n->function.class, 1);

        if (n->function.function)
            add_assoc_string(t, FORP_DUMP_ASSOC_FUNCTION, n->function.function, 1);

        if (n->function.lineno)
            add_assoc_long(t, FORP_DUMP_ASSOC_LINENO, n->function.lineno);

        if (n->function.group)
            add_assoc_string(t, FORP_DUMP_ASSOC_GROUP, n->function.group, 1);

        if (n->function.caption)
            add_assoc_string(t, FORP_DUMP_ASSOC_CAPTION, n->function.caption, 1);

        if(FORP_G(flags) & FORP_FLAG_CPU) {
            zval *time;
            MAKE_STD_ZVAL(time);
            ZVAL_DOUBLE(time, round(n->time * 1000000.0) / 1000000.0);
            add_assoc_zval(t, FORP_DUMP_ASSOC_CPU, time);
        }

        if(FORP_G(flags) & FORP_FLAG_MEMORY) {
            add_assoc_long(t, FORP_DUMP_ASSOC_MEMORY, n->mem);
        }

        add_assoc_long(t, FORP_DUMP_ASSOC_LEVEL, n->level);

        // {main} don't have parent
        if (n->parent) add_assoc_long(t, FORP_DUMP_ASSOC_PARENT, n->parent->key);

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

    if(FORP_G(flags) & FORP_FLAG_CPU) {
        php_printf("[time:%09.0f] ", node->time);
    }
    if(FORP_G(flags) & FORP_FLAG_MEMORY) {
        php_printf("[memory:%09d] ", node->mem);
    }
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