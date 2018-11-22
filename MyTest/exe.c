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
#define MAX 100

void replaceChar(char *string, char oldChar, char newChar);
void *p1(void *arg);
void *p2(void *arg);
void *p3(void *arg);
void *p4(void *arg);
void *p5(void *arg);
void *p6(void *arg);
void *p7(void *arg);

char *ip[] = {"127.0.0.1","127.0.0.1","127.0.0.1","127.0.0.1","127.0.0.1","127.0.0.1","127.0.0.1"};
char *port[] = {"6379","6380","6381","6382","6383","6384","6385","6386"};
void *(*p[])(void*) = {p1,p2,p3,p4,p5,p6,p7};  

int total_op = 0;
int union_op = 0;
int split_op = 0;

char *initial_content[] = {"22,30/47,80/225,890/12,34","12,37,78/257,789,120/36/240,55,556/90,2"};
int ufs = 0;

char *key_list[] = {"group1","group2","group3","group4"};
int key = 0;

/*arguments:
 *argv[1]: thread_num
 *argv[2]: total command number (union and split)
 *argv[3]: union command number
 *argv[4]: split command number 
 *argv[5]: id of initial content
 *argv[6]: key of ufs
*/

int main(int argc, char*argv[]) {   
    int thread_num = atoi(argv[1]);
    total_op = atoi(argv[2]);
    union_op = atoi(argv[3]);
    split_op = atoi(argv[4]);
    key = atoi(argv[6])-1;
        
	pthread_t thread[thread_num];
	int rc[thread_num];
	int i;
	char *address[thread_num];
	memset(address,0,thread_num);
    
	redisContext *conn_server = redisConnect(ip[0],atoi(port[0]));
	if(conn_server->err) printf("Connection error: %s\n",conn_server->errstr);

	redisReply* reply = NULL;	
	//reply = (redisReply*)redisCommand(conn_server,"auth jx062325");
	//freeReplyObject(reply);
	
	ufs = atoi(argv[5])-1;
	char command[100] = "uinit ";
	strcat(command,key_list[key]);
	strcat(command," ");
	strcat(command,initial_content[ufs]);
	//printf("ufs: %s\n",initial_content[ufs]);
	reply = (redisReply*)redisCommand(conn_server,command);
	
	freeReplyObject(reply);
	redisFree(conn_server);
	
	//usleep(20000); 
	   
	for (i = 0; i < thread_num; i++) {	    
	    address[i] = (char*)malloc(sizeof(char*));
	    strcpy(address[i],ip[i+1]);
	    strcat(address[i],"_");
	    strcat(address[i],port[i+1]);
	    //printf("Create thread %d %s\n",i,address[i]);
	    
	    rc[i] = pthread_create(&thread[i],NULL,p[i],address[i]);
	    printf("Create thread %ld %s %d\n",thread[i],address[i],rc[i]);
	    if (rc[i]) {
	        printf("ERROR: return code is %d\n",rc[i]);
	        return -1;
	    }
	}

	for (i = 0; i < thread_num; i++) {
	    pthread_join(thread[i],NULL);
	    //usleep(20000);
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
    char *r,*ip,*port;
    int i = 0;
    int len = 1;
    
    r = strtok((char*)arg,"_");
    while (r != NULL) {
       if (i == 0) {
          ip = (char*)malloc(strlen(r)+1);
          strcpy(ip,r);
       } else {
          port = (char*)malloc(strlen(r)+1);
          strcpy(port,r);
       }
       r = strtok(NULL,"_");
       i++;
    }
    free((char*)arg);
    
    printf("ip: %s port: %s \n",ip,port);
    
	redisContext *conn_client = redisConnect(ip,atoi(port));
	if(conn_client->err) printf("Connection error: %s \n",conn_client->errstr);

	redisReply* reply = NULL;	
	/***
	reply = (redisReply*)redisCommand(conn_client,"umembers group1");
	
	char str[strlen(reply->str)];
	strcpy(str,reply->str);
    **/
	
	char str[MAX];
	strcpy(str,initial_content[ufs]);
		
    replaceChar(str,'/',',');

	for (i = 0; i < strlen(str); i++)
		if (str[i] == ',') len++;
			
	char *ufs[len];
	memset(ufs,0,len);
	for (i = 0; i < len; i++)
		ufs[i] = (char *)malloc(sizeof(char *));
		
	i = 0;
	r = strtok(str,",");
	while (r != NULL) {
		strcpy((char *)ufs[i],(char *)r);
		r = strtok(NULL,",");
		i++;
	}
    
	//freeReplyObject(reply);
	
	//*generate and execute random operations
	i = 0;
	int op_type,argv1,argv2;
	int union_num = 0;
	int split_num = 0;
	int op_num = total_op;
	char *command = NULL;
	
	srand((unsigned int)time(NULL));
	while (i < op_num) {
	    command = (char *)malloc(sizeof(char)*100);

		//choose operation type:
		op_type = rand()/(double)RAND_MAX*2;
		
		if (union_op != 0 && split_op != 0) {
			if (op_type == OP_UNION && union_num == union_op) 
				op_type = OP_SPLIT;
			if (op_type == OP_SPLIT && split_num == split_op)
				op_type = OP_UNION;
		}
			
		if (op_type == OP_UNION) {
			argv1 = rand()/(double)RAND_MAX*len;
			argv2 = rand()/(double)RAND_MAX*len;
			
			strcpy(command,"union ");
			strcat(command,ufs[argv1]);
			strcat(command," ");
			strcat(command,ufs[argv2]);
			strcat(command," ");
			strcat(command,key_list[key]);
				
			union_num++;	
		} else {
			argv1 = rand()/(double)RAND_MAX*len;
			
			strcpy(command,"split ");
			strcat(command,ufs[argv1]);
			strcat(command," ");
			strcat(command,key_list[key]);
			
			split_num++;			
		}
		reply = (redisReply*)redisCommand(conn_client,command);
		free(command);
		usleep(50000); //wait 50ms to guarantee send repl ack to server 
		freeReplyObject(reply);	
		i++;
	}
	
	redisFree(conn_client);
	for (i = 0; i < len; i++) free(ufs[i]);
	
	free(ip);
	free(port);
	return NULL;
}

void *p2(void *arg) {
    char *r,*ip,*port;
    int i = 0;
    int len = 1;
    
    r = strtok((char*)arg,"_");
    while (r != NULL) {
       if (i == 0) {
          ip = (char*)malloc(strlen(r)+1);
          strcpy(ip,r);
       } else {
          port = (char*)malloc(strlen(r)+1);
          strcpy(port,r);
       }
       r = strtok(NULL,"_");
       i++;
    }
    free((char*)arg);
    
    printf("ip: %s port: %s \n",ip,port);
    
	redisContext *conn_client = redisConnect(ip,atoi(port));
	if(conn_client->err) printf("Connection error: %s \n",conn_client->errstr);

	redisReply* reply = NULL;	
	/***
	reply = (redisReply*)redisCommand(conn_client,"umembers group1");
	
	char str[strlen(reply->str)];
	strcpy(str,reply->str);
    **/
	
	char str[MAX];
	strcpy(str,initial_content[ufs]);
		
    replaceChar(str,'/',',');

	for (i = 0; i < strlen(str); i++)
		if (str[i] == ',') len++;
			
	char *ufs[len];
	memset(ufs,0,len);
	for (i = 0; i < len; i++)
		ufs[i] = (char *)malloc(sizeof(char *));
		
	i = 0;
	r = strtok(str,",");
	while (r != NULL) {
		strcpy((char *)ufs[i],(char *)r);
		r = strtok(NULL,",");
		i++;
	}
    
	//freeReplyObject(reply);
	
	//*generate and execute random operations
	i = 0;
	int op_type,argv1,argv2;

	int union_num = 0;
	int split_num = 0;
	int op_num = total_op;
	char *command = NULL;
	
	srand((unsigned int)time(NULL));
	while (i < op_num) {
	    command = (char *)malloc(sizeof(char)*100);

		//choose operation type:
		op_type = rand()/(double)RAND_MAX*2;
		
		if (union_op != 0 && split_op != 0) {
			if (op_type == OP_UNION && union_num == union_op) 
				op_type = OP_SPLIT;
			if (op_type == OP_SPLIT && split_num == split_op)
				op_type = OP_UNION;
		}
			
		if (op_type == OP_UNION) {
			argv1 = rand()/(double)RAND_MAX*len;
			argv2 = rand()/(double)RAND_MAX*len;
			
			strcpy(command,"union ");
			strcat(command,ufs[argv1]);
			strcat(command," ");
			strcat(command,ufs[argv2]);
			strcat(command," ");
			strcat(command,key_list[key]);
				
			union_num++;	
		} else {
			argv1 = rand()/(double)RAND_MAX*len;
			
			strcpy(command,"split ");
			strcat(command,ufs[argv1]);
			strcat(command," ");
			strcat(command,key_list[key]);
			
			split_num++;			
		}
		reply = (redisReply*)redisCommand(conn_client,command);
		free(command);
		usleep(50000); //wait 50ms to guarantee send repl ack to server 
		freeReplyObject(reply);	
		i++;
	}
	
	redisFree(conn_client);
	for (i = 0; i < len; i++) free(ufs[i]);
	
	free(ip);
	free(port);
	return NULL;
}

void *p3(void *arg) {
    char *r,*ip,*port;
    int i = 0;
    int len = 1;
    
    r = strtok((char*)arg,"_");
    while (r != NULL) {
       if (i == 0) {
          ip = (char*)malloc(strlen(r)+1);
          strcpy(ip,r);
       } else {
          port = (char*)malloc(strlen(r)+1);
          strcpy(port,r);
       }
       r = strtok(NULL,"_");
       i++;
    }
    free((char*)arg);
    
    printf("ip: %s port: %s \n",ip,port);
    
	redisContext *conn_client = redisConnect(ip,atoi(port));
	if(conn_client->err) printf("Connection error: %s \n",conn_client->errstr);

	redisReply* reply = NULL;	
	/***
	reply = (redisReply*)redisCommand(conn_client,"umembers group1");
	
	char str[strlen(reply->str)];
	strcpy(str,reply->str);
    **/
	
	char str[MAX];
	strcpy(str,initial_content[ufs]);
		
    replaceChar(str,'/',',');

	for (i = 0; i < strlen(str); i++)
		if (str[i] == ',') len++;
			
	char *ufs[len];
	memset(ufs,0,len);
	for (i = 0; i < len; i++)
		ufs[i] = (char *)malloc(sizeof(char *));
		
	i = 0;
	r = strtok(str,",");
	while (r != NULL) {
		strcpy((char *)ufs[i],(char *)r);
		r = strtok(NULL,",");
		i++;
	}
    
	//freeReplyObject(reply);
	

	//*generate and execute random operations

	i = 0;

	int op_type,argv1,argv2;

	int union_num = 0;
	int split_num = 0;
	int op_num = total_op;
	char *command = NULL;
	
	srand((unsigned int)time(NULL));
	while (i < op_num) {
	    command = (char *)malloc(sizeof(char)*100);

		//choose operation type:
		op_type = rand()/(double)RAND_MAX*2;
		
		if (union_op != 0 && split_op != 0) {
			if (op_type == OP_UNION && union_num == union_op) 
				op_type = OP_SPLIT;
			if (op_type == OP_SPLIT && split_num == split_op)
				op_type = OP_UNION;
		}
			
		if (op_type == OP_UNION) {
			argv1 = rand()/(double)RAND_MAX*len;
			argv2 = rand()/(double)RAND_MAX*len;
			
			strcpy(command,"union ");
			strcat(command,ufs[argv1]);
			strcat(command," ");
			strcat(command,ufs[argv2]);
			strcat(command," ");
			strcat(command,key_list[key]);
				
			union_num++;	
		} else {
			argv1 = rand()/(double)RAND_MAX*len;
			
			strcpy(command,"split ");
			strcat(command,ufs[argv1]);
			strcat(command," ");
			strcat(command,key_list[key]);
			
			split_num++;			
		}
		reply = (redisReply*)redisCommand(conn_client,command);
		free(command);
		usleep(50000); //wait 50ms to guarantee send repl ack to server 
		freeReplyObject(reply);	
		i++;
	}
	
	redisFree(conn_client);
	for (i = 0; i < len; i++) free(ufs[i]);
	
	free(ip);
	free(port);
	return NULL;
}

void *p4(void *arg) {
    char *r,*ip,*port;
    int i = 0;
    int len = 1;
    
    r = strtok((char*)arg,"_");
    while (r != NULL) {
       if (i == 0) {
          ip = (char*)malloc(strlen(r)+1);
          strcpy(ip,r);
       } else {
          port = (char*)malloc(strlen(r)+1);
          strcpy(port,r);
       }
       r = strtok(NULL,"_");
       i++;
    }
    free((char*)arg);
    
    printf("ip: %s port: %s \n",ip,port);
    
	redisContext *conn_client = redisConnect(ip,atoi(port));
	if(conn_client->err) printf("Connection error: %s \n",conn_client->errstr);

	redisReply* reply = NULL;	
	/***
	reply = (redisReply*)redisCommand(conn_client,"umembers group1");
	
	char str[strlen(reply->str)];
	strcpy(str,reply->str);
    **/
	
	char str[MAX];
	strcpy(str,initial_content[ufs]);
		
    replaceChar(str,'/',',');

	for (i = 0; i < strlen(str); i++)
		if (str[i] == ',') len++;
			
	char *ufs[len];
	memset(ufs,0,len);
	for (i = 0; i < len; i++)
		ufs[i] = (char *)malloc(sizeof(char *));
		
	i = 0;
	r = strtok(str,",");
	while (r != NULL) {
		strcpy((char *)ufs[i],(char *)r);
		r = strtok(NULL,",");
		i++;
	}
    
	//freeReplyObject(reply);
	

	//*generate and execute random operations

	i = 0;

	int op_type,argv1,argv2;

	int union_num = 0;
	int split_num = 0;
	int op_num = total_op;
	char *command = NULL;
	
	srand((unsigned int)time(NULL));
	while (i < op_num) {
	    command = (char *)malloc(sizeof(char)*100);

		//choose operation type:
		op_type = rand()/(double)RAND_MAX*2;
		
		if (union_op != 0 && split_op != 0) {
			if (op_type == OP_UNION && union_num == union_op) 
				op_type = OP_SPLIT;
			if (op_type == OP_SPLIT && split_num == split_op)
				op_type = OP_UNION;
		}
			
		if (op_type == OP_UNION) {
			argv1 = rand()/(double)RAND_MAX*len;
			argv2 = rand()/(double)RAND_MAX*len;
			
			strcpy(command,"union ");
			strcat(command,ufs[argv1]);
			strcat(command," ");
			strcat(command,ufs[argv2]);
			strcat(command," ");
			strcat(command,key_list[key]);
				
			union_num++;	
		} else {
			argv1 = rand()/(double)RAND_MAX*len;
			
			strcpy(command,"split ");
			strcat(command,ufs[argv1]);
			strcat(command," ");
			strcat(command,key_list[key]);
			
			split_num++;			
		}
		reply = (redisReply*)redisCommand(conn_client,command);
		free(command);
		usleep(50000); //wait 50ms to guarantee send repl ack to server 
		freeReplyObject(reply);	
		i++;
	}
	
	redisFree(conn_client);
	for (i = 0; i < len; i++) free(ufs[i]);
	
	free(ip);
	free(port);
	return NULL;
}

void *p5(void *arg) {
    char *r,*ip,*port;
    int i = 0;
    int len = 1;
    
    r = strtok((char*)arg,"_");
    while (r != NULL) {
       if (i == 0) {
          ip = (char*)malloc(strlen(r)+1);
          strcpy(ip,r);
       } else {
          port = (char*)malloc(strlen(r)+1);
          strcpy(port,r);
       }
       r = strtok(NULL,"_");
       i++;
    }
    free((char*)arg);
    
    printf("ip: %s port: %s \n",ip,port);
    
	redisContext *conn_client = redisConnect(ip,atoi(port));
	if(conn_client->err) printf("Connection error: %s \n",conn_client->errstr);

	redisReply* reply = NULL;	
	/***
	reply = (redisReply*)redisCommand(conn_client,"umembers group1");
	
	char str[strlen(reply->str)];
	strcpy(str,reply->str);
    **/
	
	char str[MAX];
	strcpy(str,initial_content[ufs]);
		
    replaceChar(str,'/',',');

	for (i = 0; i < strlen(str); i++)
		if (str[i] == ',') len++;
			
	char *ufs[len];
	memset(ufs,0,len);
	for (i = 0; i < len; i++)
		ufs[i] = (char *)malloc(sizeof(char *));
		
	i = 0;
	r = strtok(str,",");
	while (r != NULL) {
		strcpy((char *)ufs[i],(char *)r);
		r = strtok(NULL,",");
		i++;
	}
    
	//freeReplyObject(reply);
	

	//*generate and execute random operations

	i = 0;

	int op_type,argv1,argv2;

	int union_num = 0;
	int split_num = 0;
	int op_num = total_op;
	char *command = NULL;
	
	srand((unsigned int)time(NULL));
	while (i < op_num) {
	    command = (char *)malloc(sizeof(char)*100);

		//choose operation type:
		op_type = rand()/(double)RAND_MAX*2;
		
		if (union_op != 0 && split_op != 0) {
			if (op_type == OP_UNION && union_num == union_op) 
				op_type = OP_SPLIT;
			if (op_type == OP_SPLIT && split_num == split_op)
				op_type = OP_UNION;
		}
			
		if (op_type == OP_UNION) {
			argv1 = rand()/(double)RAND_MAX*len;
			argv2 = rand()/(double)RAND_MAX*len;
			
			strcpy(command,"union ");
			strcat(command,ufs[argv1]);
			strcat(command," ");
			strcat(command,ufs[argv2]);
			strcat(command," ");
			strcat(command,key_list[key]);
				
			union_num++;	
		} else {
			argv1 = rand()/(double)RAND_MAX*len;
			
			strcpy(command,"split ");
			strcat(command,ufs[argv1]);
			strcat(command," ");
			strcat(command,key_list[key]);
			
			split_num++;			
		}
		reply = (redisReply*)redisCommand(conn_client,command);
		free(command);
		usleep(50000); //wait 50ms to guarantee send repl ack to server 
		freeReplyObject(reply);	
		i++;
	}
	
	redisFree(conn_client);
	for (i = 0; i < len; i++) free(ufs[i]);
	
	free(ip);
	free(port);
	return NULL;
}

void *p6(void *arg) {
    char *r,*ip,*port;
    int i = 0;
    int len = 1;
    
    r = strtok((char*)arg,"_");
    while (r != NULL) {
       if (i == 0) {
          ip = (char*)malloc(strlen(r)+1);
          strcpy(ip,r);
       } else {
          port = (char*)malloc(strlen(r)+1);
          strcpy(port,r);
       }
       r = strtok(NULL,"_");
       i++;
    }
    free((char*)arg);
    
    printf("ip: %s port: %s \n",ip,port);
    
	redisContext *conn_client = redisConnect(ip,atoi(port));
	if(conn_client->err) printf("Connection error: %s \n",conn_client->errstr);

	redisReply* reply = NULL;	
	/***
	reply = (redisReply*)redisCommand(conn_client,"umembers group1");
	
	char str[strlen(reply->str)];
	strcpy(str,reply->str);
    **/
	
	char str[MAX];
	strcpy(str,initial_content[ufs]);
		
    replaceChar(str,'/',',');

	for (i = 0; i < strlen(str); i++)
		if (str[i] == ',') len++;
			
	char *ufs[len];
	memset(ufs,0,len);
	for (i = 0; i < len; i++)
		ufs[i] = (char *)malloc(sizeof(char *));
		
	i = 0;
	r = strtok(str,",");
	while (r != NULL) {
		strcpy((char *)ufs[i],(char *)r);
		r = strtok(NULL,",");
		i++;
	}
    
	//freeReplyObject(reply);
	

	//*generate and execute random operations

	i = 0;

	int op_type,argv1,argv2;

	int union_num = 0;
	int split_num = 0;
	int op_num = total_op;
	char *command = NULL;
	
	srand((unsigned int)time(NULL));
	while (i < op_num) {
	    command = (char *)malloc(sizeof(char)*100);

		//choose operation type:
		op_type = rand()/(double)RAND_MAX*2;
		
		if (union_op != 0 && split_op != 0) {
			if (op_type == OP_UNION && union_num == union_op) 
				op_type = OP_SPLIT;
			if (op_type == OP_SPLIT && split_num == split_op)
				op_type = OP_UNION;
		}
			
		if (op_type == OP_UNION) {
			argv1 = rand()/(double)RAND_MAX*len;
			argv2 = rand()/(double)RAND_MAX*len;
			
			strcpy(command,"union ");
			strcat(command,ufs[argv1]);
			strcat(command," ");
			strcat(command,ufs[argv2]);
			strcat(command," ");
			strcat(command,key_list[key]);
				
			union_num++;	
		} else {
			argv1 = rand()/(double)RAND_MAX*len;
			
			strcpy(command,"split ");
			strcat(command,ufs[argv1]);
			strcat(command," ");
			strcat(command,key_list[key]);
			
			split_num++;			
		}
		reply = (redisReply*)redisCommand(conn_client,command);
		free(command);
		usleep(50000); //wait 50ms to guarantee send repl ack to server 
		freeReplyObject(reply);	
		i++;
	}
	
	redisFree(conn_client);
	for (i = 0; i < len; i++) free(ufs[i]);
	
	free(ip);
	free(port);
	return NULL;
}

void *p7(void *arg) {
    char *r,*ip,*port;
    int i = 0;
    int len = 1;
    
    r = strtok((char*)arg,"_");
    while (r != NULL) {
       if (i == 0) {
          ip = (char*)malloc(strlen(r)+1);
          strcpy(ip,r);
       } else {
          port = (char*)malloc(strlen(r)+1);
          strcpy(port,r);
       }
       r = strtok(NULL,"_");
       i++;
    }
    free((char*)arg);
    
    printf("ip: %s port: %s \n",ip,port);
    
	redisContext *conn_client = redisConnect(ip,atoi(port));
	if(conn_client->err) printf("Connection error: %s \n",conn_client->errstr);

	redisReply* reply = NULL;	
	/***
	reply = (redisReply*)redisCommand(conn_client,"umembers group1");
	
	char str[strlen(reply->str)];
	strcpy(str,reply->str);
    **/
	
	char str[MAX];
	strcpy(str,initial_content[ufs]);
		
    replaceChar(str,'/',',');

	for (i = 0; i < strlen(str); i++)
		if (str[i] == ',') len++;
			
	char *ufs[len];
	memset(ufs,0,len);
	for (i = 0; i < len; i++)
		ufs[i] = (char *)malloc(sizeof(char *));
		
	i = 0;
	r = strtok(str,",");
	while (r != NULL) {
		strcpy((char *)ufs[i],(char *)r);
		r = strtok(NULL,",");
		i++;
	}
    
	//freeReplyObject(reply);
	

	//*generate and execute random operations

	i = 0;

	int op_type,argv1,argv2;

	int union_num = 0;
	int split_num = 0;
	int op_num = total_op;
	char *command = NULL;
	
	srand((unsigned int)time(NULL));
	while (i < op_num) {
	    command = (char *)malloc(sizeof(char)*100);

		//choose operation type:
		op_type = rand()/(double)RAND_MAX*2;
		
		if (union_op != 0 && split_op != 0) {
			if (op_type == OP_UNION && union_num == union_op) 
				op_type = OP_SPLIT;
			if (op_type == OP_SPLIT && split_num == split_op)
				op_type = OP_UNION;
		}
			
		if (op_type == OP_UNION) {
			argv1 = rand()/(double)RAND_MAX*len;
			argv2 = rand()/(double)RAND_MAX*len;
			
			strcpy(command,"union ");
			strcat(command,ufs[argv1]);
			strcat(command," ");
			strcat(command,ufs[argv2]);
			strcat(command," ");
			strcat(command,key_list[key]);
				
			union_num++;	
		} else {
			argv1 = rand()/(double)RAND_MAX*len;
			
			strcpy(command,"split ");
			strcat(command,ufs[argv1]);
			strcat(command," ");
			strcat(command,key_list[key]);
			
			split_num++;			
		}
		reply = (redisReply*)redisCommand(conn_client,command);
		free(command);
		usleep(50000); //wait 50ms to guarantee send repl ack to server 
		freeReplyObject(reply);	
		i++;
	}
	
	redisFree(conn_client);
	for (i = 0; i < len; i++) free(ufs[i]);
	
	free(ip);
	free(port);
	return NULL;
}
