#!/bin/sh

redis-server /usr/local/redis/etc/6379.conf
if [ $1 -ge 1 ]; then redis-server /usr/local/redis/etc/6380.conf; fi
if [ $1 -ge 2 ]; then redis-server /usr/local/redis/etc/6381.conf; fi
if [ $1 -ge 3 ]; then redis-server /usr/local/redis/etc/6382.conf; fi
if [ $1 -ge 4 ]; then redis-server /usr/local/redis/etc/6383.conf; fi
if [ $1 -ge 5 ]; then redis-server /usr/local/redis/etc/6384.conf; fi
if [ $1 -ge 6 ]; then redis-server /usr/local/redis/etc/6385.conf; fi
if [ $1 -ge 7 ]; then redis-server /usr/local/redis/etc/6386.conf; fi
if [ $1 -ge 8 ]; then redis-server /usr/local/redis/etc/6387.conf; fi
if [ $1 -ge 9 ]; then redis-server /usr/local/redis/etc/6388.conf; fi
if [ $1 -ge 10 ]; then redis-server /usr/local/redis/etc/6389.conf; fi
if [ $1 -ge 11 ]; then redis-server /usr/local/redis/etc/6390.conf; fi
if [ $1 -ge 12 ]; then redis-server /usr/local/redis/etc/6391.conf; fi
echo "server started successful!"

