<?php 

// Start profiler
forp_start();

/**
 * @ProfileGroup("cat1", "cat2")
 */
function hanoi($plates, $from, $to) { 
    while($plates > 0) { 
        $using = 6 - ($from + $to); 
        hanoi(--$plates, $from, $using); 
        //print "Move plate from $from to $to<br>"; 
        $from = $using; 
    } 
} 
hanoi(5, 1, 3);

// Stop profiler
forp_end();
// Get JSON and save it into file
forp_json_google_tracer("/tmp/output_".mt_rand().".json");
?>
