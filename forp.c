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
#include "ext/standard/info.h"
#include "php_forp.h"
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#elif defined(PHP_WIN32)
#include "win32/time.h"
#endif
#include <stdio.h>
#include "zend_exceptions.h"

/* proxy */
zend_op_array* (*old_compile_file)(zend_file_handle* file_handle, int type TSRMLS_DC);
zend_op_array* forp_compile_file(zend_file_handle*, int TSRMLS_DC);

void (*old_execute)(zend_op_array *op_array TSRMLS_DC);
void forp_execute(zend_op_array *op_array TSRMLS_DC);

void (*old_execute_internal)(zend_execute_data *current_execute_data, int return_value_used TSRMLS_DC);
void forp_execute_internal(zend_execute_data *current_execute_data, int return_value_used TSRMLS_DC);

ZEND_DECLARE_MODULE_GLOBALS(forp)

/* {{{ forp_functions[]
 *
 * Every user visible function must have an entry in forp_functions[].
 */
const zend_function_entry forp_functions[] = {
	PHP_FE(forp_enable,NULL)
	PHP_FE(forp_dump, NULL)
	PHP_FE(forp_print, NULL)
	PHP_FE(forp_info, NULL)
	PHP_FE_END	/* Must be the last line in forp_functions[] */
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
	PHP_MINIT(forp),	/* Main init */
	PHP_MSHUTDOWN(forp),	/* Main shutdown */
	NULL, 			/* Request init */
	PHP_RSHUTDOWN(forp),	/* Request shutdown */
	PHP_MINFO(forp),
#if ZEND_MODULE_API_NO >= 20010901
	FORP_VERSION,
#endif
	NO_MODULE_GLOBALS, 
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
	STD_PHP_INI_BOOLEAN("forp.enable","0", PHP_INI_ALL, OnUpdateBool, enable, zend_forp_globals, forp_globals)
	STD_PHP_INI_ENTRY("forp.max_nesting_level","10", PHP_INI_ALL, OnUpdateLong, max_nesting_level, zend_forp_globals, forp_globals)
	STD_PHP_INI_BOOLEAN("forp.no_internal","1", PHP_INI_ALL, OnUpdateBool, no_internal, zend_forp_globals, forp_globals)
PHP_INI_END()
/* }}} */

/* {{{ php_forp_init_globals
 */
static void php_forp_init_globals(zend_forp_globals *forp_globals)
{
	forp_globals->enable = 0;
	forp_globals->max_nesting_level = 10;
	forp_globals->no_internal = 1;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(forp)
{
	/* If you have INI entries, uncomment these lines */ 
	REGISTER_INI_ENTRIES();
	
	/* Prof const */
	REGISTER_LONG_CONSTANT("FORP_MEMORY", FORP_MEMORY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FORP_CPU", FORP_CPU, CONST_CS | CONST_PERSISTENT);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(forp)
{
        UNREGISTER_INI_ENTRIES();
        return SUCCESS;
}
/* }}} */

/* {{{ forp_populate_function
 */
static void forp_populate_function(
	forp_function *function, 
	zend_execute_data *edata, 
	zend_op_array *op_array TSRMLS_DC
)
{
       	function->class = NULL;
	function->function = NULL;
        function->filename = NULL;

	if (edata) {

		// filename
		if (op_array) {
			function->filename = strdup(op_array->filename);
		} else {
			if (edata->op_array) {
				function->filename = strdup(edata->op_array->filename);
			} else {
				function->filename = "";
			}
		}

		// class and function names
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
        }else{
		edata = EG(current_execute_data);	
		function->class = NULL;
		function->function = "{main}";
		function->filename = edata->op_array->filename;
	}
}
/* }}} */

/* {{{ forp_begin
 */
forp_node *forp_begin(zend_execute_data *edata, zend_op_array *op_array TSRMLS_DC)
{
	struct timeval tv;
        forp_node *pn;
	int key;
       
	pn = emalloc(sizeof(forp_node));
        pn->level = FORP_G(nesting_level)++;
	pn->parent = FORP_G(current_node);

	forp_populate_function( &(pn->function), edata, op_array TSRMLS_CC);
			
	FORP_G(current_node) = pn;
	key = FORP_G(stack_len);
	pn->key = key;
	FORP_G(stack_len)++;
	FORP_G(stack) =  erealloc(FORP_G(stack), FORP_G(stack_len) * sizeof(forp_node));
	FORP_G(stack)[key] = pn;
	
	gettimeofday(&tv, NULL);
        pn->time_begin = tv.tv_sec * 1000000.0 + tv.tv_usec;

	pn->mem_begin = zend_memory_usage(0 TSRMLS_CC);

	return pn;
}
/* }}} */

/* {{{ forp_info
 */
void forp_info()
{
	php_info_print_table_start();
        php_info_print_table_row(2, "Version", FORP_VERSION);
        php_info_print_table_end();
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(forp)
{
	forp_info();
	DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(forp)
{
	// Restoring zend api methods
        if (old_execute) {
        	zend_execute = old_execute;
        	old_execute = NULL;
	}
	if (!FORP_G(no_internal)) {
                zend_compile_file = old_compile_file;
                zend_execute_internal = old_execute_internal;
        }
	return SUCCESS;
}
/* }}} */

/* {{{ forp_info
 */
ZEND_FUNCTION(forp_info)
{
	php_info_print_style();
	forp_info();
}
/* }}} */

/* {{{ forp_enable
 */
ZEND_FUNCTION(forp_enable)
{
	long opt = FORP_MEMORY | FORP_CPU;
	/*if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &opt) == FAILURE) {
                return;
        }*/
	FORP_G(enable) = opt; 
        if (FORP_G(enable)) {
        	// Proxying zend api methods
        	old_execute = zend_execute;
        	zend_execute = forp_execute;
		if (!FORP_G(no_internal)) {
			old_compile_file = zend_compile_file;
                	zend_compile_file = forp_compile_file;

        		old_execute_internal = zend_execute_internal;
        		zend_execute_internal = forp_execute_internal;
		}
		FORP_G(main) = forp_begin(NULL, NULL TSRMLS_CC);
	}
}
/* }}} */

/* {{{ forp_compile_file
 */
zend_op_array *forp_compile_file(zend_file_handle *file_handle, int type TSRMLS_DC)
{
	return old_compile_file(file_handle, type TSRMLS_CC);
}
/* }}} */

/* {{{ forp_end
 */
void forp_end(forp_node *pn TSRMLS_DC)
{
	struct timeval tv;

        /* Memory capture before next steps */
        pn->mem_end = zend_memory_usage(0 TSRMLS_CC);
        pn->mem = pn->mem_end-pn->mem_begin;
        
        gettimeofday(&tv, NULL);
        pn->time_end = tv.tv_sec * 1000000.0 + tv.tv_usec;
        pn->time = pn->time_end - pn->time_begin;
        
	/* Back to parent nesting level */
        FORP_G(current_node) = pn->parent;
        FORP_G(nesting_level)--;
}
/* }}} */

/* {{{ forp_execute
 */
void forp_execute(zend_op_array *op_array TSRMLS_DC)
{
        forp_node *pn;

	if (FORP_G(nesting_level) > FORP_G(max_nesting_level)) {
		old_execute(op_array TSRMLS_CC);
	}else{
		pn = forp_begin(EG(current_execute_data), op_array TSRMLS_CC);
		old_execute(op_array TSRMLS_CC);
		forp_end(pn TSRMLS_CC);
	}
}
/* }}} */

/* {{{ forp_execute_internal
 */
void forp_execute_internal(zend_execute_data *current_execute_data, int ret TSRMLS_DC)
{
	forp_node *pn;

	if (FORP_G(nesting_level) > FORP_G(max_nesting_level)) {
		execute_internal(current_execute_data, ret TSRMLS_CC);
	}else{
		pn = forp_begin(EG(current_execute_data), NULL TSRMLS_CC);
		if (old_execute_internal) {
			old_execute_internal(current_execute_data, ret TSRMLS_CC);
		}else{
			execute_internal(current_execute_data, ret TSRMLS_CC);
		}
		forp_end(pn TSRMLS_CC);
	}
}
/* }}} */

/* {{{ forp_stack_dump
 */
void forp_stack_dump()
{
	int i;

	MAKE_STD_ZVAL(FORP_G(dump));
	array_init(FORP_G(dump));
	
	for (i = 0; i < FORP_G(stack_len); ++i) {
                forp_node *pn;
		pn = FORP_G(stack)[i];

		if (strstr(FORP_SKIP, pn->function.function)) {
                        continue;
                }
		
		// stack entry
		zval *t;
		MAKE_STD_ZVAL(t);
	        array_init(t);

		if (pn->function.filename) 
			add_assoc_string(t, FORP_DUMP_ASSOC_FILE, pn->function.filename, 1);

		if (pn->function.class) 
			add_assoc_string(t, FORP_DUMP_ASSOC_CLASS, pn->function.class, 1);

		if (pn->function.function)
			add_assoc_string(t, FORP_DUMP_ASSOC_FUNCTION, pn->function.function, 1);
		
		zval *time;
		MAKE_STD_ZVAL(time);
		ZVAL_DOUBLE(time, round(pn->time * 1000000.0) / 1000000.0);
		add_assoc_zval(t, FORP_DUMP_ASSOC_CPU, time);
		add_assoc_long(t, FORP_DUMP_ASSOC_MEMORY, pn->mem);
		add_assoc_long(t, FORP_DUMP_ASSOC_LEVEL, pn->level);

		// {main} don't have parent
		if (pn->parent) add_assoc_long(t, FORP_DUMP_ASSOC_PARENT, pn->parent->key);
		
		if (zend_hash_next_index_insert(Z_ARRVAL_P(FORP_G(dump)), (void *) &t, sizeof(zval *), NULL) == FAILURE) {
		    return;
        	}
	}
}
/* }}} */

/* {{{ forp_dump
 */
ZEND_FUNCTION(forp_dump)
{
	if (FORP_G(enable)) {
		if(!FORP_G(dump)) {
  			forp_end(FORP_G(main) TSRMLS_CC);
			forp_stack_dump();
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

/* {{{ ZEND_MODULE_POST_ZEND_DEACTIVATE_D
 */
ZEND_MODULE_POST_ZEND_DEACTIVATE_D(forp)
{
	TSRMLS_FETCH();

	FORP_G(nesting_level) = 0;
	FORP_G(current_node) = NULL;

	// Freeing
        if(FORP_G(stack)) {
                int i;
                for (i = 0; i < FORP_G(stack_len); ++i) {
			efree(FORP_G(stack)[i]);
                }
                if (i) efree(FORP_G(stack));
        }
	FORP_G(stack_len) = 0;
        FORP_G(stack) = NULL;

        if(FORP_G(dump)) zval_ptr_dtor(&FORP_G(dump));
        FORP_G(dump) = NULL;

	return SUCCESS;
}
/* }}} */

/* {{{ forp_stack_dump_cli_node
 */
void forp_stack_dump_cli_node(forp_node *node TSRMLS_DC)
{
        int j;

        if (strstr(FORP_SKIP,node->function.function)) {
                return;
        }

        php_printf("[time:%09.0f] [memory:%09d] ", node->time, node->mem);
        for (j = 0; j < node->level; ++j) {
                if (j == node->level - 1) php_printf(" └── ");
                else php_printf(" |   ");
        }
        if(node->function.class) php_printf("%s::", node->function.class);
        php_printf("%s (%s)%s", node->function.function, node->function.filename, PHP_EOL);
}
/* }}} */

/* {{{ forp_stack_dump_cli
 */
void forp_stack_dump_cli()
{
        int i;
        php_printf("-----------------------------------------------------------------------------------------------------------%s", PHP_EOL);
        for (i = 0; i < FORP_G(stack_len); ++i) {
                forp_stack_dump_cli_node(FORP_G(stack)[i] TSRMLS_CC);
        }
        php_printf("-----------------------------------------------------------------------------------------------------------%s", PHP_EOL);
}
/* }}} */

/* {{{ forp_print
 */
ZEND_FUNCTION(forp_print)
{
	if(FORP_G(enable)) {
		forp_end(FORP_G(main) TSRMLS_CC);
        	forp_stack_dump_cli();
	} else {
		php_error_docref(
			NULL TSRMLS_CC, 
			E_NOTICE,
            		"forp_print() has no effect when forp_enable is turned off."
		);
	}	
}
/* }}} */
