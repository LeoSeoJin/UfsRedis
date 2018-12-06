#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <hiredis/hiredis.h>
#include <pthread.h>

#define ELEMENTS 50
#define OP_UNION 0
#define OP_SPLIT 1
#define MAX 100000
#define ITEM_MAX 10000

void *p1(void *arg);
void *p2(void *arg);
void *p3(void *arg);
void *p4(void *arg);
void *p5(void *arg);
void *p6(void *arg);
void *p7(void *arg);
void *p8(void *arg);
void *p9(void *arg);
void *p10(void *arg);
void *p11(void *arg);
void *p12(void *arg);
void replaceChar(char *string, char oldChar, char newChar);

char *ip[] = {"139.224.130.80",
              "47.100.34.153","47.100.34.153","47.100.34.153","47.100.34.153","47.100.34.153","47.100.34.153",
              "47.99.201.21","47.99.201.21","47.99.201.21","47.99.201.21","47.99.201.21","47.99.201.21"};
int port[] = {6379,6380,6381,6382,6383,6384,6385,6386,6387,6388,6389,6390,6391};
char *strport[] = {"6379","6380","6381","6382","6383","6384","6385","6386","6387","6388","6389","6390","6391"};
void *(*p[])(void*) = {p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12};  

char *key_list[] = {"group1","group2","group3","group4"};

int client_on = 0;
char *ufsid;
char *total_op;
char *upercent;

/*arguments:
 *argv[1]: thread_num
 *argv[2]: total command number (union and split)
 *argv[3]: union command percent
 *argv[4]: id of initial content
 *argv[5]: key of ufs
*/
int main(int argc, char*argv[]) {   
    int thread_num = atoi(argv[1]);
    total_op = argv[2];
    upercent = argv[3];
    ufsid = argv[4];
    int ufs_id = atoi(argv[4]);
    int key = atoi(argv[5])-1;
      
	pthread_t thread[thread_num];
	int rc[thread_num];
	int i;
	
	redisContext *conn_server = redisConnect(ip[0],port[0]);
	if(conn_server->err) printf("Connection error: %s\n",conn_server->errstr);

	redisReply* reply = NULL;	
	reply = (redisReply*)redisCommand(conn_server,"auth jx062325");
	freeReplyObject(reply);

	FILE *fr = fopen("/home/xue/ufs.txt","r+");
	if (!fr) {
	    printf("ERROR: fail to open file\n");
	    return -1;
	}
	
	char str[MAX];
	i = 1;
	while (!feof(fr)) {
	     fgets(str,MAX,fr); 
	     if (i == ufs_id) break; 
	     i++;  
	}
	fclose(fr);	
	str[strlen(str)-1] = '\0';	
			
	char command[100] = "uinit ";
	strcat(command,key_list[key]);
	strcat(command," ");
    strcat(command,str);
		
	reply = (redisReply*)redisCommand(conn_server,command);
	
	freeReplyObject(reply);
	redisFree(conn_server);
	
	usleep(80000); //uinit is the first command of all clients
		   
    int k = 0;
    int max_thread = 12;
	for (i = 0; i < thread_num; i++) {	    
	    k = (i < thread_num/2)?i:(max_thread/2+i-thread_num/2);
	    rc[i] = pthread_create(&thread[i],NULL,p[k],NULL);
	    printf("Create thread %ld %s_%d %d\n",thread[i],ip[k+1],port[k+1],rc[i]);

	    if (rc[i]) {
	        printf("ERROR: return code is %d\n",rc[i]);
	        return -1;
	    }
	}
	client_on = thread_num;

	for (i = 0; i < thread_num; i++) {
	    //printf("join: %d\n",i);
	    pthread_join(thread[i],NULL);
    }
	return 0 ; 
}

void replaceChar(char *string, char oldChar, char newChar) {
	char *result = string;
    int len = strlen(string);
    int i;
    for (i = 0; i < len; i++){
        if (result[i] == oldChar){
            result[i] = newChar;
        }
    }
    strcpy(string,result);
}

void *p1(void *arg) {
    while (!client_on) {}    
    int client = 1;
    //printf("%d %s_%s %s %s\n",client_on,ip[client],strport[client],total_op,upercent);
    	
	FILE *fr;
    char filename[100] = "/home/xue/workload/";
    
    strcat(filename,ufsid);
    strcat(filename,"/");
    strcat(filename,total_op);
    strcat(filename,"/");
    strcat(filename,upercent);
    strcat(filename,"/");
    strcat(filename,strport[client]);
    strcat(filename,".txt");
    
    printf("%d %s_%s %s\n",client_on,ip[client],strport[client],filename);
    
    fr = fopen(filename,"r+");
	if(!fr) {
		printf("ERROR: fail to open file\n");
		return NULL;
	}

	redisContext *conn_client = redisConnect(ip[client],port[client]);
	if(conn_client->err) printf("Connection error: %s \n",conn_client->errstr);
	redisReply* reply = NULL;	
    
    int i = 0;	
    int totalop = atoi(total_op);		
	char command[100];			
	while (i < totalop) {
	    fgets(command,100,fr);
	    replaceChar(command,'\n',' ');
		reply = (redisReply*)redisCommand(conn_client,command);
		freeReplyObject(reply);	
		i++;
	}
	
	redisFree(conn_client);	
		
	return NULL;
}

void *p2(void *arg) {
    while (!client_on) {}    
    int client = 2;
    //printf("%d %s_%s %s %s\n",client_on,ip[client],strport[client],total_op,upercent);
    	
	FILE *fr;

    char filename[100] = "/home/xue/workload/";
    strcat(filename,ufsid);
    strcat(filename,"/");
    strcat(filename,total_op);
    strcat(filename,"/");
    strcat(filename,upercent);
    strcat(filename,"/");
    strcat(filename,strport[client]);
    strcat(filename,".txt");
    
    printf("%d %s_%s %s\n",client_on,ip[client],strport[client],filename);
    
    fr = fopen(filename,"r+");
	if(!fr) {
		printf("ERROR: fail to open file\n");
		return NULL;
	}

	redisContext *conn_client = redisConnect(ip[client],port[client]);
	if(conn_client->err) printf("Connection error: %s \n",conn_client->errstr);
	redisReply* reply = NULL;	
    
    int i = 0;	
    int totalop = atoi(total_op);		
	char command[100];			
	while (i < totalop) {
	    fgets(command,100,fr);
	    replaceChar(command,'\n',' ');
		reply = (redisReply*)redisCommand(conn_client,command);
		freeReplyObject(reply);	
		i++;
	}
	
	redisFree(conn_client);	
		
	return NULL;
}

void *p3(void *arg) {
    while (!client_on) {}    
    int client = 3;
    //printf("%d %s_%s %s %s\n",client_on,ip[client],strport[client],total_op,upercent);
    	
	FILE *fr;
    char filename[100] = "/home/xue/workload/";
    strcat(filename,ufsid);
    strcat(filename,"/");    
    strcat(filename,total_op);
    strcat(filename,"/");
    strcat(filename,upercent);
    strcat(filename,"/");
    strcat(filename,strport[client]);
    strcat(filename,".txt");
    
    printf("%d %s_%s %s\n",client_on,ip[client],strport[client],filename);
    
    fr = fopen(filename,"r+");
	if(!fr) {
		printf("ERROR: fail to open file\n");
		return NULL;
	}

	redisContext *conn_client = redisConnect(ip[client],port[client]);
	if(conn_client->err) printf("Connection error: %s \n",conn_client->errstr);
	redisReply* reply = NULL;	
    
    int i = 0;	
    int totalop = atoi(total_op);		
	char command[100];			
	while (i < totalop) {
	    fgets(command,100,fr);
	    replaceChar(command,'\n',' ');
		reply = (redisReply*)redisCommand(conn_client,command);
		freeReplyObject(reply);	
		i++;
	}
	
	redisFree(conn_client);	
		
	return NULL;
}

