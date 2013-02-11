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

#ifdef ZTS
#include "TSRM.h"
#endif

/* {{{ forp_substr_replace
 *
 * @param char* subject
 * @param uint start
 * @param uint len
 * @param char* replace
 * @return char*
 */
char *forp_substr_replace(char *subject, char *replace, unsigned int start, unsigned int len) {
   char *ns = NULL;
   size_t size;
   if (
        subject != NULL && replace != NULL
        && start >= 0 && len > 0
   ) {
      size = strlen(subject);
      ns = malloc(sizeof(*ns) * (size - len + strlen(replace) + 1));
      if(ns) {
         memcpy(ns, subject, start);
         memcpy(&ns[start], replace, strlen(replace));
         memcpy(&ns[start + strlen(replace)], &subject[start + len], size - len - start + 1);
      }
      subject = strdup(ns);
      free(ns);
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
    char *found;
    while(found = strstr(subject, search)) {
        subject = forp_substr_replace(subject, replace, found - subject, strlen(search));
    }
    return subject;
}
/* }}} */