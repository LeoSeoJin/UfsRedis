# UfsRedis

UfsRedis是将master-slave架构的Redis修改为client/server架构，
并额外增加了对复制并查集数据结构的支持。用户可以使用UfsRedis
来开发利用到等价关系的应用，例如Jigasaw Puzzle Game， 
Planner Point Location等等。

## Installation - 安装

### Requirements - 必要条件
* Linux系统
* gcc编译器

### Steps
1. 下载地址：[https://github.com/LeoSeoJin/UfsRedis](https://github.com/LeoSeoJin/UfsRedis)，下载最新稳定版本。
2. 编译UfsRedis里的文件

	$ make
	
   编译结束后`src`文件下会出现redis服务程序`redis-server`,还有用于测试的客户端程序`redis-cli`.
  
3. 编译成功后，进入`src`文件夹，进行UfsRedis安装

	$ make install

   当然，你也可以选择在一开始就直接进入`src`文件夹执行`make install`命令。
   
   安装完成之后，默认在`/usr/bin`目录下会生成redis服务程序`redis-server`，
   还有用于测试的客户端程序`redis-cli`，如果你想要安装到指定目录可以执行加上
   指定目录选项（例如‘/usr/local/redis’）：
   
   $ make install PREFIX=/usr/local/redis
  
## Deployment - 部署

## Development - 开发
