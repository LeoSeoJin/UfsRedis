#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <hiredis/hiredis.h>

#define ELEMENTS 50
#define OP_UNION 0
#define OP_SPLIT 1

extern void replaceChar(char *string, char oldChar, char newChar);

/*arguments:
 *argv[1]: ip
 *argv[2]: port 
 *argv[3]: total command number (union and split)
 *argv[4]: union command number
 *argv[5]: split command number 
*/
int main(int argc, char*argv[]) { 
	printf("ip: %s port: %d \n",argv[1],atoi(argv[2]));
	redisContext *conn_client = redisConnect(argv[1],atoi(argv[2]));
	if(conn_client->err) printf("Connection error: %s \n",conn_client->errstr);
	
	redisReply* reply = NULL;	
	reply = (redisReply*)redisCommand(conn_client,"umembers group1");
	
	char str[strlen(reply->str)];
	strcpy(str,reply->str);
	
    replaceChar(str,'/',',');
    
    int len = 1;
    int i;
	for (i = 0; i < strlen(str); i++)
		if (str[i] == ',') len++;
			
	char *ufs[len];
	memset(ufs,0,len);
	for (i = 0; i < len; i++)
		ufs[i] = (char *)malloc(sizeof(char *));
		
	i = 0;
	char delm[] = ",";
	char *r = strtok(str,delm);
	while (r != NULL) {
		strcpy((char *)ufs[i],(char *)r);
		r = strtok(NULL,delm);
		i++;
	}

	freeReplyObject(reply);
	
	/*generate and execute random operations*/
	i = 0;
	int op_type,argv1,argv2;
	int union_num = 0;
	int split_num = 0;
	int op_num = atoi(argv[3]);
	char *command = (char *)malloc(sizeof(char)*100);

	srand((unsigned int)time(NULL));
	while (i < op_num) {
		/*choose operation type: */
		op_type = rand()/(double)RAND_MAX*2;
		
		if (atoi(argv[4]) != 0 && atoi(argv[5]) != 0) {
			if (op_type == OP_UNION && union_num == atoi(argv[4])) 
				op_type = OP_SPLIT;
			if (op_type == OP_SPLIT && split_num == atoi(argv[5]))
				op_type = OP_UNION;
		}
			
		if (op_type == OP_UNION) {
			argv1 = rand()/(double)RAND_MAX*len;
			argv2 = rand()/(double)RAND_MAX*len;
			
			strcpy(command,"unionot ");
			strcat(command,ufs[argv1]);
			strcat(command," ");
			strcat(command,ufs[argv2]);
			strcat(command," group1");
				
			union_num++;	
		} else {
			argv1 = rand()/(double)RAND_MAX*len;
			
			strcpy(command,"splitot ");
			strcat(command,ufs[argv1]);
			strcat(command," group1");
			
			split_num++;			
		}
		reply = (redisReply*)redisCommand(conn_client,command);
		freeReplyObject(reply);	
		i++;
	}
	
	reply = (redisReply*)redisCommand(conn_client,"umembers group1");
	printf("ufs: %s \n",reply->str);
	
	redisFree(conn_client);
	
	return 0;
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
