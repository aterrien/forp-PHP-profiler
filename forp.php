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
forp_start();

/**
 * @ProfileGroup("fibo")
 * @ProfileCaption("fibo of #1")
 */
function fibo( $x ) {
    if ( $x <= 1) {
        return $x;
    } else {
        return fibo($x - 1) + fibo($x - 2);
    }
}

for( $i = 1; $i < 20; $i++) {
    printf(
        'fibo(%1$s) = %2$s'.$br,
        $i, fibo($i)
    );
}
echo '- Dump'.$br;
$dump = forp_dump();
print_r( $dump['stack'][0] );
echo '- Print'.$br;
forp_print();