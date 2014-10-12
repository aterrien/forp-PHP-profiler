--TEST--
forp_dump() function
--SKIPIF--
<?php if (!extension_loaded("forp")) print "skip"; ?>
--FILE--
<?php
forp_start();
function foo() {
	print "bar";
}
foo();
$dump = forp_dump();
print count($dump)>1;
?>
--EXPECT--
bar1
