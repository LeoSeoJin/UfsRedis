#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <hiredis/hiredis.h>

#define MAX_SERVER_NUM 20

char *delstr(char *src, const char *sub);
void replaceChar(char *string, char oldChar, char newChar);

int main(int argc, char *arg[]) {
	DIR *dir;
	struct dirent *ptr;
	double duration;
	double max_duration;
	int otnum;
	int max_otnum;
	sds temp;
		
	double max_all_duration = 0;
	int max_all_otnum = 0;
    int max_otcalls = 0;
    
	int num;

	char *str_duration = "duration";
	/**
	char str[50];
    printf("str(needed to count):");
    fgets(str,50,stdin);
    str[strlen(str)-1]='\0';
    **/
    char str[50] = "otnum";
    
	FILE *fr,*fw;
    char filername[50] = "/home/xue/log/";
    char filewname[50] = "/home/xue/output/";
	char line[3000];
	
    if( (dir=opendir("/home/xue/log")) == NULL ) 
    {
        printf( "There is no log file in /home/xue/log\n");
        return 0;
    }
    else{
        int file_num = 0;             
		
        while ((ptr=readdir(dir)) != NULL) { 
            if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0) continue;

			printf("file: %s  ",ptr->d_name);
            file_num++;
            
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
						
				//if (strstr(line,"cmd: union") || strstr(line,"cmd: split")) fputs(line,fw);
				if (strstr(line,"duration")) fputs(line,fw);

				if (strstr(line,"REDIS BUG REPORT START")) break;
			}
			
			if (num > max_otcalls) max_otcalls = num;
			if (max_otnum > max_all_otnum) max_all_otnum = max_otnum;
			if (max_duration > max_all_duration) max_all_duration = max_duration;
			
			printf("ot calls: %d, max_otnum: %d, max_duration: %.2fms\n",num,max_otnum,max_duration/1000); 		    
		    		
			fclose(fr);
			fclose(fw);	

        	FILE *fot;
        	if (strstr(ptr->d_name,"6379")) {
        	    fot = fopen("/home/xue/ot_output/6379.csv","a+");
        	} else if (strstr(ptr->d_name,"6380")) {
        	    fot = fopen("/home/xue/ot_output/6380.csv","a+");
        	} else if (strstr(ptr->d_name,"6381")) {
        	    fot = fopen("/home/xue/ot_output/6381.csv","a+");
        	} else if (strstr(ptr->d_name,"6382")) {
        	    fot = fopen("/home/xue/ot_output/6382.csv","a+");
        	} else if (strstr(ptr->d_name,"6383")) {
        	    fot = fopen("/home/xue/ot_output/6383.csv","a+");
        	} else if (strstr(ptr->d_name,"6384")) {
        	    fot = fopen("/home/xue/ot_output/6384.csv","a+");
        	} else if (strstr(ptr->d_name,"6385")) {
        	    fot = fopen("/home/xue/ot_output/6385.csv","a+");
        	} else if (strstr(ptr->d_name,"6386")) {
        	    fot = fopen("/home/xue/ot_output/6386.csv","a+");
        	} else if (strstr(ptr->d_name,"6387")) {
        	    fot = fopen("/home/xue/ot_output/6387.csv","a+");
        	} else if (strstr(ptr->d_name,"6388")) {
        	    fot = fopen("/home/xue/ot_output/6388.csv","a+");
        	} else if (strstr(ptr->d_name,"6389")) {
        	    fot = fopen("/home/xue/ot_output/6389.csv","a+");
        	} else if (strstr(ptr->d_name,"6390")) {
        	    fot = fopen("/home/xue/ot_output/6390.csv","a+");
        	} else if (strstr(ptr->d_name,"6391")) {
        	    fot = fopen("/home/xue/ot_output/6391.csv","a+");
        	} else {
        	}
        	        	
        	if (!fot) {
        	    printf("ERROR: fail to open file\n");
        	    return -1;
        	}
        	
		    fprintf(fot,"\t\t\t\t%s\t%d\t%d\t%.2f\n",arg[1],num,max_otnum,max_duration/1000);
	        fclose(fot);
	                    					
			delstr(filername,ptr->d_name);
			delstr(filewname,ptr->d_name);	
        }
        printf("client_num: %d\n",file_num-1);
 		char str_max_otcalls[30];
		char str_max_all_otnum[30];
		char str_max_all_duration[30];
		//itos(max_otcalls,str_max_otcalls,10);
		//itos(max_all_otnum,str_max_all_otnum,10);
		//gcvt(max_all_duration,10,str_max_all_duration);
		sprintf(str_max_otcalls,"%d",max_otcalls);
		sprintf(str_max_all_otnum,"%d",max_all_otnum);
		sprintf(str_max_all_duration,"%.2f",max_all_duration/1000);
		
       	fw = fopen("/home/xue/remote.csv","r+");
       	if (!fw) {
       	    printf("ERROR: fail to open file\n");
       	    return -1;
       	}
       	
       	int linelen = 0;
       	while (!feof(fw)) {
       	    fgets(line,1000,fw);
       	    if(feof(fw)) { 
       	        linelen = strlen(line); 	   
                replaceChar(line,'\n',' ');          
           	    strcat(line,"\t");
           	    strcat(line,str_max_otcalls);
           	    strcat(line,"\t");
           	    strcat(line,str_max_all_otnum);
           	    strcat(line,"\t");
           	    strcat(line,str_max_all_duration);
           	    strcat(line,"\n");
           	    //fputs(line,fw);	        
           	}
	    }	    
	    fclose(fw); 
	    fw = fopen("/home/xue/remote.csv","rb+");
	    fseek(fw,-1L*linelen,SEEK_END);  
	    fwrite(line,strlen(line),1,fw);
	    fclose(fw);
    } 
    closedir(dir);
	        
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

char *itos(int value, char *string, int radix) {
    char zm[37]="0123456789abcdefghijklmnopqrstuvwxyz";  
    char aa[100]={0};  
  
    int sum=value;  
    char *cp=string;  
    int i=0;  

    while(sum>0)  
    {  
        aa[i++]=zm[sum%radix];  
        sum/=radix;  
    }  
  
    for(int j=i-1;j>=0;j--)  
    {  
        *cp++=aa[j];  
    }  
    *cp='\0';  
    return string;  
}
