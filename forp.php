<?php
$br = (php_sapi_name() == "cli")? "\n":"<br>\n";
$module = 'forp';
if(!extension_loaded($module)) {
    dl($module . '.' . PHP_SHLIB_SUFFIX);
}
$functions = get_extension_funcs($module);
echo "Functions available in the test extension:". $br;
foreach($functions as $func) {
    echo $func.$br;
}
echo $br;

// testing forp functions
echo '- Enable'.$br;
forp_enable();

function fibo( $x ) {
    if ( $x < 2) {
        return 1;
    } else {
        return fibo($x - 1) + fibo($x - 2);
    }
}

for( $i = 1; $i < 10; $i++) {
    printf(
        'fibo(%1$s) = %2$s'.$br,
        $i, fibo($i)
    );
}
echo '- Dump'.$br;
$dump = forp_dump();
print_r( $dump );
echo '- Print'.$br;
forp_print();
sleep(10);