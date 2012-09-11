!under development!

on : PHP 5.3.8,  Debian Squeeze

[![Build Status](https://secure.travis-ci.org/aterrien/forp.png?branch=master)](http://travis-ci.org/aterrien/forp)


# Introduction #

forp is a lightweight PHP extension which provides CPU and memory profiling datas.

## PHP functions ##
- forp_enable() : starts forp
- forp_dump() : Array - forp stack
- forp_print() : displays forp stack (CLI)

## Install ##

* make
```make
git clone https://github.com/aterrien/forp
cd forp
phpize
./configure
make
make test
make install
```

* php.ini
```phpini
extension=forp.so
```
