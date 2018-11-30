#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ELEMENTS 50
#define OP_UNION 0
#define OP_SPLIT 1
#define MAX 1000
#define ITEM_MAX 100

void replaceChar(char *string, char oldChar, char newChar);
void generate(char *id, char *total, char *upercent, int client);

char *port[] = {"6379","6380","6381","6382","6383","6384","6385","6386","6387","6388","6389","6390","6391"}; 

int total_op = 0;
int union_op = 0;
int split_op = 0;

//char *initial_content[] = {"22,30/47,80/225,890/12,34","12,37,78/257,789,120/36/240,55,556/90,2","red,yellow/blue,gray,black/scarlet/green,pink/white,purple"};

char *key_list[] = {"group1","group2","group3","group4"};
int key = 0;

int len = 1;
char *ufs[ITEM_MAX];

/*arguments:
 *argv[1]: thread_num
 *argv[2]: total command number (union and split)
 *argv[3]: union command percent
 *argv[4]: id of initial content
 *argv[5]: key of ufs
*/

int main(int argc, char*argv[]) {   
    int thread_num = atoi(argv[1]);
    total_op = atoi(argv[2]);
    double union_percent = atof(argv[3]);
    int ufs_id = atoi(argv[4]);
    key = atoi(argv[5])-1;

    union_op = total_op*union_percent;
    split_op = total_op-union_op;
            
	pthread_t thread[thread_num];
	int rc[thread_num];
	int i;
		
	char *r;
	char str[MAX];
	
	FILE *fr = fopen("/home/xue/ufs.txt","r+");
	if (!fr) {
	    printf("ERROR: fail to open file\n");
	    return -1;
	}
	
	i = 1;
	while (!feof(fr)) {
	     fgets(str,MAX,fr); 
	     if (i == ufs_id) break; 
	     i++;  
	}
	fclose(fr);
	
	str[strlen(str)-1] = '\0';	
		
    replaceChar(str,'/',',');
	for (i = 0; i < strlen(str); i++)
		if (str[i] == ',') len++;
			
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
	   
    int k = 0;
    int max_thread = 12;
	for (i = 0; i < thread_num; i++) {	    
	    k = (i < thread_num/2)?i:(max_thread/2+i-thread_num/2);
	    generate(argv[4],argv[2],argv[3],k+1);
	    sleep(1);	   
	}
	
	for (i = 0; i < len; i++) free(ufs[i]);	
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


void generate(char *id, char *total, char *upercent, int client) {
    int i;    
    printf("port: %s\n",port[client]);
    
	FILE *fw;
    char filename[50] = "/home/xue/workload/";
    
    strcat(filename,id);
    strcat(filename,"/");
    strcat(filename,total);
    strcat(filename,"/");
    strcat(filename,upercent);
    strcat(filename,"/");
    strcat(filename,port[client]);
    strcat(filename,".txt");
    
    fw = fopen(filename,"w+");
	if(!fw) {
		printf("ERROR: fail to open file %s\n",filename);
		return;
	}
	    	
	//*generate random operations
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
		strcat(command,"\n");
		fputs(command,fw);
		free(command);
		i++;
	}
	fclose(fw);
}


