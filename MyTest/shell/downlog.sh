#!/bin/sh

ip1="139.224.130.80"
ip2="47.100.34.153"
ip3="47.99.201.21"

scp root@$ip1:/usr/local/redis/log/*.log /home/xue/log
scp root@$ip2:/usr/local/redis/log/*.log /home/xue/log
scp root@$ip3:/usr/local/redis/log/*.log /home/xue/log

#echo "download log files from server $1"