void *p4(void *arg) {
    while (!client_on) {}    
    int client = 4;
    //printf("%d %s_%s %s %s\n",client_on,ip[client],strport[client],total_op,upercent);
    	
	FILE *fr;
    char filename[100] = "/home/xue/workload/";
    strcat(filename,ufsid);
    strcat(filename,"/");    
    strcat(filename,total_op);
    strcat(filename,"/");
    strcat(filename,upercent);
    strcat(filename,"/");
    strcat(filename,strport[client]);
    strcat(filename,".txt");
    
    printf("%d %s_%s %s\n",client_on,ip[client],strport[client],filename);
    
    fr = fopen(filename,"r+");
	if(!fr) {
		printf("ERROR: fail to open file\n");
		return NULL;
	}

	redisContext *conn_client = redisConnect(ip[client],port[client]);
	if(conn_client->err) printf("Connection error: %s \n",conn_client->errstr);
	redisReply* reply = NULL;	
    
    int i = 0;	
    int totalop = atoi(total_op);		
	char command[100];			
	while (i < totalop) {
	    fgets(command,100,fr);
	    replaceChar(command,'\n',' ');
		reply = (redisReply*)redisCommand(conn_client,command);
		freeReplyObject(reply);	
		i++;
	}
	
	redisFree(conn_client);	
		
	return NULL;
}

void *p5(void *arg) {
    while (!client_on) {}    
    int client = 5;
    //printf("%d %s_%s %s %s\n",client_on,ip[client],strport[client],total_op,upercent);
    	
	FILE *fr;
    char filename[100] = "/home/xue/workload/";
    strcat(filename,ufsid);
    strcat(filename,"/");    
    strcat(filename,total_op);
    strcat(filename,"/");
    strcat(filename,upercent);
    strcat(filename,"/");
    strcat(filename,strport[client]);
    strcat(filename,".txt");
    
    printf("%d %s_%s %s\n",client_on,ip[client],strport[client],filename);
    
    fr = fopen(filename,"r+");
	if(!fr) {
		printf("ERROR: fail to open file\n");
		return NULL;
	}

	redisContext *conn_client = redisConnect(ip[client],port[client]);
	if(conn_client->err) printf("Connection error: %s \n",conn_client->errstr);
	redisReply* reply = NULL;	
    
    int i = 0;	
    int totalop = atoi(total_op);		
	char command[100];			
	while (i < totalop) {
	    fgets(command,100,fr);
	    replaceChar(command,'\n',' ');
		reply = (redisReply*)redisCommand(conn_client,command);
		freeReplyObject(reply);	
		i++;
	}
	
	redisFree(conn_client);	
		
	return NULL;
}

void *p6(void *arg) {
    while (!client_on) {}    
    int client = 6;
    //printf("%d %s_%s %s %s\n",client_on,ip[client],strport[client],total_op,upercent);
    	
	FILE *fr;
    char filename[100] = "/home/xue/workload/";
    strcat(filename,ufsid);
    strcat(filename,"/");    
    strcat(filename,total_op);
    strcat(filename,"/");
    strcat(filename,upercent);
    strcat(filename,"/");
    strcat(filename,strport[client]);
    strcat(filename,".txt");
    
    printf("%d %s_%s %s\n",client_on,ip[client],strport[client],filename);
    
    fr = fopen(filename,"r+");
	if(!fr) {
		printf("ERROR: fail to open file\n");
		return NULL;
	}

	redisContext *conn_client = redisConnect(ip[client],port[client]);
	if(conn_client->err) printf("Connection error: %s \n",conn_client->errstr);
	redisReply* reply = NULL;	
    
    int i = 0;	
    int totalop = atoi(total_op);		
	char command[100];			
	while (i < totalop) {
	    fgets(command,100,fr);
	    replaceChar(command,'\n',' ');
		reply = (redisReply*)redisCommand(conn_client,command);
		freeReplyObject(reply);	
		i++;
	}
	
	redisFree(conn_client);	
		
	return NULL;
}

