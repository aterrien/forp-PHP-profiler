<?php 

// Start profiler
forp_start();

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
$json = forp_json_google_tracer();
file_put_contents(__DIR__."/output-hanoi.json", $json);
?>
