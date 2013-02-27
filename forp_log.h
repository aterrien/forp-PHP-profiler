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

#ifndef FORP_LOG_H
#define FORP_LOG_H

#define FORP_LOG_DEPTH    1

#include "php.h"

#ifdef ZTS
#include "TSRM.h"
#endif

typedef struct forp_var_t {
    char *type;
    char *name;
    char *key;
    char *property;
    char *value;
    char *class;
    struct forp_var_t **arr;
    int arr_len;
} forp_var_t;

forp_var_t *forp_zval_var(forp_var_t *v, zval *expr, int depth TSRMLS_DC);

zval **forp_find_symbol(char* name TSRMLS_DC);

void forp_inspect(zval *expr TSRMLS_DC);

#endif  /* FORP_LOG_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */