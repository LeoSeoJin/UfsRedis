#include "server.h"
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>

int checkUnionArgv(client *c, robj **argv);
int checkSplitArgv(client *c, robj **argv);

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

cudGraph *getUfs(sds key) {
	listNode *ln;
    listIter li;
	
	listRewind(server.ufslist,&li);
    while((ln = listNext(&li))) {
        cudGraph *ufs = ln->value;
        if (!sdscmp(ufs->key,key)) return ufs;
    }
    
    serverLog(LL_LOG,"ufs with key %s does not exist!!",key);
    return NULL;
    
}

void bubbleSort(cudGraph *ufs, sds *list, int len) {
	sds temp;
	for(int i = 0; i < len - 1; i++) {
        for(int j = 0; j < len - i -1; j++) {
            if(locateVex(ufs,list[j]) > locateVex(ufs,list[j + 1])) {
                temp = list[j];

                list[j] = list[j + 1];
                list[j + 1] = temp;
            }
    	}
    }
}

void insertSort(cudGraph *ufs, sds *list, int len) {
	sds temp;
	int i, j;
	for (i = 1; i < len; i++) {    
		temp = list[i];    
		j = i;    
		while ((j > 0) && (locateVex(ufs,list[j-1]) > locateVex(ufs,temp))) {    
			list[j] = list[j - 1];    
            --j;
        }
            
        list[j] = temp;    
    }
}

int partition(cudGraph *ufs, sds *list, int low, int high) {
	sds temp = list[low];
	
	while (low < high) {
		while (low < high && locateVex(ufs,list[high]) >= locateVex(ufs,temp)) high--;

		list[low] = list[high];
		while (low < high && locateVex(ufs,list[low]) <= locateVex(ufs,temp)) low++;
		list[high] = list[low];
	}
	list[low] = temp;
	return low;
}

void quickSort(cudGraph *ufs, sds *list, int low, int high) {
	if (low >= high) return;
	
	int pivotloc = partition(ufs,list,low,high);
	quickSort(ufs,list,low,pivotloc-1);
	quickSort(ufs,list,pivotloc+1,high);	
}

int locateVex(cudGraph *ufs, char *e) {   
    for(int i = 0; i < ufs->vertexnum; i++)  
        if(!strcmp(e, ufs->dlist[i].data))  return i;  
    return -1;  
}

/*edge from v1 to v2*/
int existEdge(cudGraph *ufs,int v1, int v2) {
	edge *p = ufs->dlist[v1].firstedge;
	while(p != NULL) {
		if (p->adjvex == v2) return 1;
		p = p->nextedge;
	}
	return 0;
}

int existElement(cudGraph *ufs, sds e) {
    for(int i = 0; i < ufs->vertexnum; i++) { 
        if(!sdscmp(e, ufs->dlist[i].data))  return 1; 
    } 
    return 0; 	
}

int existElements(cudGraph *ufs, sds *list, int len) {
    int i,j;
	for (i = 0; i < len; i++) {
       for(j = 0; j < ufs->vertexnum; j++)  
          if(!sdscmp(list[i], ufs->dlist[j].data)) break;
       if (j == ufs->vertexnum) break;
	}
	
	if (i == len) return 1;
	return 0;
}

int isConnected(cudGraph *ufs, sds *elements, int len) {
	for (int i = 0; i < len; i++) {
		for (int j  = i + 1; j < len; j++) {
			if (!existEdge(ufs,locateVex(ufs,elements[i]),locateVex(ufs,elements[j]))) return 0;
		}
	}
	return 1;
}

void createEdge(cudGraph *ufs,int v1, int v2) {
    if (v1 == v2 || (existEdge(ufs,v1,v2) && existEdge(ufs,v2,v1))) {
    	return;
    } else {
    	if (!existEdge(ufs,v1,v2)) {
    		edge *p = zmalloc(sizeof(edge));  
        	p->adjvex = v2;
        	p->nextedge = ufs->dlist[v1].firstedge;   
        	ufs->dlist[v1].firstedge = p; 	
 			ufs->edgenum++;
    	} 
    	if (!existEdge(ufs,v2, v1)) {
        	edge *q = zmalloc(sizeof(edge));  
        	q->adjvex = v1; 
        	q->nextedge = ufs->dlist[v2].firstedge; 
        	ufs->dlist[v2].firstedge = q;
        	ufs->edgenum++;    
    	}
    }	
}

void createLinks(cudGraph *ufs, int i, int j) {
    createEdge(ufs,i, j);
	edge *p = ufs->dlist[i].firstedge;
	edge *q;
	while(p != NULL) {
		q = ufs->dlist[j].firstedge;
		while(q != NULL) {
			if (p->adjvex != j) createEdge(ufs,p->adjvex,j);
			createEdge(ufs,p->adjvex, q->adjvex);
			q = q->nextedge;
		}
		p = p->nextedge;
	}

	p = ufs->dlist[j].firstedge;
	while(p != NULL) {
		q = ufs->dlist[i].firstedge;
		while(q != NULL) {
		    if (p->adjvex != i) createEdge(ufs,p->adjvex,i);
			createEdge(ufs,p->adjvex, q->adjvex);
			q = q->nextedge;
		}
		p = p->nextedge;
	}
}

void removeEdge(cudGraph *ufs, sds v1, char *v2) {
    int i = locateVex(ufs, v1);
	int j = locateVex(ufs, v2);
		
	edge *p, *q;
	p = ufs->dlist[i].firstedge;
	while (p && p->adjvex != j) {
		q = p;
		p = p->nextedge;
	}
	if (p && p->adjvex == j) {
		if (p == ufs->dlist[i].firstedge)
			ufs->dlist[i].firstedge = p->nextedge;
		else 
			q->nextedge = p->nextedge;
		zfree(p);
		ufs->edgenum--;
	}
	
	p = ufs->dlist[j].firstedge;
	while (p && p->adjvex != i) {
		q = p;
		p = p->nextedge;
	}
	if (p && p->adjvex == i) {
		if (p == ufs->dlist[j].firstedge)
			ufs->dlist[j].firstedge = p->nextedge;
		else 
			q->nextedge = p->nextedge;
		zfree(p);
		ufs->edgenum--;
	}
}

void removeAdjEdge(cudGraph *ufs, char *v, sds *list, int len) {
	int i = locateVex(ufs, v);
	edge *p = ufs->dlist[i].firstedge;
	if (p != NULL) {
		edge *q = NULL;
		while (p != NULL) {
			if (!list || !find(list, ufs->dlist[p->adjvex].data, len)) {
				if (p == ufs->dlist[i].firstedge) {
					ufs->dlist[i].firstedge = p->nextedge;
					zfree(p);
					p = ufs->dlist[i].firstedge;
				} else {
					q->nextedge = p->nextedge;
					zfree(p);
					p = q->nextedge;
				}
			} else {
				q = p;
			 	p = p->nextedge;
			} 
		}
		for (int j = 0; j < ufs->vertexnum; j++) {
			p = ufs->dlist[j].firstedge;
			while (p != NULL) {
				if (p->adjvex == i) {
					if (!list || !find(list, ufs->dlist[j].data, len)) {
						if (p == ufs->dlist[j].firstedge) {
							ufs->dlist[j].firstedge = p->nextedge;
							zfree(p);
							p = ufs->dlist[j].firstedge;

						} else {
							q->nextedge = p->nextedge;
							zfree(p);
							p = q->nextedge;
						}
					} else {
						q = p;
					 	p = p->nextedge;
					} 
				} else {
						q = p;
						p = p->nextedge;
				}
			}
		}
	}
}

void updateVerticeUfsFromState(char *u, int type, char *argv1, char *argv2, vertice *v) {
    int len, i;
    sds v_ufs;
    sds temp = NULL;
    sds *elements = sdssplitlen(u,strlen(u),"/",1,&len);
    //for (int j = 0; j < len; j++) serverLog(LL_LOG,"elements[%d] %s",j,elements[j]);
   
    if (type == OPTYPE_UNION) {
	   	//serverLog(LL_LOG,"updaeVerticeState from %s union %s %s",u,argv1,argv2);
		if (!strcmp(argv1,"*") || !strcmp(argv2,"*")) {
			v_ufs = sdsempty();
		} else {
            //union operation
            int argv1class = -1; 
            int argv2class = -1;
            for (i = 0; i < len; i++) {
                if (argv1class == -1 && findSds(elements[i],argv1)) argv1class = i;
                if (argv2class == -1 && findSds(elements[i],argv2)) argv2class = i;
                if (argv1class != -1 && argv2class != -1) break;
            }
    		
    		//serverLog(LL_LOG,"argv1class: %d argv2class: %d ",argv1class, argv2class);
            //for (int j = 0; j < len; j++) serverLog(LL_LOG,"elements[%d] %s",j,elements[j]);           
            
            if (argv1class != argv2class) {
            	v_ufs = sdsempty();
                int low = (argv1class < argv2class)? argv1class: argv2class;
                int high = (argv1class > argv2class)? argv1class: argv2class;
                
                temp = sdsnew(elements[low]);
                temp = sdscat(temp,",");
                temp = sdscat(temp,elements[high]);
                
                for (i = 0; i < len; i++) {
                	if (i == high) continue;
    				if (i == low) {
                        //serverLog(LL_LOG,"temp: %s",temp);
        				v_ufs = sdscat(v_ufs,temp); 
        	        } else {
        	            //serverLog(LL_LOG,"elements[%d] %s",i,elements[i]);
        	            v_ufs = sdscat(v_ufs,elements[i]);
        	        }
                    if (high != len-1) {
                        if (i != len-1) v_ufs = sdscat(v_ufs,"/");
                    } else {
                        if (i != len-2) v_ufs = sdscat(v_ufs,"/");
                    }
                }
           } else {
                v_ufs = sdsempty();
           }
        }       
   } else {
       //serverLog(LL_LOG,"updaeVerticeState from %s split %s",u,argv1);
       	if (!strcmp(argv1,"*")) {
       	    v_ufs = sdsempty();
       	} else { 
            int argvclass = -1;
    	 		
            for (i = 0; i < len; i++) {
               if (findSds(elements[i],argv1)){
                  //if (sdscntcmp(elements[i],argv1)) {
	          if (sdslen(elements[i]) != strlen(argv1)) {
                      argvclass = i; 
                  }     
                  break;
               } 
            }
            //serverLog(LL_LOG,"argvclass: %d",argvclass);
            //for (int j = 0; j < len; j++) serverLog(LL_LOG,"elements[%d] %s",j,elements[j]);
            
            if (argvclass != -1) {  
                temp = sdsnew(elements[argvclass]);
                temp = sdsDel(temp,argv1);            
                
            	v_ufs = sdsempty();
                for (i = 0; i < len; i++) {
                    if (i != argvclass) {
                        v_ufs = sdscat(v_ufs,elements[i]);
                        if (i != len-1) v_ufs = sdscat(v_ufs,"/");
                    } else {
                        v_ufs = sdscat(v_ufs,temp);
                        v_ufs = sdscat(v_ufs,"/");
                        v_ufs = sdscat(v_ufs,argv1);
                        if (argvclass != len-1) v_ufs = sdscat(v_ufs,"/");
                    }  
                }                
            } else {
               v_ufs = sdsempty();
            }
        }
   }
   
   if (sdslen(v_ufs) == 0) {
       v->content = u;    
   } else {    
       v->content = (char*)zmalloc(sdslen(v_ufs)+1);
       memcpy(v->content,v_ufs,sdslen(v_ufs));
       v->content[sdslen(v_ufs)] = '\0';
   }
   //serverLog(LL_LOG,"updateVerticeState(end): v_ufs %s len: %lu v->content %s len %lu",v_ufs,sdslen(v_ufs),v->content,strlen(v->content));
   
   //for (i = 0; i < len; i++) sdsfree(elements[i]);
   sdsfreesplitres(elements,len);
   sdsfree(v_ufs);
   sdsfree(temp);
}

void updateVerticeUfs(cudGraph *ufs, vertice *v) { 
	int i,j;
	int skip = 0;
	int flag = 0;
	sds list;
	sds result = sdsempty();
	
	for (i = 0; i < ufs->vertexnum; i++) {
		for (j = 0; j < i; j++) {
			if (existEdge(ufs,i, j)) {
				skip++;	
				if (i == ufs->vertexnum-1) flag = 1;
				break;
			}
		}
		if (j < i) continue;
		//list = sdsempty();
		//list = sdscat(list, ufs->dlist[i].data);
		list = sdsnew(ufs->dlist[i].data);
		list = sdscat(list,",");
		
		if (ufs->dlist[i].firstedge != NULL) {
			edge *p = ufs->dlist[i].firstedge;
			while(p != NULL) {
				list = sdscat(list, ufs->dlist[p->adjvex].data);
				if (p->nextedge) list = sdscat(list,","); 
				p = p->nextedge;
			}
		} else {
			//has no adjacent vertices
			list = sdstrim(list,",");
		}

		//sort the list by increasing adjvex of vertexex
		int len;
		sds *elements = sdssplitlen(list,sdslen(list),",",1,&len);

		//bubbleSort(ufs,elements,len);
		//insertSort(ufs,elements,len);
		//quickSort(ufs,elements,0,len-1);
		sdsclear(list);

		for (j = 0; j < len; j++) {
			list = sdscat(list,elements[j]);
			if (j != len-1) list = sdscat(list,",");
		}
		
		result = sdscat(result,list);
		if (i != (ufs->vertexnum - 1)) {
			result = sdscat(result,"/"); 
		}
		sdsfreesplitres(elements,len);
		sdsfree(list);
	}
	if ((skip == ufs->vertexnum - 1) || flag == 1) sdsrange(result,0,sdslen(result)-2);
	
	v->content = (char*)zmalloc(sdslen(result)+1);
	strcpy(v->content,result);	
	sdsfree(result);
	//serverLog(LL_LOG,"updateVerticeUfs(end): %s",v->content);
}

void unionProc(client *c, sds argv1, sds argv2) {
	int i,j;
	int argv1len = strchr(argv1,',')==NULL?1:2;
	int argv2len = strchr(argv2,',')==NULL?1:2;
	cudGraph *ufs = getUfs(c->argv[c->argc-1]->ptr);
	//serverLog(LL_LOG,"unionProc arguments num: %d %d ", argv1len, argv2len);
	
	/*case1: argv1 is a set and argv2 is a singleton element
	 *case2: argv2 is a set and argv1 is a singleton element
	 *case3: argv1 and argv2 are both sets
	 *case4: argv1 and argv2 are both singleton elements
	 *special case: argv1 or argv2 is NULL (generated by ot)*/
	if (!strcmp(argv1,"*") || !strcmp(argv2,"*")) {
		c->flag_ufs = 1;
		addReply(c,shared.ok); //this case only happens when receiving o from server or finishing OT process locally
	} else {
    	if (argv1len > 1 && argv2len == 1) {
    		sds *elements;
    		int len;
    		
    		elements = sdssplitlen(argv1,strlen(argv1),",",1,&len);
    		if (!existEdge(ufs,locateVex(ufs,elements[0]),locateVex(ufs,argv2))) {
   				for (i = 0; i < len; i++) 
   					createLinks(ufs,locateVex(ufs, elements[i]), locateVex(ufs, argv2));
   			}
   		    c->flag_ufs = 1;
   			addReply(c,shared.ok);
    		sdsfreesplitres(elements,len);
    	} else if (argv2len > 1 && argv1len == 1){
    		sds *elements;
    		int len;
    		
    		elements = sdssplitlen(argv2,strlen(argv2),",",1,&len);
   			if (!existEdge(ufs,locateVex(ufs,elements[0]),locateVex(ufs,argv1))) {
   				for (i = 0; i < len; i++) 
   					createLinks(ufs,locateVex(ufs, elements[i]), locateVex(ufs, argv1));
   			}					
   			c->flag_ufs = 1;
   			addReply(c,shared.ok);
    		sdsfreesplitres(elements,len);
    	} else if (argv1len > 1 && argv2len > 1) {
    		sds *elements1, *elements2;
    		int len1, len2;
    
    		elements1 = sdssplitlen(argv1,strlen(argv1),",",1,&len1);
    		elements2 = sdssplitlen(argv2,strlen(argv2),",",1,&len2);
       		
    		if (!existEdge(ufs,locateVex(ufs,elements1[0]),locateVex(ufs,elements2[0]))) {
    			for (i = 0; i < len1; i++) {
    				for (j = 0; j < len2; j++) {
    					createLinks(ufs,locateVex(ufs,elements1[i]),locateVex(ufs,elements2[j]));
    				}
    			}
    		}	
    		c->flag_ufs = 1;
    		addReply(c,shared.ok);
    		
    		sdsfreesplitres(elements1,len1);
    		sdsfreesplitres(elements2,len2);
    	} else {    		
    		//serverLog(LL_LOG,"union x x");   		
    	    createLinks(ufs,locateVex(ufs, argv1),locateVex(ufs, argv2));
    		c->flag_ufs = 1;
    		addReply(c,shared.ok);
    	}
   } 
}

void splitProc(client *c, sds argv1) {
	cudGraph *ufs = getUfs(c->argv[c->argc-1]->ptr);
	if (strchr(argv1,',') != NULL){
		sds *elements;
		int len;
	
		elements = sdssplitlen(argv1,strlen(argv1),",",1,&len);

		for (int i = 0; i < len; i++) 
			removeAdjEdge(ufs,elements[i], elements, len);
		
		c->flag_ufs = 1;
		addReply(c,shared.ok);
		sdsfreesplitres(elements,len);
	} else {
	    //argument may empty, just do nothing locally for this case
		if (strcmp(argv1,"*")) removeAdjEdge(ufs,argv1, NULL, 0); 
		c->flag_ufs = 1;
		addReply(c,shared.ok);
	}	
}

int checkArgv(client *c, robj **argv) {
    if (!strcmp(c->argv[0]->ptr,"union")) {
        return checkUnionArgv(c, argv);
    } else {
        return checkSplitArgv(c, argv);
    }
}

int checkUnionArgv(client *c, robj **argv) {
	int argv1len = strchr(argv[1]->ptr,',')==NULL?1:2;
	int argv2len = strchr(argv[2]->ptr,',')==NULL?1:2;
	cudGraph *ufs = getUfs(c->argv[c->argc-1]->ptr);
	//serverLog(LL_LOG,"check union argv");
	
	/*case1: argv1 is a set and argv2 is a singleton element
	 *case2: argv2 is a set and argv1 is a singleton element
	 *case3: argv1 and argv2 are both sets
	 *case4: argv1 and argv2 are both singleton elements
	 *special case: argv1 or argv2 is NULL (generated by ot)*/
	if (!strcmp(argv[1]->ptr,"*") || !strcmp(argv[2]->ptr,"*")) {
	    if (strcmp(argv[1]->ptr,"*") && !existElement(ufs,argv[1]->ptr)) {
            c->flag_ufs = 0;
            addReply(c,shared.argvnoexistallelems);
            return Argv_ERR;
	    } 
	    if (strcmp(argv[2]->ptr,"*") && !existElement(ufs,argv[2]->ptr)) {
            c->flag_ufs = 0;
            addReply(c,shared.argvnoexistallelems);
            return Argv_ERR;
	    } 
        return Argv_OK;
	} else {
    	if (argv1len > 1 && argv2len == 1) {
	    	sds *elements;
		    int len;
		
    		elements = sdssplitlen(argv[1]->ptr,strlen(argv[1]->ptr),",",1,&len);
    		/*check whether elements of argv1 are in one equivalent class*/
    		if (!isConnected(ufs,elements,len)) {
    			c->flag_ufs = 0;
    			addReply(c,shared.argv1nosameclass);
    			return Argv_ERR;
    		} else {
    		    if (!existElements(ufs,elements,len) || !existElement(ufs,argv[2]->ptr)) {
                    c->flag_ufs = 0;
                    addReply(c,shared.argvnoexistallelems);
                    return Argv_ERR;
    		    }
    			return Argv_OK;
    		}
    		sdsfreesplitres(elements,len);
    	} else if (argv2len > 1 && argv1len == 1){
    		sds *elements;
    		int len;
    		
    		elements = sdssplitlen(argv[2]->ptr,strlen(argv[2]->ptr),",",1,&len);
    		/*check whether elements of argv2 are in one equivalent class*/
    		if (!isConnected(ufs,elements,len)) {
    			c->flag_ufs = 0;
    			addReply(c,shared.argv2nosameclass);
    			return Argv_ERR;
    		} else {
    		    if (!existElements(ufs,elements,len) || !existElement(ufs,argv[1]->ptr)) {
                    c->flag_ufs = 0;
                    addReply(c,shared.argvnoexistallelems);
                    return Argv_ERR;
    		    }    		
    			return Argv_OK;
    		}
    		sdsfreesplitres(elements,len);
    	} else if (argv1len > 1 && argv2len > 1) {
    		sds *elements1, *elements2;
    		int len1, len2;
    
    		elements1 = sdssplitlen(argv[1]->ptr,strlen(argv[1]->ptr),",",1,&len1);
    		elements2 = sdssplitlen(argv[2]->ptr,strlen(argv[2]->ptr),",",1,&len2);
    		
    		if (!isConnected(ufs,elements1,len1) && !isConnected(ufs,elements2,len2)) {
    			c->flag_ufs = 0;
    			addReply(c,shared.argv12nosameclass);
    			return Argv_ERR;
    		} else if (!isConnected(ufs,elements1,len1)) {
    			c->flag_ufs = 0;
    			addReply(c,shared.argv1nosameclass);
    			return Argv_ERR;
    		} else if (!isConnected(ufs,elements2,len2)) {
    			c->flag_ufs = 0;
    			addReply(c,shared.argv2nosameclass);
    			return Argv_ERR;
    		} else {
    		    if (!existElements(ufs,elements1,len1) || !existElements(ufs,elements2,len2)) {
                    c->flag_ufs = 0;
                    addReply(c,shared.argvnoexistallelems);
                    return Argv_ERR;
    		    }
    			return Argv_OK;
    		}
    		sdsfreesplitres(elements1,len1);
    		sdsfreesplitres(elements2,len2);
    	} else {
    	    if (!existElement(ufs, argv[1]->ptr) || !existElement(ufs, argv[2]->ptr)) {
                c->flag_ufs = 0;
                addReply(c, shared.argvnoexistallelems);
                return Argv_ERR;
    	    }
    		return Argv_OK;
    	}
   } 
}

int checkSplitArgv(client *c, robj **argv) {
	cudGraph *ufs = getUfs(c->argv[c->argc-1]->ptr);
	if (strchr(argv[1]->ptr,',')!=NULL) {
		sds *elements;
		int len;
	
		elements = sdssplitlen(argv[1]->ptr,strlen(argv[1]->ptr),",",1,&len);
	
		if (!isConnected(ufs,elements,len)) {
			c->flag_ufs = 0;
			addReply(c,shared.argvsplitnosameclass);
			return Argv_ERR;
		} else {
		    if (!existElements(ufs,elements,len)) {
                c->flag_ufs = 0;
                addReply(c, shared.argvnoexistallelems);
                return Argv_ERR;
		    }
			return Argv_OK;
		}
		sdsfreesplitres(elements,len);
	} else {
		if (!existElement(ufs,argv[1]->ptr)) {
			c->flag_ufs = 0;
			addReply(c,shared.argvnoexistelem);
			return Argv_ERR;
		}
	    return Argv_OK;
	}	
}

void controlAlg(client *c) {
	if (server.masterhost) {
		list *space = getSpace(c->argv[c->argc-1]->ptr);
		if (!(c->flags & CLIENT_MASTER)) {
			/*local processing: new locally generated operation*/
			if (checkArgv(c,c->argv) == Argv_ERR) return; 

			cudGraph *ufs = getUfs(c->argv[c->argc-1]->ptr); /*op argv1 argv2 tag1*/
			op_num++;
			sds oid = sdsempty();		

            sds oids = sdsempty();
			oids = sdscat(oids,"init");
			vertice *v;
           
            v = locateVertice(space, NULL);
            oids = calculateOids(space->head->value,oids);          
            			
			char buf1[Port_Num+1] = {0};
			char buf2[Max_Op_Num] = {0};
			oid = sdscat(oid,itos(server.port-6378,buf1,10));
			oid = sdscat(oid,"_");
			oid = sdscat(oid,itos(op_num,buf2,10));
	        
			vertice *v1 = createVertice();
			listAddNodeTail(space,v1);
			
			char *a1 = NULL;
			char *a2 = NULL;
			
			char *coid = (char *)zmalloc(sdslen(oid)+1);
			memcpy(coid,oid,sdslen(oid));
			coid[sdslen(oid)]='\0';
			
			if (!strcmp(c->argv[0]->ptr,"union")) {
			    a1 = (char*)zmalloc(sdslen(c->argv[1]->ptr)+1);
			    memcpy(a1,c->argv[1]->ptr,sdslen(c->argv[1]->ptr));
			    a1[sdslen(c->argv[1]->ptr)] = '\0';
			    a2 = (char*)zmalloc(sdslen(c->argv[2]->ptr)+1);
			    memcpy(a2,c->argv[2]->ptr,sdslen(c->argv[2]->ptr));
			    a2[sdslen(c->argv[2]->ptr)] = '\0';
				v->ledge = createOpEdge(OPTYPE_UNION,a1,a2,coid,v1); 
			} else {
			    a1 = (char*)zmalloc(sdslen(c->argv[1]->ptr)+1);
			    memcpy(a1,c->argv[1]->ptr,sdslen(c->argv[1]->ptr));
			    a1[sdslen(c->argv[1]->ptr)] = '\0';			
				v->ledge = createOpEdge(OPTYPE_SPLIT,a1,NULL,coid,v1);
			}

			/*propagate to master: construct and rewrite cmd of c
			 *(propagatetomaster will call replicationfeedmaster(c->argv,c->argc))
			 *union x y oid ctx or split x oid ctx(just for propagation)
			 */
			if (c->argc == 4) {
			    //union x y oid ctx key
                robj *argv[3];   	    	    	
    	    	argv[0] = createStringObject(v->ledge->oid,strlen(v->ledge->oid)); //oid
    		    argv[1] = createStringObject(oids,sdslen(oids)); //oids
                argv[2] = createStringObject(c->argv[3]->ptr,sdslen(c->argv[3]->ptr));  //key  		    
   
    		    rewriteClientCommandArgument(c,3,argv[0]);
    		    rewriteClientCommandArgument(c,4,argv[1]);
    	        rewriteClientCommandArgument(c,5,argv[2]); 
                //decrRefCount(argv[0]);
                //decrRefCount(argv[1]);
                //decrRefCount(argv[2]);
                    	        
                //serverLog(LL_LOG,"rewriteClientCommandArgument, union, %s %s refcount: %d",v->ledge->oid, oids,argv[0]->refcount);
                //serverLogCmd(c);       
			} else {
                //split x oid ctx key
                robj *argv[3];   	    	    	
    	    	argv[0] = createStringObject(v->ledge->oid,strlen(v->ledge->oid)); //oid
    		    argv[1] = createStringObject(oids,sdslen(oids)); //oids
                argv[2] = createStringObject(c->argv[2]->ptr,sdslen(c->argv[2]->ptr));  //key  		    
   
    		    rewriteClientCommandArgument(c,2,argv[0]);
    		    rewriteClientCommandArgument(c,3,argv[1]);
    	        rewriteClientCommandArgument(c,4,argv[2]); 
                //decrRefCount(argv[0]);
                //decrRefCount(argv[1]);
                //decrRefCount(argv[2]);
                
	    		//serverLog(LL_LOG,"rewriteClientCommandArgument, split, %s %s refcount: %d",v->ledge->oid, oids,argv[0]->refcount);
                //serverLogCmd(c);      
			}
			sdsfree(oid);
			sdsfree(oids);
			
			//execution of local generated operation
			if (!strcmp(c->argv[0]->ptr, "union")) unionProc(c, c->argv[1]->ptr,c->argv[2]->ptr); 
            if (!strcmp(c->argv[0]->ptr, "split")) splitProc(c, c->argv[1]->ptr);
			
            //update ufs of vertice
            if (v->ledge) {
            	updateVerticeUfs(ufs,v1);
			}
		} else {
			/*remote processing: op argv1 (argv2) oid oids tag1*/
			sds ctx;
			char *a1 = NULL;
			char *a2 = NULL;
            char *oid;				
			if (c->argc == 6) {            
			    ctx = c->argv[4]->ptr;
			    oid = (char *)zmalloc(sdslen(c->argv[3]->ptr)+1);
    			memcpy(oid,c->argv[3]->ptr,sdslen(c->argv[3]->ptr));
    			oid[sdslen(c->argv[3]->ptr)]='\0';		    	
			    a1 = (char*)zmalloc(sdslen(c->argv[1]->ptr)+1);
			    memcpy(a1,c->argv[1]->ptr,sdslen(c->argv[1]->ptr));
			    a1[sdslen(c->argv[1]->ptr)] = '\0';
			    a2 = (char*)zmalloc(sdslen(c->argv[2]->ptr)+1);
			    memcpy(a2,c->argv[2]->ptr,sdslen(c->argv[2]->ptr));
			    a2[sdslen(c->argv[2]->ptr)] = '\0';		    	
			} else {			
    			ctx = c->argv[3]->ptr;
			    oid = (char *)zmalloc(sdslen(c->argv[2]->ptr)+1);
    			memcpy(oid,c->argv[2]->ptr,sdslen(c->argv[2]->ptr));
    			oid[sdslen(c->argv[2]->ptr)]='\0';	
			    a1 = (char*)zmalloc(sdslen(c->argv[1]->ptr)+1);
			    memcpy(a1,c->argv[1]->ptr,sdslen(c->argv[1]->ptr));  		
			    a1[sdslen(c->argv[1]->ptr)] = '\0';
			}
			
			long long start = ustime();            
			vertice *u = locateVertice(space,ctx);
			long long duration = ustime()-start;
			serverLog(LL_LOG,"remote processing: locatevertice: %lld",duration);
			
			vertice *v = createVertice();			
			listAddNodeTail(space,v);

			if (!strcmp(c->argv[0]->ptr,"union")) {
			    /**
				if (strcmp(a1,c->argv[1]->ptr)) { 
				    zfree(a1);
				    a1 = NULL;
			        a1 = (char*)zmalloc(sdslen(c->argv[1]->ptr)+1);
			        memcpy(a1,c->argv[1]->ptr,sdslen(c->argv[1]->ptr));
			        a1[sdslen(c->argv[1]->ptr)] = '\0';
			    }
				if (strcmp(a2,c->argv[2]->ptr)) { 
				    zfree(a2);
				    a2 = NULL;
			        a2 = (char*)zmalloc(sdslen(c->argv[2]->ptr)+1);
			        memcpy(a1,c->argv[2]->ptr,sdslen(c->argv[2]->ptr));
			        a2[sdslen(c->argv[2]->ptr)] = '\0';
			    }
			    **/
				u->redge = createOpEdge(OPTYPE_UNION,a1,a2,oid,v); 			    			    
				updateVerticeUfsFromState(u->content,OPTYPE_UNION,c->argv[1]->ptr,c->argv[2]->ptr,v);
			} else {
			    /**
				if (strcmp(a1,c->argv[1]->ptr)) { 
				    zfree(a1);
				    a1 = NULL;
			        a1 = (char*)zmalloc(sdslen(c->argv[1]->ptr)+1);
			        memcpy(a1,c->argv[1]->ptr,sdslen(c->argv[1]->ptr));
			        a1[sdslen(c->argv[1]->ptr)] = '\0';
			    }
			    **/
				u->redge = createOpEdge(OPTYPE_SPLIT,a1,NULL,oid,v); 			    				
				updateVerticeUfsFromState(u->content,OPTYPE_SPLIT,c->argv[1]->ptr,NULL,v);
			}

			vertice *ul;
			vertice *p = v;
		
			//if (u->ledge) serverLog(LL_LOG,"ot process"); 
			int ot_num = 0;
			while (u->ledge != NULL) {
			    ot_num++;
				vertice *pl = createVertice();	
				listAddNodeTail(space,pl);
				p->ledge = createOpEdge(-1,NULL,NULL,u->ledge->oid,pl);
				
				ul = u->ledge->adjv;
				ul->redge = createOpEdge(-1,NULL,NULL,u->redge->oid,pl);

				c->cmd->otproc(u->content,u->ledge,u->redge,p->ledge, ul->redge, OT_SPLIT);

                updateVerticeUfsFromState(ul->content,ul->redge->optype,ul->redge->argv1,ul->redge->argv2,pl); 
                
				u = ul;
				p = pl;
			}
			
			if (ot_num!=0) serverLog(LL_LOG,"otnum: %d",ot_num);
			/*execute the trasformed remote operation (finish ot process)
			 *or original remote operation (u->ledge = NULL, no ot process)
			 */
            if (!strcmp(c->argv[0]->ptr, "union")) {
            	//serverLog(LL_LOG,"unionProc: %d %s %s",u->redge->optype,u->redge->argv1,u->redge->argv2);
            	unionProc(c,u->redge->argv1,u->redge->argv2);            	
            }	
            if (!strcmp(c->argv[0]->ptr, "split")) {
                //serverLog(LL_LOG,"splitProc: %d %s",u->redge->optype,u->redge->argv1);                
                splitProc(c,u->redge->argv1);
            }
		}
	} else {
		/*server processing, union x y oid ctx tag1*/
		sds ctx;
        char *a1 = NULL;
        char *a2 = NULL;
        char *oid = NULL;
        if (c->argc == 6) {            
            ctx = c->argv[4]->ptr;
            oid = (char*)zmalloc(sdslen(c->argv[3]->ptr)+1);
            memcpy(oid,c->argv[3]->ptr,sdslen(c->argv[3]->ptr));
            oid[sdslen(c->argv[3]->ptr)] = '\0';
            a1 = (char*)zmalloc(sdslen(c->argv[1]->ptr)+1);
            memcpy(a1,c->argv[1]->ptr,sdslen(c->argv[1]->ptr));
            a1[sdslen(c->argv[1]->ptr)] = '\0';
            a2 = (char*)zmalloc(sdslen(c->argv[2]->ptr)+1);
            memcpy(a2,c->argv[2]->ptr,sdslen(c->argv[2]->ptr));
            a2[sdslen(c->argv[2]->ptr)] = '\0';
        } else {            
            ctx = c->argv[3]->ptr;
            oid = (char*)zmalloc(sdslen(c->argv[2]->ptr)+1);
            memcpy(oid,c->argv[2]->ptr,sdslen(c->argv[2]->ptr));
            oid[sdslen(c->argv[2]->ptr)] = '\0';            
            a1 = (char*)zmalloc(sdslen(c->argv[1]->ptr)+1);
            memcpy(a1,c->argv[1]->ptr,sdslen(c->argv[1]->ptr));
            a1[sdslen(c->argv[1]->ptr)] = '\0';
        }
		//serverLog(LL_LOG,"receive op: %s %s %s", (char*)c->argv[0]->ptr, a1, a2);
		
		verlist *s = locateVerlist(c->slave_listening_port,c->argv[c->argc-1]->ptr);
		vertice *u = locateVertice(s->vertices,ctx);
		
		//sds oids = sdsempty();
		//oids = sdscat(oids,ctx);
		sds oids = sdsnew(ctx);
		vertice *v = createVertice();	
		listAddNodeTail(s->vertices,v);
			
		if (!strcmp(c->argv[0]->ptr,"union")) { 
			u->ledge = createOpEdge(OPTYPE_UNION,a1,a2,oid,v);
		} else {
			u->ledge = createOpEdge(OPTYPE_SPLIT,a1,a2,oid,v);
		}	

		//serverLog(LL_LOG,"create a new local operation finished, current state %s op: %d %s %s", u->content,u->ledge->optype,u->ledge->argv1,u->ledge->argv2); 	

        updateVerticeUfsFromState(u->content,u->ledge->optype,u->ledge->argv1,u->ledge->argv2,v);
        //serverLog(LL_LOG,"updaeVerticeState finish (local op): %s",v->content);
        
        //serverLog(LL_LOG,"new local edge: %d %s %s %s ",u->ledge->optype, u->ledge->argv1,u->ledge->argv2,u->ledge->oid); 
        
        //serverLog(LL_LOG,"server processing, memory (before ot): %ld ", zmalloc_used_memory());
		vertice *ur;
		vertice *p = v;
		//if (u->redge) serverLog(LL_LOG,"ot process");
		int ot_num = 0;
		while (u->redge != NULL) { 
		ot_num++; 
    	    oids = sdscat(oids,",");
    		oids = sdscat(oids,u->redge->oid);  		
		
			vertice *pr = createVertice();	
			listAddNodeTail(s->vertices,pr);
			p->redge = createOpEdge(-1,NULL,NULL,u->redge->oid,pr);
			
			ur = u->redge->adjv;
			ur->ledge = createOpEdge(-1,NULL,NULL,u->ledge->oid,pr);
			
			c->cmd->otproc(u->content,u->redge,u->ledge,p->redge,ur->ledge,OT_SPLIT);
			
			updateVerticeUfsFromState(ur->content,ur->ledge->optype,ur->ledge->argv1,ur->ledge->argv2,pr); 
			//serverLog(LL_LOG,"updaeVerticeState finish (ot process) pr->content: %s",pr->content);			
			
			u = ur;
			p = pr;
	    }	
	    //serverLog(LL_LOG,"ot process finish");
	    if (ot_num != 0) serverLog(LL_LOG,"otnum: %d",ot_num);
	    //save argv2 to each other client's space
	    listNode *ln;
        listIter li;
		
	    listRewind(sspacelist->spaces,&li);	    
        while((ln = listNext(&li))) {
            verlist *s = ln->value;
            if (sdscmp(s->key,c->argv[c->argc-1]->ptr) || s->id == c->slave_listening_port) continue;
			
            vertice *loc = locateVertice(s->vertices,NULL);
            vertice *locr = createVertice();
            listAddNodeTail(s->vertices,locr);
			
			loc->redge = createOpEdge(u->ledge->optype,u->ledge->argv1,u->ledge->argv2,u->ledge->oid,locr);

            updateVerticeUfsFromState(loc->content,loc->redge->optype,loc->redge->argv1,loc->redge->argv2,locr); 
            //serverLog(LL_LOG,"updaeVerticeState finish (other space)locr->content: %s",locr->content);
        }

    	c->flag_ufs = 1;
    	
    	/*rewrite the command argv and transform it to other clients;*/
		if (c->argc == 6) {
	    	//union x y oid ctx tag1	        
	        robj *argv[3];   	
	    	argv[0] = createStringObject(u->ledge->argv1,strlen(u->ledge->argv1));
	    	argv[1] = createStringObject(u->ledge->argv2,strlen(u->ledge->argv2));
		    argv[2] = createStringObject(oids,sdslen(oids));
   
		    rewriteClientCommandArgument(c,1,argv[0]);
		    rewriteClientCommandArgument(c,2,argv[1]);
	        rewriteClientCommandArgument(c,4,argv[2]); 

	        //serverLog(LL_LOG,"argv: %s %s",(char*)argv[0]->ptr,(char*)argv[1]->ptr);
	        //serverLog(LL_LOG,"after rewrite u->ledge: %d %s %s %s",u->ledge->optype,u->ledge->argv1,u->ledge->argv2,u->ledge->oid);
	        //serverLogCmd(c);       
		} else {
	        //split x oid ctx tag1	
	        robj *argv[2];   	
	    	argv[0] = createStringObject(u->ledge->argv1,strlen(u->ledge->argv1));
		    argv[1] = createStringObject(oids,sdslen(oids));
		
			rewriteClientCommandArgument(c,1,argv[0]);	
		    rewriteClientCommandArgument(c,3,argv[1]);  
		    
		    //serverLog(LL_LOG,"argv: %s",(char*)argv[0]->ptr);
		    //serverLog(LL_LOG,"after rewrite u->ledge: %d %s %s",u->ledge->optype,u->ledge->argv1,u->ledge->oid);
	        //serverLogCmd(c);      
		}
		sdsfree(oids);		
	}	
}

void unionCommand(client *c) {
    controlAlg(c);
}

void splitCommand(client *c) {
    controlAlg(c);
}


/* example: uinit key 1 1,2/3/4/5 */
void uinitCommand(client *c) {
    int eqcs_num,num;
    sds *eqcs, *elements;
    int i,k,m;
    int vertexnum = 0;
    int j = 0;
    
	sds key = c->argv[1]->ptr;
	
    eqcs = sdssplitlen(c->argv[2]->ptr,sdslen(c->argv[2]->ptr),"/",1,&eqcs_num);
    for (i = 0; i < eqcs_num; i++) {        
        elements = sdssplitlen(eqcs[i],sdslen(eqcs[i]),",",1,&num);
        vertexnum += num;
        sdsfreesplitres(elements,num);
    }
	
	cudGraph *ufs = zmalloc(sizeof(cudGraph)+sizeof(vertex)*(vertexnum)+sizeof(char*));
    ufs->vertexnum = vertexnum;
    ufs->edgenum = 0;
    //ufs->key = sdsempty();
    //ufs->key = sdscat(ufs->key,c->argv[1]->ptr);
    ufs->key = sdsnew(c->argv[1]->ptr);
        
    for (i = 0; i < eqcs_num; i++) {        
        elements = sdssplitlen(eqcs[i],sdslen(eqcs[i]),",",1,&num);
        for (k = 0; k < num; k++) {
            //ufs->dlist[j].data = sdsempty();
            //ufs->dlist[j].data = sdscat(ufs->dlist[j].data,elements[k]);
            ufs->dlist[j].data = sdsnew(elements[k]);
            ufs->dlist[j].firstedge = NULL;
            j++;
        }
        for (k = 0; k < num; k++) {
            for (m = 0; m < num; m++)
                if (m != k) {
                    createLinks(ufs,locateVex(ufs,elements[k]),locateVex(ufs,elements[m]));
                }            
        }
		sdsfreesplitres(elements,num);        		
    }
    sdsfreesplitres(eqcs,eqcs_num);  

	/*add the new ufs to tail of server.ufslist*/
	if (!server.ufslist) server.ufslist = listCreate();
	listAddNodeTail(server.ufslist,ufs);
	
	/* create 2Dstatespace */
	if (server.masterhost) {
		if (!cspacelist) {
			cspacelist = zmalloc(sizeof(cdss));
			cspacelist->spaces = listCreate();
		}
		if (existCVerlist(key)) {
			c->flag_ufs = 0;
			addReply(c,shared.argvuaddexistufs);
			return;
		} else {
	    	cverlist *v = createCVerlist(key);
			vertice *top = createVertice();
			updateVerticeUfs(ufs,top);
			listAddNodeTail(v->vertices,top);
			listAddNodeTail(cspacelist->spaces,v); 
		}
       
	} else {
		if (sspacelist == NULL) {
			sspacelist = zmalloc(sizeof(sdss));
			sspacelist->spaces = listCreate();
		}
		
		//create a new 2Dstatespace for each client
		if (server.slaves) {
		    listNode *ln;
		    listIter li;
			listRewind(server.slaves,&li);
	        while((ln = listNext(&li))) {
    	    	client *c = ln->value;
    	    	
				verlist *v = createVerlist(c->slave_listening_port,key);
				vertice *top = createVertice();
				updateVerticeUfs(ufs,top);
				listAddNodeTail(v->vertices,top);
				listAddNodeTail(sspacelist->spaces,v);
				serverLog(LL_LOG,"create 2D state space for port: %d, key: %s", v->id, v->key);
			}
	    }	
	}
    
	c->flag_ufs = 1;
	addReply(c,shared.ok);
}

void findCommand(client *c) {
	robj *o;
	cudGraph *ufs = getUfs(c->argv[2]->ptr);
    int i = locateVex(ufs, c->argv[1]->ptr);
    serverLog(LL_LOG,"findCommand: argv: %s locate: %d ", (char *)c->argv[1]->ptr, i);

	sds list = sdsempty();
	list = sdscat(list, c->argv[1]->ptr);
	list = sdscat(list,",");
	if (ufs->dlist[i].firstedge != NULL) {
		edge *p = ufs->dlist[i].firstedge;
		while(p != NULL) {
			list = sdscat(list, ufs->dlist[p->adjvex].data);
			if (p->nextedge) list = sdscat(list,","); 
			p = p->nextedge;
		}
	}
	
	serverLog(LL_LOG, "findCommand: list %s ", list);

	//sort the list by increasing adjvex of vertexex
	int len;
	sds *elements = sdssplitlen(list,sdslen(list),",",1,&len);
	for (int i=0;i<len;i++) serverLog(LL_LOG,"splitcommand: elements %s len %d ", elements[i], len);
	//bubbleSort(ufs,elements,len);
	//insertSort(ufs,elements,len);
	quickSort(ufs,elements,0,len-1);
	sdsclear(list);
	for (int i = 0; i < len; i++) {
		list = sdscat(list,elements[i]);
	}
	sdsfreesplitres(elements,len);
	
	o = createObject(OBJ_STRING,list);
    addReplyBulk(c,o);
}

