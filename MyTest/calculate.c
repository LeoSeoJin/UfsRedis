#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hiredis/hiredis.h>

char *ip[] = {"127.0.0.1","127.0.0.1","127.0.0.1","127.0.0.1","127.0.0.1","127.0.0.1","127.0.0.1"};
char *port[] = {"6379","6380","6381","6382","6383","6384","6385","6386"};
/*
 *argv[1]:client num
*/
int main(int argc, char *arg[]) {
    int client_num = atoi(arg[1]);
    sds memory = sdsempty();
    sds op = sdsempty();
    double total_time_union = 0;
    double total_time_split = 0;

	redisContext *conn_server = redisConnect(ip[0],atoi(port[0]));
	if(conn_server->err) printf("Connection error: %s\n",conn_server->errstr);
   
    char *command_memory = "info memory";
    char *command_time = "info commandstats";

    int totlines,num,t;
    sds *lines, *argv, *c;
    redisReply* reply = NULL;
    
	reply = (redisReply*)redisCommand(conn_server,command_memory);	
	lines = sdssplitlen(reply->str,strlen(reply->str),"\n",1,&totlines);
    lines[2] = sdstrim(lines[2]," \t\r\n");	
	
	argv = sdssplitlen(lines[2],sdslen(lines[2]),":",1,&num);
	memory = sdscat(memory,argv[1]);

    //printf("memory: server: %s\n",memory);
    
	sdsfreesplitres(lines,totlines);
	sdsfreesplitres(argv,num);

	freeReplyObject(reply);
	redisFree(conn_server);
		
	for (int i = 1; i <= client_num; i++) {
    	conn_server = redisConnect(ip[i],atoi(port[i]));
    	if(conn_server->err) printf("Connection error: %s\n",conn_server->errstr);	
	
	    reply = (redisReply*)redisCommand(conn_server,command_memory);	
    	lines = sdssplitlen(reply->str,strlen(reply->str),"\n",1,&totlines);    	
        lines[2] = sdstrim(lines[2]," \t\r\n");
	    
	    argv = sdssplitlen(lines[2],sdslen(lines[2]),":",1,&num);
    	memory = sdscat(memory,"+");
    	memory = sdscat(memory,argv[1]);
    
        //printf("client %s: %s\n",port[i],memory);
   	
    	sdsfreesplitres(lines,totlines);
    	sdsfreesplitres(argv,num); 
        
        freeReplyObject(reply);
	    reply = (redisReply*)redisCommand(conn_server,command_time);	
    	lines = sdssplitlen(reply->str,strlen(reply->str),"\n",1,&totlines);       	
    	
    	for (int j = 0; j < totlines; j++) {
    	    if (strstr(lines[j],"union")) {
    	        lines[j] = sdstrim(lines[j]," \t\r\n");
    	        argv = sdssplitlen(lines[j],sdslen(lines[j]),",",1,&num);
    	        if (i == 1) {
    	            op = sdscat(op,"union ");
    	            c = sdssplitlen(argv[0],sdslen(argv[0]),"=",1,&t);
        	        op = sdscat(op,c[1]);
        	        op = sdscat(op," ");
        	        sdsfreesplitres(c,t);    	            
    	        }
    	        c = sdssplitlen(argv[2],sdslen(argv[2]),"=",1,&t);
    	        total_time_union += atof(c[1]);
    	        sdsfreesplitres(c,t);
    	        sdsfreesplitres(argv,num);
    	    } else if (strstr(lines[j],"split")){
    	        lines[j] = sdstrim(lines[j]," \t\r\n");
    	        argv = sdssplitlen(lines[j],sdslen(lines[j]),",",1,&num);
    	        if (i == 1) {
    	            op = sdscat(op,"split ");
    	            c = sdssplitlen(argv[0],sdslen(argv[0]),"=",1,&t);
        	        op = sdscat(op,c[1]);
        	        op = sdscat(op," ");
        	        sdsfreesplitres(c,t);    	            
    	        }    	        
    	        c = sdssplitlen(argv[2],sdslen(argv[2]),"=",1,&t);
    	        total_time_split += atof(c[1]);
    	        sdsfreesplitres(c,t);
    	        sdsfreesplitres(argv,num);    	    
    	    } else {
    	        continue;
    	    }
    	}
    	
	    freeReplyObject(reply);
    	redisFree(conn_server);    	  	        
	}

	printf("memory: %s\n",memory);
	printf("op: %s\n",op);
	printf("average time: union: %lfms split: %lfms\n",(total_time_union/client_num-delay)/1000,(total_time_split/client_num-delay)/1000);
	
	sdsfree(memory);
	sdsfree(op);
	
	return 0;
}