void *p7(void *arg) {
    while (!client_on) {}    
    int client = 7;
    //printf("%d %s_%s %s %s\n",client_on,ip[client],strport[client],total_op,upercent);
    	
	FILE *fr;
    char filename[100] = "/home/xue/workload/";
    strcat(filename,ufsid);
    strcat(filename,"/");    
    strcat(filename,total_op);
    strcat(filename,"/");
    strcat(filename,upercent);
    strcat(filename,"/");
    strcat(filename,strport[client]);
    strcat(filename,".txt");
    
    printf("%d %s_%s %s\n",client_on,ip[client],strport[client],filename);
    
    fr = fopen(filename,"r+");
	if(!fr) {
		printf("ERROR: fail to open file\n");
		return NULL;
	}

	redisContext *conn_client = redisConnect(ip[client],port[client]);
	if(conn_client->err) printf("Connection error: %s \n",conn_client->errstr);
	redisReply* reply = NULL;	
    
    int i = 0;	
    int totalop = atoi(total_op);		
	char command[100];			
	while (i < totalop) {
	    fgets(command,100,fr);
	    replaceChar(command,'\n',' ');
		reply = (redisReply*)redisCommand(conn_client,command);
		freeReplyObject(reply);	
		i++;
	}
	
	redisFree(conn_client);	
		
	return NULL;
}

void *p8(void *arg) {
    while (!client_on) {}    
    int client = 8;
    //printf("%d %s_%s %s %s\n",client_on,ip[client],strport[client],total_op,upercent);
    	
	FILE *fr;
    char filename[100] = "/home/xue/workload/";
    strcat(filename,ufsid);
    strcat(filename,"/");    
    strcat(filename,total_op);
    strcat(filename,"/");
    strcat(filename,upercent);
    strcat(filename,"/");
    strcat(filename,strport[client]);
    strcat(filename,".txt");
    
    printf("%d %s_%s %s\n",client_on,ip[client],strport[client],filename);
    
    fr = fopen(filename,"r+");
	if(!fr) {
		printf("ERROR: fail to open file\n");
		return NULL;
	}

	redisContext *conn_client = redisConnect(ip[client],port[client]);
	if(conn_client->err) printf("Connection error: %s \n",conn_client->errstr);
	redisReply* reply = NULL;	
    
    int i = 0;	
    int totalop = atoi(total_op);		
	char command[100];			
	while (i < totalop) {
	    fgets(command,100,fr);
	    replaceChar(command,'\n',' ');
		reply = (redisReply*)redisCommand(conn_client,command);
		freeReplyObject(reply);	
		i++;
	}
	
	redisFree(conn_client);	
		
	return NULL;
}

void *p9(void *arg) {
    while (!client_on) {}    
    int client = 9;
    //printf("%d %s_%s %s %s\n",client_on,ip[client],strport[client],total_op,upercent);
    	
	FILE *fr;
    char filename[100] = "/home/xue/workload/";
    strcat(filename,ufsid);
    strcat(filename,"/");    
    strcat(filename,total_op);
    strcat(filename,"/");
    strcat(filename,upercent);
    strcat(filename,"/");
    strcat(filename,strport[client]);
    strcat(filename,".txt");
    
    printf("%d %s_%s %s\n",client_on,ip[client],strport[client],filename);
    
    fr = fopen(filename,"r+");
	if(!fr) {
		printf("ERROR: fail to open file\n");
		return NULL;
	}

	redisContext *conn_client = redisConnect(ip[client],port[client]);
	if(conn_client->err) printf("Connection error: %s \n",conn_client->errstr);
	redisReply* reply = NULL;	
    
    int i = 0;	
    int totalop = atoi(total_op);		
	char command[100];			
	while (i < totalop) {
	    fgets(command,100,fr);
	    replaceChar(command,'\n',' ');
		reply = (redisReply*)redisCommand(conn_client,command);
		freeReplyObject(reply);	
		i++;
	}
	
	redisFree(conn_client);	
		
	return NULL;
}

