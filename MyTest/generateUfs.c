#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX 1000
#define MAX_VALUE 1000
#define RAND_NUM 1000

/*
 *argv[1]: the num of generated values
*/
int main(int argc, char *argv[]) {
    int num = atoi(argv[1]);
	srand((unsigned int)time(NULL));
	char ufs[MAX] = "";	
    int i,w,t,flag;
    int rand_list[RAND_NUM];
    
    for (i = 0; i < RAND_NUM; i++)
        rand_list[i] = i+1;
    
    for (i = 0; i < RAND_NUM; i++) {
        w = rand()%(MAX_VALUE-i)+i;
        t = rand_list[i];
        rand_list[i] = rand_list[w];
        rand_list[w] = t;
    }
    
    int j;
    for (i = 0; i < RAND_NUM; i++) {
        for (j = i+1; j < RAND_NUM; j++) 
            if (rand_list[i] == rand_list[j]) break;
        if (j < RAND_NUM) break;
    }	
	if (i < RAND_NUM) {
	    printf("generate same item!");
	    return -1;
	}

	for (i = 0; i < num; i++) { 
	    char str_rand_list[10];
		flag = rand()/(double)RAND_MAX*2;

		sprintf(str_rand_list,"%d",rand_list[i]);	
		strcat(ufs,str_rand_list);

		if (i != num-1) {
			if (!flag) strcat(ufs,",");
			else strcat(ufs,"/");
		}
	}
	strcat(ufs,"\n");
	FILE *fw = fopen("/home/xue/ufs.txt","a+");
	if (!fw) {
		printf("ERROR: Fail to open file!");
		return -1;
	}	
	fputs(ufs,fw);
	fclose(fw);
	
	printf("%s",ufs);
	return 0;
}

    
