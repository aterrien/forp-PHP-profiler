# Introduction #

forp is a lightweight PHP extension which provides PHP profile datas.

Summary of features :
- measurement of time and allocated memory for each function
- CPU usage
- file and line number of the function call
- caption of functions
- grouping of functions
- aliases of functions (useful for anonymous functions)

forp is non intrusive, it provides PHP annotations to do its work.

# Simple (almost the most complicated) example #

Example :
```php
<?php
// first thing to do, enable forp profiler
forp_start();

// here, our PHP code we want to profile
function foo()
{
    echo 'Hello world !';
};
foo();

// stop forp buffering
forp_end();

// get the stack as an array
$profileStack = forp_dump();

print_r($profileStack);
```

Result :
```
foo = Hello world !
forp stack =
Array
(
    [utime] => 0
    [stime] => 0
    [stack] => Array
        (
            [0] => Array
                (
                    [file] => /home/anthony/phpsrc/php-5.3.8/ext/forp/README.php
                    [function] => {main}
                    [usec] => 94
                    [pusec] => 6
                    [bytes] => 524
                    [level] => 0
                )

            [1] => Array
                (
                    [file] => /home/anthony/phpsrc/php-5.3.8/ext/forp/README.php
                    [function] => foo
                    [lineno] => 10
                    [usec] => 9
                    [pusec] => 6
                    [bytes] => 120
                    [level] => 1
                    [parent] => 0
                )

        )

)
```

# Example with annotations #

Example :
```php
<?php
// first thing to do, enable forp profiler
forp_start();

/**
 * here, our PHP code we want to profile
 * with annotations
 *
 * @ProfileGroup("Test")
 * @ProfileCaption("Foo #1")
 * @ProfileAlias("foo")
 */
function fooWithAnnotations($bar)
{
    return 'Foo ' . $bar;
}

echo "foo = " . fooWithAnnotations("bar") . "\n";

// stop forp buffering
forp_end();

// get the stack as an array
$profileStack = forp_dump();

echo "forp stack = \n";
print_r($profileStack);
```

Result :
```
foo = Foo bar
forp stack =
Array
(
    [utime] => 0
    [stime] => 0
    [stack] => Array
        (
            [0] => Array
                (
                    [file] => /home/anthony/phpsrc/php-5.3.8/ext/forp/READMEforp.php
                    [function] => {main}
                    [usec] => 113
                    [pusec] => 6
                    [bytes] => 568
                    [level] => 0
                )

            [1] => Array
                (
                    [file] => /home/anthony/phpsrc/php-5.3.8/ext/forp/READMEforp.php
                    [function] => foo
                    [lineno] => 41
                    [groups] => Array
                        (
                            [0] => Test
                        )

                    [caption] => Foo bar
                    [usec] => 39
                    [pusec] => 24
                    [bytes] => 124
                    [level] => 1
                    [parent] => 0
                )

        )

)
```

# php.ini options #

- forp.max_nesting_level : default 50
- forp.no_internals : default 0

# forp PHP API #

- forp_start(flags*) : start forp collector
- forp_end() : stop forp collector
- forp_dump() : return stack as flat array
- forp_print() : display forp stack (SAPI CLI)

## forp_start() flags ##

- FORP_FLAG_CPU : activate collect of time
- FORP_FLAG_MEMORY : activate collect of memory usage
- FORP_FLAG_CPU : retrieve the cpu usage
- FORP_FLAG_CAPTION : enable caption handler
- FORP_FLAG_ALIAS : enable alias handler
- FORP_FLAG_GROUPS : enable groups handler
- FORP_FLAG_HIGHLIGHT : enable HTML highlighting

## forp_dump() ##

forp_dump() provides an array composed of :

- global fields : utime, stime ...
- stack : a flat PHP array of stack entries.

Global fields :

- utime : CPU used for user function calls
- stime : CPU used for system calls

Fields of a stack entry :

- file : file of the call
- lineno : line number of the call
- class : Class name
- function : function name
- groups : list of associated groups
- caption : caption of the function
- usec : function time (without the profiling overhead)
- pusec : inner profiling time (without executing the function)
- bytes : memory usage of the function
- level : depth level from the forp_start call
- parent : parent index (numeric)

