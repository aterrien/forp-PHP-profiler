dnl $Id$
dnl config.m4 for extension forp

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(forp, for forp support,
dnl Make sure that the comment is aligned:
dnl [  --with-forp             Include forp support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(forp, whether to enable forp support,
Make sure that the comment is aligned:
[  --enable-forp           Enable forp support])

if test "$PHP_FORP" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-forp -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/php_forp.h /include/forp.h"  # you most likely want to change this
  dnl if test -r $PHP_FORP/$SEARCH_FOR; then # path given as parameter
  dnl   FORP_DIR=$PHP_FORP
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for forp files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       FORP_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$FORP_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the forp distribution])
  dnl fi

  dnl # --with-forp -> add include path
  dnl PHP_ADD_INCLUDE($FORP_DIR/include)

  dnl # --with-forp -> check for lib and symbol presence
  dnl LIBNAME=forp # you may want to change this
  dnl LIBSYMBOL=forp # you most likely want to change this

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $FORP_DIR/lib, FORP_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_FORPLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong forp lib version or lib not found])
  dnl ],[
  dnl   -L$FORP_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(FORP_SHARED_LIBADD)

  PHP_NEW_EXTENSION(forp, php_forp.c forp.c, $ext_shared)
fi
