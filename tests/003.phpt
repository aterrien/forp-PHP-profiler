--TEST--
forp_print() function - basic test
--SKIPIF--
<?php if (!extension_loaded("forp")) print "skip"; ?>
--FILE--
<?php
ob_start();
forp_start();
var_dump('test');
forp_print();
$res = ob_get_clean();
echo strpos($res,'--------------------------------------------------------------------------------')>0;
echo strpos($res,'time:')>0;
echo strpos($res,'mem:')>0;
echo strpos($res,'{main}')>0;
?>
--EXPECT--
1111