## forp_json() ##

Prints stack as JSON string directly on stdout.
This is the fastest method in order to send the stack to a JSON compatible client.

See forp_dump() for its structure.

## forp_inspect() ##

forp_inspect('symbol', $symbol) will output a detailed representation of a variable in the forp_dump() result.

Usage :
```php
$var = array(0 => "my", "strkey" => "inspected", 3 => "array");
forp_inspect('var', $var);
print_r(forp_dump());
```

Result :
```php
Array
(
    [utime] => 0
    [stime] => 0
    [inspect] => Array
        (
            [var] => Array
                (
                    [type] => array
                    [size] => 3
                    [value] => Array
                        (
                            [0] => Array
                                (
                                    [type] => string
                                    [value] => my
                                )

                            [strkey] => Array
                                (
                                    [type] => string
                                    [value] => inspected
                                )

                            [3] => Array
                                (
                                    [type] => string
                                    [value] => array
                                )

                        )

                )

        )

)
```

## Available annotations ##

- @ProfileGroup

Sets groups that function belongs.

```php
/**
 * @ProfileGroup("data loading","rendering")
 */
function exec($query) {
    /* ... */
}
```

- @ProfileCaption

Adds caption to functions. Caption string may contain references (#<param num>) to parameters of the function.

```php
/**
 * @ProfileCaption("Find row for pk #1")
 */
public function findByPk($pk) {
    /* ... */
}
```

- @ProfileAlias

Gives an alias name to a function. Useful for anonymous functions

```php
/**
 * @ProfileAlias("MyAnonymousFunction")
 */
$fn = function() {
    /* ... */
}
```

- @ProfileHighlight

Adds a frame around output generated by the function.

```php
/**
 * @ProfileHighlight("1")
 */
function render($datas) {
    /* ... */
}
```

# Demos / UI #

forp provides a structure so simple that it's easy to make your own UI.

But, if you are looking for a user interface with a lot of features, we've made a rich JavaScript client :
[forp-ui](https://github.com/aterrien/forp-ui/)

You can see forp in action with the fastest PHP Frameworks :
- [Beaba](http://87.117.252.103/beaba/hello) (https://github.com/ichiriac/beaba-light)
- [Slim](http://87.117.252.103/slim/hello) (https://github.com/codeguy/Slim)
- [Silex](http://87.117.252.103/silex/hello) (https://github.com/fabpot/Silex)
- [Laravel](http://87.117.252.103/laravel/hello) (https://github.com/laravel/laravel)

![groups](https://raw.github.com/aterrien/forp-ui/master/doc/ui-consolas-groups.png)

# Build status #

[![Build Status](https://secure.travis-ci.org/aterrien/forp-PHP-profiler.png)](http://travis-ci.org/aterrien/forp-PHP-profiler)

# Installation, requirements #

## Linux Install ##

* requirement (php5-dev)

```sh
apt-get install php5-dev
```

* download

current release
```sh
wget https://github.com/aterrien/forp/archive/1.0.1.tar.gz
tar -xvzf 1.0.1.tar.gz
cd 1.0.1
```

or master (unstable)
```sh
git clone https://github.com/aterrien/forp
cd forp
```

* install

```sh
phpize
./configure
make && make install
```

* php.ini

```sh
extension=forp.so
```

... you can restart your PHP server.

## Tested OS and platforms ##

### Apache ###

Apache/2.2.16 (Debian)

PHP 5.3.8 (cli) (built: Sep 25 2012 22:55:18)

### Nginx / php-fpm ###

nginx version: nginx/1.2.6

PHP 5.3.21-1~dotdeb.0 (fpm-fcgi)
PHP 5.4.10-1~dotdeb.0 (fpm-fcgi)

### Cloud / AWS ###

Centos 6.3 (AMI)

Apache/2.2.15 (Unix)

PHP 5.4.13 (cli) (built: Mar 15 2013 11:27:51)

# Contributors #

[Anthony Terrien](https://github.com/aterrien/),
[Ioan Chiriac](https://github.com/ichiriac/),
[Alexis Okuwa](https://github.com/wojons/)

# MIT License

Copyright (C) 2013 Anthony Terrien

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
