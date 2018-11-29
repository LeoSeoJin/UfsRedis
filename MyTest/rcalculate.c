#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hiredis/hiredis.h>

char *ip[] = {"139.224.130.80",
              "47.100.34.153","47.100.34.153","47.100.34.153","47.100.34.153","47.100.34.153","47.100.34.153",
              "47.99.201.21","47.99.201.21","47.99.201.21","47.99.201.21","47.99.201.21","47.99.201.21"};
int port[] = {6379,6380,6381,6382,6383,6384,6385,6386,6387,6388,6389,6390,6391};
char *key_list[] = {"group1","group2","group3","group4"};

/*
 *arg[1]:client num
 *arg[2]:key of ufs
*/
int main(int argc, char *arg[]) {
    int client_num = atoi(arg[1]);
    sds memory = sdsempty();
    sds op = sdsempty();
    double total_time_union = 0;
    double total_time_split = 0;
   	double mem = 0;
   	char mem_double[30];
    sds ufs_list = sdsempty();  	
    int key = atoi(arg[2])-1;
    
	redisContext *conn_server = redisConnect(ip[0],port[0]);
	if(conn_server->err) printf("Connection error: %s\n",conn_server->errstr);

	redisReply* reply = NULL;	
	reply = (redisReply*)redisCommand(conn_server,"auth jx062325");
	freeReplyObject(reply);
	    
    char *command_memory = "info memory";
    char *command_time = "info commandstats";
    sds command_ufs = sdsnew("umembers ");
    command_ufs = sdscat(command_ufs,key_list[key]);
    
    int totlines,num,t;
    sds *lines, *argv, *c,temp;
    
	reply = (redisReply*)redisCommand(conn_server,command_memory);	
	lines = sdssplitlen(reply->str,strlen(reply->str),"\n",1,&totlines);
    lines[2] = sdstrim(lines[2]," \t\r\n");	
	
	argv = sdssplitlen(lines[2],sdslen(lines[2]),":",1,&num);
	temp = sdsnew(argv[1]);
	if (strchr(temp,'M')) {
	    temp = sdstrim(temp,"M");
	    mem = atof(temp)-0.8;
	    memory = sdscat(memory,gcvt(mem,8,mem_double));
	    memory = sdscat(memory,"M");
	} else {
	    memory = sdscat(memory,argv[1]);
	}
	sdsfree(temp);
    //memory = sdscat(memory,argv[1]);
    
	sdsfreesplitres(lines,totlines);
	sdsfreesplitres(argv,num);

	freeReplyObject(reply);
	reply = (redisReply*)redisCommand(conn_server,command_time);	
	lines = sdssplitlen(reply->str,strlen(reply->str),"\n",1,&totlines);
    int total_op = 0;
    int op_union = 0;
    int op_split = 0;
    double avg_time_union = 0;
    double avg_time_split = 0;
    
    for (int j = 0; j < totlines; j++) {
        if (strstr(lines[j],"union")) {
            lines[j] = sdstrim(lines[j]," \t\r\n");
            argv = sdssplitlen(lines[j],sdslen(lines[j]),",",1,&num);
            c = sdssplitlen(argv[0],sdslen(argv[0]),"=",1,&t);
            total_op += atoi(c[1]);
            op_union = atoi(c[1]);
            
            sdsfreesplitres(c,t);
            sdsfreesplitres(argv,num);
        }
        if (strstr(lines[j],"split")) {
            lines[j] = sdstrim(lines[j]," \t\r\n");
            argv = sdssplitlen(lines[j],sdslen(lines[j]),",",1,&num);
            c = sdssplitlen(argv[0],sdslen(argv[0]),"=",1,&t);
            total_op += atoi(c[1]);
            op_split = atoi(c[1]);
            
            sdsfreesplitres(c,t);
            sdsfreesplitres(argv,num);
        }
   	}
    
    freeReplyObject(reply);
	redisFree(conn_server);
	int op_num,counter,i;
	
	int k = 0;
	int max_thread = 12;
	for (i = 1; i <= client_num; i++) {
    	op_num = 0;
    	counter = 0;
    	
    	k = (i <= client_num/2)?i:(max_thread/2+i-client_num/2);
    		
    	conn_server = redisConnect(ip[k],port[k]);
    	if(conn_server->err) printf("Connection error: %s\n",conn_server->errstr);		
        	
	    reply = (redisReply*)redisCommand(conn_server,command_memory);	
    	lines = sdssplitlen(reply->str,strlen(reply->str),"\n",1,&totlines);    	
        lines[2] = sdstrim(lines[2]," \t\r\n");
	    
	    argv = sdssplitlen(lines[2],sdslen(lines[2]),":",1,&num);
    	memory = sdscat(memory,"+");
    	
    	temp = sdsnew(argv[1]);
	    if (strchr(temp,'M')) {
    	    temp = sdstrim(temp,"M");
    	    mem = atof(temp)-0.8;
    	    memory = sdscat(memory,gcvt(mem,8,mem_double));
    	    memory = sdscat(memory,"M");
    	} else {
    	    memory = sdscat(memory,argv[1]);
    	}
    	sdsfree(temp);
    	//memory = sdscat(memory,argv[1]);

    	sdsfreesplitres(lines,totlines);
    	sdsfreesplitres(argv,num); 
        
        freeReplyObject(reply);
	    reply = (redisReply*)redisCommand(conn_server,command_time);	
    	lines = sdssplitlen(reply->str,strlen(reply->str),"\n",1,&totlines);       	
    	   	
    	for (int j = 0; j < totlines; j++) {
    	    if (strstr(lines[j],"union")) {
    	        lines[j] = sdstrim(lines[j]," \t\r\n");
    	        argv = sdssplitlen(lines[j],sdslen(lines[j]),",",1,&num);   	        
  	            
  	            c = sdssplitlen(argv[0],sdslen(argv[0]),"=",1,&t);
        	    op_num += atoi(c[1]);  
        	    counter++;	    
    	        if (i == 1) {
    	            op = sdscat(op,"union ");
        	        op = sdscat(op,c[1]);
        	        op = sdscat(op," ");        	            	            
    	        }
    	        sdsfreesplitres(c,t);
    	        
    	        c = sdssplitlen(argv[2],sdslen(argv[2]),"=",1,&t);
    	        total_time_union += atof(c[1]);
    	        sdsfreesplitres(c,t);
    	        
    	        sdsfreesplitres(argv,num);
    	    } else if (strstr(lines[j],"split")){
    	        lines[j] = sdstrim(lines[j]," \t\r\n");
    	        argv = sdssplitlen(lines[j],sdslen(lines[j]),",",1,&num);
    	        
  	            c = sdssplitlen(argv[0],sdslen(argv[0]),"=",1,&t);
        	    op_num += atoi(c[1]);  
        	    counter++;   	        
    	        if (i == 1) {
    	            op = sdscat(op,"split ");
        	        op = sdscat(op,c[1]);
        	        op = sdscat(op," ");        	          	            
    	        }    	        
    	        sdsfreesplitres(c,t); 
    	        
    	        c = sdssplitlen(argv[2],sdslen(argv[2]),"=",1,&t);
    	        total_time_split += atof(c[1]);
    	        sdsfreesplitres(c,t);
    	        
    	        sdsfreesplitres(argv,num);    	    
    	    } else {
    	        continue;
    	    }
    	    if (counter == 2) break;
    	}
    	
	    freeReplyObject(reply);
	    if (op_num != total_op) {
	        redisFree(conn_server);
	        break;
	    } else {
    	    reply = (redisReply*)redisCommand(conn_server,command_ufs);
    	    if (i == 1) {
    	        ufs_list = sdscat(ufs_list,reply->str);
    	    } else {
    	        ufs_list = sdscat(ufs_list,"+");
    	        ufs_list = sdscat(ufs_list,reply->str);
    	    }
    	    freeReplyObject(reply);
    	    redisFree(conn_server); 
    	}    	   	  	        
	}

    if (i > client_num) {
        int len,i;
        sds *elements = sdssplitlen(ufs_list,sdslen(ufs_list),"+",1,&len);
        //printf("elements: %s\n",ufs_list);
        printf("1: %s\n",elements[0]);
        for (i = 0; i < len-1; i++) {
            printf("%d: %s\n",i+2,elements[i+1]);
            if(sdscmp(elements[i],elements[i+1])) {
                printf("Contents of ufs are not the same!\n");
                break;
            }
        }
        sdsfreesplitres(elements,len);
        if (i == len-1) {
            avg_time_union = total_time_union/client_num/1000;
            avg_time_split = total_time_split/client_num/1000;
            
            printf("memory: %s\n",memory);
    	    printf("op: %s\n",op);
        	printf("average time: union: %.2fms split: %.2fms\n",avg_time_union,avg_time_split);
        	
        	FILE *fw = fopen("/home/xue/remote.csv","a+");
        	if (!fw) {
        	    printf("ERROR: fail to open file\n");
        	    return -1;
        	}
        	
		    fprintf(fw,"\t\t%d\t%d\t%d\t%d\t%.2f\t%.2f\t%s\n",client_num,total_op,op_union,op_split,avg_time_union,avg_time_split,memory);
	        fclose(fw);
        	
        }       
	} else {
        printf("There still exists client that does not finish processing!\n");	
	}

    sdsfree(ufs_list);
	sdsfree(memory);
	sdsfree(op);
	
	return 0;
}
