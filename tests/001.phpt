--TEST--
forp_enable() function - basic test
--SKIPIF--
<?php if (!extension_loaded("forp")) print "skip"; ?>
--FILE--
<?php
var_dump(forp_enable());
?>
--EXPECT--
NULL
