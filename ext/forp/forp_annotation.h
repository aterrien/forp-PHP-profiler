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

#ifndef FORP_ANNOTATION_H
#define FORP_ANNOTATION_H

#include "php.h"

#ifdef ZTS
#include "TSRM.h"
#endif

void forp_annotation_args(char *str, char ***args, int *args_count TSRMLS_DC);

char *forp_annotation_tok(const char *doc_comment, char *tag TSRMLS_DC);

char *forp_annotation_string(const char *doc_comment, char *tag TSRMLS_DC);

void forp_annotation_array(const char *doc_comment, char *tag, char ***args, int *args_count TSRMLS_DC);

#endif  /* FORP_ANNOTATION_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */