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

#include "forp_string.h"
#include "forp_annotation.h"

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#elif defined(PHP_WIN32)
#include "win32/time.h"
#include <math.h>
static inline double round(double val) {
    return floor(val + 0.5);
}
#endif

#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#if defined(PHP_WIN32)
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
#define strndup(s,n) forp_strndup(s, n)
#endif

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
    function->_filename = NULL;
    function->filename = NULL;
    function->lineno = 0;

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

    // Retrieves filename
    if (op_array) {
        function->_filename = strdup(op_array->filename);
    } else {
        if (edata->op_array) {
            function->_filename = strdup(edata->op_array->filename);
        } else {
            function->_filename = "";
        }
    }
    if(FORP_G(current_node)) {
        function->filename = strdup(FORP_G(current_node)->function._filename);
    }

    // Retrieves call lineno
    if(edata->opline) {
        function->lineno = edata->opline->lineno;
    }
}
/* }}} */

/* {{{ forp_open_node
 */
forp_node_t *forp_open_node(zend_execute_data *edata, zend_op_array *op_array TSRMLS_DC) {
    struct timeval tv;
    forp_node_t *n;
    int key;
    double start_time;

    // self duration on open
    gettimeofday(&tv, NULL);
    start_time = tv.tv_sec * 1000000.0 + tv.tv_usec;

    // Inits node
    n = malloc(sizeof (forp_node_t));

    n->level = FORP_G(nesting_level)++;
    n->state = 1; // opened
    n->parent = FORP_G(current_node);

    n->time_begin = 0;
    n->time_end = 0;
    n->time = 0;

    n->profiler_duration = 0;

    n->mem_begin = 0;
    n->mem_end = 0;
    n->mem = 0;

    // Annotations
    n->alias = NULL;
    n->caption = NULL;
    n->function.groups = NULL;
    n->function.groups_len = 0;

    // Handles fn annotations to know what to do
    if((FORP_G(flags) & FORP_FLAG_ANNOTATIONS) && op_array && op_array->doc_comment) {

        // TODO Optimize it !

        // Alias : allows to give a name to anonymous functions
        n->alias = forp_annotation_string(op_array->doc_comment, "ProfileAlias" TSRMLS_CC);

        // Caption
        n->caption = forp_annotation_string(op_array->doc_comment, "ProfileCaption" TSRMLS_CC);

        // Group
        // TODO no alloc / realloc when group found
        n->function.groups = malloc(sizeof(char*) * 10);
        n->function.groups_len = 0;
        forp_annotation_array(op_array->doc_comment, "ProfileGroup", &(n->function.groups), &(n->function.groups_len) TSRMLS_CC);

        // Frame
        n->function.highlight = forp_annotation_string(op_array->doc_comment, "ProfileHighlight" TSRMLS_CC);
        if(n->function.highlight) php_printf(FORP_HIGHLIGHT_BEGIN);

    } else {
        n->function.highlight = NULL;
    }

    // Node of type function
    if(edata) {
        forp_populate_function(&(n->function), edata, op_array TSRMLS_CC);

        // Collects params
        if(n->caption) {
            void **params;
            int params_count;
            int i;

            // Retrieves params in zend_vm_stack
#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)
            params = EG(argument_stack)->top-1;
#else
            params = EG(argument_stack).top_element-1;
#endif
            params_count = (ulong) *params;

            // TODO extract only required parameters
            for(i = 1; i <= params_count; i++) {

                char c[4];
                char *v, *v_copy;
                zval *expr;
                zval expr_copy;
                int use_copy;

                sprintf(c, "#%d", params_count - i + 1);
                expr = *((zval **) (params - i));

                if(Z_TYPE_P(expr) == IS_OBJECT) {
                    // Object or Closure
                    // Closure throws a Recoverable Fatal in zend_make_printable_zval
                    v_copy = malloc(sizeof(char*) * (strlen(Z_OBJCE_P(expr)->name) + 2));
                    sprintf(v_copy, "(%s)", Z_OBJCE_P(expr)->name);
                    v = strdup(v_copy);
                    free(v_copy);
                } else {
                    // Uses zend_make_printable_zval
                    zend_make_printable_zval(expr, &expr_copy, &use_copy);
                    if(use_copy) {
                        v = strdup((char*)(expr_copy).value.str.val);
                        zval_dtor(&expr_copy);
                    } else {
                        v = strdup((char*)(*expr).value.str.val);
                    }
                }

                n->caption = forp_str_replace(
                    c, v,
                    n->caption TSRMLS_CC
                );
            }
        }

    } else {
        // Root node
        edata = EG(current_execute_data);
        //n->parent = NULL;
        n->function.class = NULL;
        n->function.function = "{main}";
        n->function._filename = edata->op_array->filename;
        n->function.filename = strdup(n->function._filename);
        n->function.lineno = 0;
    }

    FORP_G(current_node) = n;
    key = FORP_G(stack_len);
    n->key = key;
    FORP_G(stack_len)++;
    FORP_G(stack) = realloc(
        FORP_G(stack),
        FORP_G(stack_len) * sizeof (forp_node_t)
    );
    FORP_G(stack)[key] = n;

    if(FORP_G(flags) & FORP_FLAG_MEMORY) {
        n->mem_begin = zend_memory_usage(0 TSRMLS_CC);
    }

    if(FORP_G(flags) & FORP_FLAG_TIME) {
        gettimeofday(&tv, NULL);
        n->time_begin = tv.tv_sec * 1000000.0 + tv.tv_usec;
        n->profiler_duration = n->time_begin - start_time;
    }

    return n;
}
/* }}} */