void umembersCommand(client *c) {
	robj *o;
	int i,j;
	int skip = 0;
	int flag = 0;
	sds list;
	sds result = sdsempty();
	cudGraph *ufs = getUfs(c->argv[1]->ptr);
	
	for (i = 0; i < ufs->vertexnum; i++) {
		for (j = 0; j < i; j++) {
			if (existEdge(ufs,i, j)) {
				skip++;	
				if (i == ufs->vertexnum-1) flag = 1;
				break;
			}
		}
		if (j < i) continue;
		//list = sdsempty();
		//list = sdscat(list, ufs->dlist[i].data);
		list = sdsnew(ufs->dlist[i].data);
		if (ufs->dlist[i].firstedge != NULL) list = sdscat(list,",");

		if (ufs->dlist[i].firstedge != NULL) {
			edge *p = ufs->dlist[i].firstedge;
			while(p != NULL) {
				list = sdscat(list, ufs->dlist[p->adjvex].data);
				if (p->nextedge) list = sdscat(list,","); 
				p = p->nextedge;
			}
		}
		//serverLog(LL_LOG, "findCommand: list %s ", list);

		//sort the list by increasing adjvex of vertexex
		int len;
		sds *elements = sdssplitlen(list,sdslen(list),",",1,&len);

		//for (j=0;j<len;j++) serverLog(LL_LOG,"splitcommand: elements %s len %d ", elements[j], len);
		//bubbleSort(ufs,elements,len);
		//insertSort(ufs,elements,len);
		quickSort(ufs,elements,0,len-1);
		sdsclear(list);

		for (j = 0; j < len; j++) {
			list = sdscat(list,elements[j]);
			if (j != len-1) list = sdscat(list,",");
		}
		result = sdscat(result,list);
		if (i != (ufs->vertexnum - 1)) {
			result = sdscat(result,"/"); 
		}
		sdsfreesplitres(elements,len);
		sdsfree(list);
	}
	if ((skip == ufs->vertexnum - 1) || flag == 1) sdsrange(result,0,sdslen(result)-2);
	o = createObject(OBJ_STRING,result);
    addReplyBulk(c,o);		
}

void test (sds t) {
    sds p = t;
    int i = 1;
    while (i<4) {
    p = sdscat(p,"mm");
    i++;}
}

void test2  (sds t) {
    int i = 1;
    sds q = t;
    while (i<4) {
   q = sdscat(q,"nn");
    i++;} 
}

void test3 (sds t) {
    int i = 1;
    sds q= sdsnewlen(t,20);
    sds m = sdsempty();
    m = sdscat(m,"nn");
    while(i<4){
    q = sdscat(q,"nn");
    i++;}
    i=1;
    while(i<3){
    q = sdscat(q,m);
    i++;}
}

void testCommand(client *c) {
    addReply(c,shared.ok);
}