void *p10(void *arg) {
    while (!client_on) {}    
    int client = 10;
    //printf("%d %s_%s %s %s\n",client_on,ip[client],strport[client],total_op,upercent);
    	
	FILE *fr;
    char filename[100] = "/home/xue/workload/";
    strcat(filename,ufsid);
    strcat(filename,"/");    
    strcat(filename,total_op);
    strcat(filename,"/");
    strcat(filename,upercent);
    strcat(filename,"/");
    strcat(filename,strport[client]);
    strcat(filename,".txt");
    
    printf("%d %s_%s %s\n",client_on,ip[client],strport[client],filename);
    
    fr = fopen(filename,"r+");
	if(!fr) {
		printf("ERROR: fail to open file\n");
		return NULL;
	}

	redisContext *conn_client = redisConnect(ip[client],port[client]);
	if(conn_client->err) printf("Connection error: %s \n",conn_client->errstr);
	redisReply* reply = NULL;	
    
    int i = 0;	
    int totalop = atoi(total_op);		
	char command[100];			
	while (i < totalop) {
	    fgets(command,100,fr);
	    replaceChar(command,'\n',' ');
		reply = (redisReply*)redisCommand(conn_client,command);
		freeReplyObject(reply);	
		i++;
	}
	
	redisFree(conn_client);	
		
	return NULL;
}

void *p11(void *arg) {
    while (!client_on) {}    
    int client = 11;
    //printf("%d %s_%s %s %s\n",client_on,ip[client],strport[client],total_op,upercent);
    	
	FILE *fr;
    char filename[100] = "/home/xue/workload/";
    strcat(filename,ufsid);
    strcat(filename,"/");    
    strcat(filename,total_op);
    strcat(filename,"/");
    strcat(filename,upercent);
    strcat(filename,"/");
    strcat(filename,strport[client]);
    strcat(filename,".txt");
    
    printf("%d %s_%s %s\n",client_on,ip[client],strport[client],filename);
    
    fr = fopen(filename,"r+");
	if(!fr) {
		printf("ERROR: fail to open file\n");
		return NULL;
	}

	redisContext *conn_client = redisConnect(ip[client],port[client]);
	if(conn_client->err) printf("Connection error: %s \n",conn_client->errstr);
	redisReply* reply = NULL;	
    
    int i = 0;	
    int totalop = atoi(total_op);		
	char command[100];			
	while (i < totalop) {
	    fgets(command,100,fr);
	    replaceChar(command,'\n',' ');
		reply = (redisReply*)redisCommand(conn_client,command);
		freeReplyObject(reply);	
		i++;
	}
	
	redisFree(conn_client);	
		
	return NULL;
}

void *p12(void *arg) {
    while (!client_on) {}    
    int client = 12;
    //printf("%d %s_%s %s %s\n",client_on,ip[client],strport[client],total_op,upercent);
    	
	FILE *fr;
    char filename[100] = "/home/xue/workload/";
    strcat(filename,ufsid);
    strcat(filename,"/");    
    strcat(filename,total_op);
    strcat(filename,"/");
    strcat(filename,upercent);
    strcat(filename,"/");
    strcat(filename,strport[client]);
    strcat(filename,".txt");
    
    printf("%d %s_%s %s\n",client_on,ip[client],strport[client],filename);
    
    fr = fopen(filename,"r+");
	if(!fr) {
		printf("ERROR: fail to open file\n");
		return NULL;
	}

	redisContext *conn_client = redisConnect(ip[client],port[client]);
	if(conn_client->err) printf("Connection error: %s \n",conn_client->errstr);
	redisReply* reply = NULL;	
    
    int i = 0;	
    int totalop = atoi(total_op);		
	char command[100];			
	while (i < totalop) {
	    fgets(command,100,fr);
	    replaceChar(command,'\n',' ');
		reply = (redisReply*)redisCommand(conn_client,command);
		freeReplyObject(reply);	
		i++;
	}
	
	redisFree(conn_client);	
		
	return NULL;
}
