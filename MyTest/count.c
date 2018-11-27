#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <hiredis/hiredis.h>

#define MAX_SERVER_NUM 20

char *delstr(char *src, const char *sub);

int main() {
	DIR *dir;
	struct dirent *ptr;
	double duration;
	double max_duration;
	int otnum;
	int max_otnum;
	sds temp;
	
    int file_num = 0;
	int num;
	int result[MAX_SERVER_NUM];
	
	char *str_duration = "duration";
	char str[50];
    printf("str(needed to count):");
    fgets(str,50,stdin);
    str[strlen(str)-1]='\0';
  
	FILE *fr,*fw;
    char filername[50] = "/home/xue/log/";
    char filewname[50] = "/home/xue/output/";
	
    if( (dir=opendir("/home/xue/log")) == NULL ) 
    {
        printf( "There is no log file in /home/xue/log\n");
        return 0;
    }
    else{
        while ((ptr=readdir(dir)) != NULL) { 
            if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0) continue;

			printf("file: %s  ",ptr->d_name);
			//**
			strcat(filername,ptr->d_name);
			strcat(filewname,ptr->d_name);
    
			fr = fopen(filername,"r+");
			fw = fopen(filewname,"w+");
    
			if(!fr || !fw) {
				printf("ERROR: fail to open file\n");
				return -1;
			}
            num = 0;
            max_otnum = 0;
            max_duration = 0;
			char line[3000];
			while(!feof(fr)) {
				fgets(line,1024,fr);
				
				if (strstr(line,str)) {
				    temp = sdsnew(line);
				    temp = sdstrim(temp,"\t\r\n");	
				    //printf("otnum:%s\n",(strstr(temp,str)+strlen(str)+2));				    
                    otnum = atoi(strstr(temp,str)+strlen(str)+2);
                    if (otnum > max_otnum) max_otnum = otnum;
    				num++;
    				sdsfree(temp);
				}
			
				if (strstr(line,str_duration)) {
				    temp = sdsnew(line);
				    temp = sdstrim(temp,"\t\r\n");	
				    //printf("duration:%s\n",(strstr(temp,str_duration)+strlen(str_duration)+1));				    
                    duration = atof(strstr(temp,str_duration)+strlen(str_duration)+1);
                    if (duration > max_duration) max_duration = duration;
    				sdsfree(temp);
				}	
						
				if (strstr(line,"cmd: union") || strstr(line,"cmd: split")) fputs(line,fw);

				if (strstr(line,"REDIS BUG REPORT START")) break;
			}
			result[file_num] = num;
			file_num++;
			
			printf("ot calls: %d, max_otnum: %d, max_duration: %.2fms\n",num,max_otnum,max_duration/1000);

			fclose(fr);
			fclose(fw);	
						
			delstr(filername,ptr->d_name);
			delstr(filewname,ptr->d_name);		
        }
    } 
    closedir(dir);
	        
	FILE *f = fopen("/home/xue/r.csv","at");
    if (!f) {
   	    printf("ERROR: fail to open file\n");
  	    return -1;
   	}
        	
    fprintf(f,"\t\t1\t1\t\t\t\t\t\t\t\t\n");
	fclose(f);
	
	/**
	printf("num(%s): ",str);
	for (int i = 0; i < file_num; i++) {
	    if (i < file_num-1)	printf("%d+",result[i]);
	    else printf("%d\n",result[i]);
    }
    **/
	return 0;
}

char *delstr(char *src, const char *sub) {
    char *st = src, *s1 = NULL;
    const char *s2 = NULL;
    while (*st&& *sub)
    {
        s1 = st;
        s2 = sub;
        while (*s1 && *s2 &&!(*s1 - *s2))
        {
            s1++;
            s2++;
        }
        if (!*s2)
        {
            while (*st++=*s1++);
            st = src;  
        }
        st++;
    }
    return (src);
}

