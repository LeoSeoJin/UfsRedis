# UfsRedis

Redis是当前比较热门的NOSQL系统之一，它是一个key-value存储系统。它支持存储的value类型相对更多，
包括string、list、set、zset和hash。这些数据类型都支持push/pop、add/remove及取交集
并集和差集及更丰富的操作。在此基础上，Redis支持各种不同方式的排序。

Redis数据都是缓存在计算机内存中，同时也提供了持久化机制。它会周期性的把
更新的数据写入磁盘或者把修改操作写入追加的记录文件，实现数据的持久化。

Redis支持Master-Slave架构，只有Master服务器能够修改数据，而Slave服务器只能读取数据。
同时Redis提供的同步机制保证了数据在所有服务器上是最终一致的。

UfsRedis是对Redis的扩展，主要包括两个方面：
* 结构调整：原本的Redis是Master-Slave架构，现将其Redis修改为Client/Sserver架构。Master
  服务器和Slave服务器都可以进行数据更新，包括写入新数据和修改旧数据。
* 类型扩充：在原本支持的数据类型之外，增加了对并查集数据结构的支持。提供了对应的命令
  实现对并查集的创建，修改。

用户可以使用UfsRedis来开发利用到等价关系的应用，例如Jigasaw Puzzle Game， 
Planner Point Location等等。


## Installation - 安装

### Requirements - 必要条件
* Linux系统
* gcc编译器

### Steps
1. 下载地址：[https://github.com/LeoSeoJin/UfsRedis](https://github.com/LeoSeoJin/UfsRedis)，下载最新稳定版本。
2. 编译UfsRedis里的文件
   ```
   $ make
   ```	
   编译结束后`src`文件下会出现两个命令`redis-server`和`redis-cli`，这两个命令分别用于启动
   服务器和客户端。
  
3. 编译成功后，进入`src`文件夹，进行UfsRedis安装
   ``` 
   $ make install
   ```
   当然，你也可以选择在一开始就直接进入`src`文件夹执行`make install`命令。 `make install`命令的执行结果等价于:   
   ```
   $ mv src/redis-server src/redis-cli /usr/local/bin
   ```
   
   安装完成之后，默认在`/usr/local/bin`目录下生成启动redis服务器的命令`redis-server`，
   和启动客户端命令`redis-cli`，如果你想要安装到指定目录可以执行加上
   指定目录选项（例如‘/usr/local/redis’）：
   ```
   $ make install PREFIX=/usr/local/redis
   ```

## Deployment - 部署

假设当前需要构建一个分布式系统，包括1个Server和3个Client。

1. 创建配置文件

   每个服务器对应一个配置文件，将所有服务器的配置文件放置到`/usr/local/redis/etc`目录下：   
   ```
   $ mkdir /usr/local/redis/etc
   $ mv redis.conf /usr/local/redis/etc
   $ cp redis.conf 6379.conf              //6379.conf是Server的配置文件
   $ cp redis.conf 6380.conf              //6380.conf，6381.conf，6382.conf分别是三个Client的配置文件
   $ cp redis.conf 6381.conf	
   $ cp redis.conf 6382.conf
   ```
   
2. 启动Server服务器
   
   假设使用命令：
   ```
   $ redis-server
   ```
   这样的话，服务器在前台运行，也就是说，一旦关闭Linux当前会话，服务器也立即关闭，所以
   需要在后台运行服务器。
   
   为了使得服务器在后台运行，需将配置文件中`daemonize`选项的从默认值`no`
   修改为`yes`。
   
   另外还需要修改配置文件中的下面几个选项：   
   ```
   % 'port=6379'
   % 'pidfile=/var/run/redis_6379.pid'
   % 'logfile=/usr/local/redis/6379.log'
   % 'dbfilename=6379.rdb'
   % 'dir=/usr/local/redis'
   ```
    
   使用下面的命令来启动Server服务器：
   ```
   $ redis-server /usr/loca/redis/etc/6379.conf
   ```

3. 启动Client服务器
   
   这里以端口号为6380的Client服务器为例。首先同样需要修改配置文件中的上述
   几个选项内容：  
   ```
   % 'port = 6380'
   % 'pidfile = /var/run/redis_6380.pid'
   % 'logfile = /usr/local/redis/6380.log'
   % 'dbfilename = 6380.rdb'
   % 'dir = /usr/local/redis'
   ```  
  
   除此以外，为了能够让Client服务器同样能够写入和修改数据，需要修改选项： 
   ```
   % 'slaveof = 127.0.0.1 6379'
   ``` 
   
   使用下面的命令来启动Client服务器：
   ```
   $ redis-server /usr/loca/redis/etc/6380.conf
   ```

4. 启动客户端
    
   每个客户端连接到一个指定的服务器，一个服务器可以连接多个客户端。使用下面的命令来启动客户端：   
   ```
   $ redis-cli -h 127.0.0.1 -p 6379    //启动一个连接到Server服务器的客户端
   ```
   
5. 关闭客户端
	
   可以使用`redis-cli`命令来关闭：  
   ```
   $ redis-cli -h 127.0.0.1 -p 6379 shutdown   //关闭连接到Server服务器的客户端
   ``` 
   
   因为Redis可以妥善处理SIGTERM信号，所以直接kill-9也是可以的   
   ```
   $ kill -9 pid      //pid是客户端的进程号
   ```    

## Development - 开发
