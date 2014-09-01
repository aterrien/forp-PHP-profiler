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

#include "forp_string.h"
#include "forp_annotation.h"

/* {{{ forp_annotation_args
 *
 * Parses args of an annotation
 *
 * @param char* str
 * @param char*** args
 * @param int* args_count
 * @return void
 */
void forp_annotation_args(char *str, char ***args, int *args_count TSRMLS_DC) {
    int esc = 0, buf = 0, i = 0, j = 0;
    char *ex;

    *args_count = 0;
    if(strlen(str) > 0) {
        ex = malloc(sizeof(char*));
        while(str[i] != '\0') {
            if(!esc) {
                if(str[i] == '\\') {
                    esc = 1;
                }
                if((!esc) && str[i] == '"') {
                    if(buf && j > 0) {
                        ex = realloc(ex, sizeof(char*) * (j + 1));
                        ex[j] = '\0';
                        (*args)[(*args_count)] = strdup(ex);
                        (*args_count)++;
                        memset(ex, 0, sizeof(char*));
                        j = 0;
                    }
                    buf = !buf;
                } else {
                    if(buf) {
                        ex = realloc(ex, sizeof(char*) * (j + 1));
                        ex[j] = str[i];
                        j++;
                    }
                }
            } else {
                if(buf) {
                    ex = realloc(ex, sizeof(char*) * (j + 1));
                    ex[j] = str[i];
                    j++;
                }
                esc = 0;
            }
            i++;
        }

        //if(buf) {
            //ex[j] = '\0';
            //printf("NOT CLOSED \"!:|%s|\n", ex);
        //}

        free(ex);
    }
}
/* }}} */

/* {{{ forp_annotation_tok
 *
 * Handles annotations in doc_comment : @<tag>(<params>)<\n>
 *
 * @param char* doc_comment
 * @param char* tag
 * @return char*
 */
char *forp_annotation_tok(const char *doc_comment, char *tag TSRMLS_DC) {
    char *v = NULL, *v_search = NULL, *t_start = NULL, *tmp = NULL, *eot = NULL;
    unsigned int v_start, v_end;

    v_search = malloc(sizeof(char*) * (strlen(tag) + 3));
    if(v_search) {
        sprintf(v_search, "@%s(", tag);
        t_start = strstr(doc_comment, v_search);
        if (t_start) {
            v_start = t_start - doc_comment + strlen(v_search);
            tmp = strndup(doc_comment + v_start, strlen(doc_comment));
            eot = strstr(tmp, ")");
            v_end = eot - tmp;
            v = strndup(doc_comment + v_start, v_end);
        }
        free(v_search);
    }

    return v;
}
/* }}} */

/* {{{ forp_annotation_string
 *
 * Retrieves string arg in an annotation
 *
 * @param char* doc_comment
 * @param char* tag
 * @return char*
 */
char *forp_annotation_string(const char *doc_comment, char *tag TSRMLS_DC) {
    int args_count;
    char *v = NULL;
    char **args = malloc(sizeof(char*));
    char *args_str = forp_annotation_tok(doc_comment, tag TSRMLS_CC);

    if(args_str != NULL) {
        forp_annotation_args(args_str, &args, &args_count TSRMLS_CC);
        if(args_count > 0) {
            v = strdup(args[0]);
        }
    }
    free(args);

    return v;
}
/* }}} */

/* {{{ forp_annotation_array
 *
 * Retrieves args array in an annotation
 *
 * @param char* doc_comment
 * @param char* tag
 * @param char*** args
 * @param int* args_count
 * @return void
 */
void forp_annotation_array(const char *doc_comment, char *tag, char ***args, int *args_count TSRMLS_DC) {
    char *args_str = forp_annotation_tok(doc_comment, tag TSRMLS_CC);
    if(args_str != NULL) {
        forp_annotation_args(args_str, args, args_count TSRMLS_CC);
    }
}
/* }}} */