#!/bin/sh

redis-cli -h 127.0.0.1 -p 6379 shutdown

if [ $1 -ge 1 ]; then redis-cli -h 127.0.0.1 -p 6380 shutdown ;fi
if [ $1 -ge 2 ]; then redis-cli -h 127.0.0.1 -p 6381 shutdown ;fi
if [ $1 -ge 3 ]; then redis-cli -h 127.0.0.1 -p 6382 shutdown ;fi
if [ $1 -ge 4 ]; then redis-cli -h 127.0.0.1 -p 6383 shutdown ;fi
if [ $1 -ge 5 ]; then redis-cli -h 127.0.0.1 -p 6384 shutdown ;fi
if [ $1 -ge 6 ]; then redis-cli -h 127.0.0.1 -p 6385 shutdown ;fi
if [ $1 -ge 7 ]; then redis-cli -h 127.0.0.1 -p 6386 shutdown ;fi
if [ $1 -ge 8 ]; then redis-cli -h 127.0.0.1 -p 6387 shutdown ;fi
if [ $1 -ge 9 ]; then redis-cli -h 127.0.0.1 -p 6388 shutdown ;fi
if [ $1 -ge 10 ]; then redis-cli -h 127.0.0.1 -p 6389 shutdown ;fi
if [ $1 -ge 11 ]; then redis-cli -h 127.0.0.1 -p 6390 shutdown ;fi
if [ $1 -ge 12 ]; then redis-cli -h 127.0.0.1 -p 6391 shutdown ;fi

