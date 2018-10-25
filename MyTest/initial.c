#include <stdio.h>
#include <string.h>
#include <hiredis/hiredis.h>

/*argument: argv[0]: initial value of ufs
*/
int main(int argc, char *argv[]) { 
	redisContext *conn_server = redisConnect("127.0.0.1",6379);
	if(conn_server->err) printf("Connection error: %s\n",conn_server->errstr);

	redisReply* reply = NULL;	
	//reply = (redisReply*)redisCommand("auth jx062325");
	//freeReplyObject(reply);
	
	char command[100] = "uinit ";
	strcat(command,"group1 ");
	strcat(command,argv[1]);
	reply = (redisReply*)redisCommand(conn_server,command);
	
	freeReplyObject(reply);
	redisFree(conn_server);
	
	return 0;
}