/* {{{ forp_close_node
 */
void forp_close_node(forp_node_t *n TSRMLS_DC) {

    struct timeval tv;

    // closed state
    n->state = 2;

    // dump duration and memory before next steps
    if(FORP_G(flags) & FORP_FLAG_TIME) {
        gettimeofday(&tv, NULL);
        n->time_end = tv.tv_sec * 1000000.0 + tv.tv_usec;
        n->time = n->time_end - n->time_begin;
    }

    if(FORP_G(flags) & FORP_FLAG_MEMORY) {
        n->mem_end = zend_memory_usage(0 TSRMLS_CC);
        n->mem = n->mem_end - n->mem_begin;
    }

    if(n->function.highlight) {
        php_printf(FORP_HIGHLIGHT_END, (n->time / 1000), (n->mem / 1024), n->level);
    }

    FORP_G(current_node) = n->parent;
    FORP_G(nesting_level)--;

    // self duration on exit
    if(FORP_G(flags) & FORP_FLAG_TIME) {
        gettimeofday(&tv, NULL);
        n->profiler_duration += (tv.tv_sec * 1000000.0 + tv.tv_usec) - n->time_end ;
    }
}
/* }}} */

/* {{{ forp_start
 */
void forp_start(TSRMLS_D) {

    if(FORP_G(started)) {
        php_error_docref(
            NULL TSRMLS_CC,
            E_NOTICE,
            "forp is already started."
            );
    } else {
        FORP_G(started) = 1;

#if HAVE_SYS_RESOURCE_H
        if(FORP_G(flags) & FORP_FLAG_CPU) {
            struct rusage ru;
            getrusage(RUSAGE_SELF, &ru);
            FORP_G(utime) = ru.ru_utime.tv_sec * 1000000.0 + ru.ru_utime.tv_usec;
            FORP_G(stime) = ru.ru_stime.tv_sec * 1000000.0 + ru.ru_stime.tv_usec;
        }
#endif

        // Proxying zend api methods
        old_execute = zend_execute;
        zend_execute = forp_execute;

        if (!FORP_G(no_internals)) {
            old_compile_file = zend_compile_file;
            zend_compile_file = forp_compile_file;

            old_execute_internal = zend_execute_internal;
            zend_execute_internal = forp_execute_internal;
        }

        FORP_G(main) = forp_open_node(NULL, NULL TSRMLS_CC);
    }
}
/* }}} */

/* {{{ forp_end
 */
void forp_end(TSRMLS_D) {

    if(FORP_G(started)) {

#if HAVE_SYS_RESOURCE_H
        if(FORP_G(flags) & FORP_FLAG_CPU) {
            struct rusage ru;
            getrusage(RUSAGE_SELF, &ru);
            FORP_G(utime) = (ru.ru_utime.tv_sec * 1000000.0 + ru.ru_utime.tv_usec) - FORP_G(utime);
            FORP_G(stime) = (ru.ru_stime.tv_sec * 1000000.0 + ru.ru_stime.tv_usec) - FORP_G(stime);
        }
#endif

        // Close ancestors
        while(FORP_G(current_node)) {
            forp_close_node(FORP_G(current_node) TSRMLS_CC);
        }

        // Close main
        //forp_close_node(FORP_G(main) TSRMLS_CC);

        // Restores zend api methods
        if (old_execute) {
            zend_execute = old_execute;
        }
        if (!FORP_G(no_internals)) {
            zend_compile_file = old_compile_file;
            zend_execute_internal = old_execute_internal;
        }

        // Stop
        FORP_G(started) = 0;
    }
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

/* {{{ forp_execute
 */
void forp_execute(zend_op_array *op_array TSRMLS_DC) {
    forp_node_t *n;

    if (FORP_G(nesting_level) > FORP_G(max_nesting_level)) {
        old_execute(op_array TSRMLS_CC);
    } else {
        n = forp_open_node(EG(current_execute_data), op_array TSRMLS_CC);
        old_execute(op_array TSRMLS_CC);
        if(n->state < 2) forp_close_node(n TSRMLS_CC);
    }
}
/* }}} */

/* {{{ forp_execute_internal
 */
void forp_execute_internal(zend_execute_data *current_execute_data, int ret TSRMLS_DC)
{
    forp_node_t *n;

    if (FORP_G(nesting_level) > FORP_G(max_nesting_level)) {
        execute_internal(current_execute_data, ret TSRMLS_CC);
    } else {
        n = forp_open_node(EG(current_execute_data), NULL TSRMLS_CC);
        if (old_execute_internal) {
            old_execute_internal(current_execute_data, ret TSRMLS_CC);
        } else {
            execute_internal(current_execute_data, ret TSRMLS_CC);
        }
        if(n->state < 2) forp_close_node(n TSRMLS_CC);
    }
}
/* }}} */

/* {{{ forp_stack_dump
 */
void forp_stack_dump(TSRMLS_D) {
    int i;
    int j = 0;
    zval *entry, *stack;

    /**
     * array(
     *  'stime' => ,
     *  'utime' => ,
     *  'stack' => array(... entry+ ...)
     * )
     */

    MAKE_STD_ZVAL(FORP_G(dump));
    array_init(FORP_G(dump));

    if(FORP_G(flags) & FORP_FLAG_CPU) {
        add_assoc_double(FORP_G(dump), "utime", FORP_G(utime));
        add_assoc_double(FORP_G(dump), "stime", FORP_G(stime));
    }

    MAKE_STD_ZVAL(stack);
    array_init(stack);
    add_assoc_zval(FORP_G(dump), "stack", stack);

    for (i = 0; i < FORP_G(stack_len); ++i) {
        forp_node_t *n;

        n = FORP_G(stack)[i];

        if (
            strstr(n->function.function,"forp_dump")
            || strstr(n->function.function,"forp_end")
            || strstr(n->function.function,"forp_start")
        ) {
            continue;
        }

        // stack entry
        MAKE_STD_ZVAL(entry);
        array_init(entry);

        if (n->function.filename)
            add_assoc_string(entry, FORP_DUMP_ASSOC_FILE, n->function.filename, 1);

        if (n->function.class)
            add_assoc_string(entry, FORP_DUMP_ASSOC_CLASS, n->function.class, 1);

        if(n->alias) {
            add_assoc_string(entry, FORP_DUMP_ASSOC_FUNCTION, n->alias, 1);
        } else if (n->function.function) {
            add_assoc_string(entry, FORP_DUMP_ASSOC_FUNCTION, n->function.function, 1);
        }

        if (n->function.lineno)
            add_assoc_long(entry, FORP_DUMP_ASSOC_LINENO, n->function.lineno);

        if (n->function.groups && n->function.groups_len > 0) {
            zval *groups;
            MAKE_STD_ZVAL(groups);
            array_init(groups);
            j = 0;
            while(j < n->function.groups_len) {
                add_next_index_string(groups, n->function.groups[j], 1);
                j++;
            }
            add_assoc_zval(entry, FORP_DUMP_ASSOC_GROUPS, groups);
        }

        if (n->caption) {
            add_assoc_string(entry, FORP_DUMP_ASSOC_CAPTION, n->caption, 1);
        }

        if(FORP_G(flags) & FORP_FLAG_TIME) {
            zval *time, *profiler_duration;
            MAKE_STD_ZVAL(time);
            ZVAL_DOUBLE(time, round(n->time * 1000000.0) / 1000000.0);
            add_assoc_zval(entry, FORP_DUMP_ASSOC_DURATION, time);

            MAKE_STD_ZVAL(profiler_duration);
            ZVAL_DOUBLE(profiler_duration, round(n->profiler_duration * 1000000.0) / 1000000.0);
            add_assoc_zval(entry, FORP_DUMP_ASSOC_PROFILERTIME, profiler_duration);
        }

        if(FORP_G(flags) & FORP_FLAG_MEMORY) {
            add_assoc_long(entry, FORP_DUMP_ASSOC_MEMORY, n->mem);
        }

        add_assoc_long(entry, FORP_DUMP_ASSOC_LEVEL, n->level);

        // {main} don't have parent
        if (n->parent)
            add_assoc_long(entry, FORP_DUMP_ASSOC_PARENT, n->parent->key);

        if (
            zend_hash_next_index_insert(Z_ARRVAL_P(stack), (void *) &entry,
            sizeof (zval *), NULL) == FAILURE
        ) {
            return;
        }
    }
}
/* }}} */

/* {{{ forp_stack_dump_cli_node
 */
void forp_stack_dump_cli_node(forp_node_t *node TSRMLS_DC) {
    int j;

    if(FORP_G(flags) & FORP_FLAG_TIME) {
        php_printf("[time:%09.0f] ", node->time);
    }
    if(FORP_G(flags) & FORP_FLAG_MEMORY) {
        php_printf("[memory:%09d] ", node->mem);
    }
    for (j = 0; j < node->level; ++j) {
        if (j == node->level - 1) php_printf(" |--- ");
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
    php_printf("--------------------------------------------------------------------------------%s", PHP_EOL);
    for (i = 0; i < FORP_G(stack_len); ++i) {
        forp_stack_dump_cli_node(FORP_G(stack)[i] TSRMLS_CC);
    }
    php_printf("--------------------------------------------------------------------------------%s", PHP_EOL);
}
/* }}} */

/* {{{ forp_inspect
 */
void forp_inspect(zval *expr TSRMLS_DC) {
    php_printf("Not implemented yet !");
}
/* }}} */