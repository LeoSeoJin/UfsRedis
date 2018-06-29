#include "server.h"
#include <sys/time.h>
#include <stdlib.h>

int randSleepTime() {
	srand(time(NULL));
	return rand()%4+2;
}

sds *transfRobjToSdsptr(robj **argv, int len) {
	sds *result = zmalloc(sizeof(sds)*len);
	if (argv[0] == shared.unionot) {
		result[0] = shared.sdsunionot;
	} else {
		result[0] = shared.sdssplitot;
	}
	for (int i = 1; i < len; i++) {
		result[i] = sdsnew(argv[i]->ptr);
    }
	return result;
}

sds transfRobjToSds(robj **argv, int len) {
	sds result;
	if (argv[0] == shared.unionot) {
		result = sdsnew("unionot");
	} else {
		result = sdsnew("splitot");
	}
	
	for (int i = 1; i < len; i++) {
		result = sdscat(result," ");
		result = sdscat(result,argv[i]->ptr);
    }
    return result;
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

cudGraph *getUfs(sds key) {
	listNode *ln;
    listIter li;
	
	listRewind(server.ufslist,&li);
    while((ln = listNext(&li))) {
        cudGraph *ufs = ln->value;
        if (!sdscmp(ufs->key,key)) return ufs;
    }
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
    for(int i = 0; i < ufs->vertexnum; i++)  
        if(!sdscmp(e, ufs->dlist[i].data))  return 1;  
    return 0; 	
}

int existElements(cudGraph *ufs, sds *list, int len) {
    int i,j;
	for (i = 0; i < len; i++) {
       for(j = 0; j < ufs->vertexnum; j++)  
          if(!sdscmp(list[i], ufs->dlist[j].data)) break;
       if (j != ufs->vertexnum) break;
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
	serverLog(LL_LOG,"vertex index: %d %d ", i, j);

    createEdge(ufs,i, j);
	serverLog(LL_LOG, "createEdge success ");

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
	serverLog(LL_LOG,"removeAdjEdge: vertex: %d ", i);
	if (p != NULL) {
		edge *q;
		while (p != NULL) {
			if (!list || !find(list, ufs->dlist[p->adjvex].data, len)) {
				serverLog(LL_LOG, "removeAdjEdge: v1 %s v2: %s ",v,ufs->dlist[p->adjvex].data);
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
		
		serverLog(LL_LOG,"removeAdjEdge: edgenum %d ", ufs->edgenum);
		
		//if (!list) ufs->dlist[i].firstedge = NULL;

		for (int j = 0; j < ufs->vertexnum; j++) {
			p = ufs->dlist[j].firstedge;
			while (p != NULL) {
				if (p->adjvex == i) {
					if (!list || !find(list, ufs->dlist[j].data, len)) {
						serverLog(LL_LOG, "removeAdjEdge: v1 %s v2: %s ",ufs->dlist[j].data,v);
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

void updateVerticeUfsFromState(sds ufs, robj **argv, vertice *v) {
    //flagtodo: execute cmd and construct a new sds to store the ufs and store it to v.ufs 
    int len, i;
    sds *elements = sdssplitlen(ufs, sdslen(ufs), "/", 1, &len);
    sds v_ufs;
    
    if (argv[0] == shared.unionot || (argv[0]!=shared.splitot && !strcmp(argv[0]->ptr,"unionot"))) {
	   	serverLog(LL_LOG,"updaeVerticeUfsFromState: unionot");
		if (!sdscmp(argv[1]->ptr,"*") || !strcmp(argv[2]->ptr,"*")) {
			v_ufs = sdsnew(ufs);
		} else {
        //union operation
        int argv1class = -1; 
        int argv2class = -1;
        for (i = 0; i < len; i++) {
            if (argv1class == -1 && findSds(elements[i],argv[1]->ptr)) argv1class = i;
            if (argv2class == -1 && findSds(elements[i],argv[2]->ptr)) argv2class = i;
            if (argv1class != -1 && argv2class != -1) break;
        }
		
		serverLog(LL_LOG,"argv1class: %d argv2class: %d ",argv1class, argv2class);
        
        if (argv1class != argv2class) {
        	v_ufs = sdsempty();
            int low = (argv1class < argv2class)? argv1class: argv2class;
            int high = (argv1class > argv2class)? argv1class: argv2class;
            for (i = 0; i < len; i++) {
                if (i != low || i == high) continue;
                elements[i] = sdscat(elements[i],",");
                elements[i] = sdscat(elements[i],elements[high]);
                break;
            }
            for (i = 0; i < len; i++) {
            	if (i == high) continue;
				v_ufs = sdscat(v_ufs,elements[i]);
                if (high != len-1) {
                    if (i != len-1) v_ufs = sdscat(v_ufs,"/");
                } else {
                    if (i != len-2) v_ufs = sdscat(v_ufs,"/");
                }
            }
       } else {
            v_ufs = sdsnew(ufs);
       }
       }       
    } else {
        //split operaton
    	serverLog(LL_LOG,"updaeVerticeUfsFromState: splitot");
    	if (!strcmp(argv[1]->ptr,"*")) {
    		v_ufs = sdsnew(ufs);
    	} else { 
        int argvclass = -1;
        sds class2 = sdsnew(argv[1]->ptr);
		//v_ufs = sdsempty();
		
        for (i = 0; i < len; i++) {
            if (findSds(elements[i],argv[1]->ptr) && sdscntcmp(elements[i],argv[1]->ptr)) {
                argvclass = i;
                elements[i] = sdsDel(elements[i],class2);  
                serverLog(LL_LOG,"updateVerticeFromState: splitot sdsDel elements[i]: %s ",elements[i]);      
                break;
            } 
        }
        if (argvclass != -1) {
        	v_ufs = sdsempty();
            for (i = 0; i < len; i++) {
                if (i != argvclass) {
                    v_ufs = sdscat(v_ufs,elements[i]);
                    if (i != len-1) v_ufs = sdscat(v_ufs,"/");
                } else {
                    v_ufs = sdscat(v_ufs,elements[i]);
                    v_ufs = sdscat(v_ufs,"/");
                    v_ufs = sdscat(v_ufs,class2);
                    if (argvclass != len-1) v_ufs = sdscat(v_ufs,"/");
                }  
            }
        } else {
           v_ufs = sdsnew(ufs);
        }
        }
    }
    serverLog(LL_LOG,"updateVerticeFromState: v_ufs %s ",v_ufs);
    sdsfreesplitres(elements,len);
    v->content = v_ufs;
}

void updateVerticeUfs(cudGraph *ufs, vertice *v) { 
	serverLog(LL_LOG,"updateVerticeUfs start");
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
		list = sdsempty();
		list = sdscat(list, ufs->dlist[i].data);
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
	v->content = result;
	serverLog(LL_LOG,"updateVerticeUfs finish: %s",v->content);
}

void unionProcess(client *c, robj **argv) {
	int i,j;
	int argv1len = strlen(argv[1]->ptr);
	int argv2len = strlen(argv[2]->ptr);
	cudGraph *ufs = getUfs(c->argv[c->argc-1]->ptr);
	list *space = getSpace(c->argv[c->argc-1]->ptr);
	serverLog(LL_LOG,"union arguments num: %d %d ", argv1len, argv2len);
	
	/*case1: argv1 is a set and argv2 is a singleton element
	 *case2: argv2 is a set and argv1 is a singleton element
	 *case3: argv1 and argv2 are both sets
	 *case4: argv1 and argv2 are both singleton elements
	 *special case: argv1 or argv2 is NULL (generated by ot)*/
	if (!strcmp(argv[1]->ptr,"*") || !strcmp(argv[2]->ptr,"*")) {
		//server.dirty++;
		c->flag_ufs = 1;
		addReply(c,shared.ok); //this case only happens when receiving o from server or finishing OT process locally
	} else {
    	if (argv1len > 1 && argv2len == 1) {
    		sds *elements;
    		int len;
    		
    		elements = sdssplitlen(argv[1]->ptr,strlen(argv[1]->ptr),",",1,&len);
    		/*check whether elements of argv1 are in one equivalent class*/
    		if (!isConnected(ufs,elements,len)) {
    			c->flag_ufs = 0;
    			removeLastAddEdge(space);
    			addReply(c,shared.argv1nosameclass);
    		} else {
    			if (!existEdge(ufs,locateVex(ufs,elements[0]),locateVex(ufs,argv[2]->ptr))) {
    				for (i = 0; i < len; i++) 
    					createLinks(ufs,locateVex(ufs, elements[i]), locateVex(ufs, argv[2]->ptr));
    			}
    		    //server.dirty++;
    		    c->flag_ufs = 1;
    			addReply(c,shared.ok);
    		}
    		sdsfreesplitres(elements,len);
    	} else if (argv2len > 1 && argv1len == 1){
    		sds *elements;
    		int len;
    		
    		elements = sdssplitlen(argv[2]->ptr,strlen(argv[2]->ptr),",",1,&len);
    		/*check whether elements of argv2 are in one equivalent class*/
    		if (!isConnected(ufs,elements,len)) {
    			c->flag_ufs = 0;
    			removeLastAddEdge(space);	
    			addReply(c,shared.argv2nosameclass);
    		} else {
    			if (!existEdge(ufs,locateVex(ufs,elements[0]),locateVex(ufs,argv[1]->ptr))) {
    				for (i = 0; i < len; i++) 
    					createLinks(ufs,locateVex(ufs, elements[i]), locateVex(ufs, argv[1]->ptr));
    			}					
    			//server.dirty++;
    			c->flag_ufs = 1;
    			addReply(c,shared.ok);
    		}
    		sdsfreesplitres(elements,len);
    	} else if (argv1len > 1 && argv2len > 1) {
    		sds *elements1, *elements2;
    		int len1, len2;
    
    		elements1 = sdssplitlen(argv[1]->ptr,strlen(argv[1]->ptr),",",1,&len1);
    		elements2 = sdssplitlen(argv[2]->ptr,strlen(argv[2]->ptr),",",1,&len2);
       		
    		if (!isConnected(ufs,elements1,len1) && !isConnected(ufs,elements2,len2)) {
    			c->flag_ufs = 0;
    			removeLastAddEdge(space);
    			addReply(c,shared.argv12nosameclass);		
    		} else if (!isConnected(ufs,elements1,len1)) {
    			c->flag_ufs = 0;
    			removeLastAddEdge(space);
    			addReply(c,shared.argv1nosameclass);
    		} else if (!isConnected(ufs,elements2,len2)) {
    			c->flag_ufs = 0;
    			removeLastAddEdge(space);
    			addReply(c,shared.argv2nosameclass);
    		} else {
    			if (!existEdge(ufs,locateVex(ufs,elements1[0]),locateVex(ufs,elements2[0]))) {
    				for (i = 0; i < len1; i++) {
    					for (j = 0; j < len2; j++) {
    						createLinks(ufs,locateVex(ufs,elements1[i]),locateVex(ufs,elements2[j]));
    					}
    				}
    			}	
    			//server.dirty++;
    			c->flag_ufs = 1;
    			addReply(c,shared.ok);
    		}
    		sdsfreesplitres(elements1,len1);
    		sdsfreesplitres(elements2,len2);
    	} else {    		
    		serverLog(LL_LOG,"union x x");   		
    	    createLinks(ufs,locateVex(ufs, argv[1]->ptr),locateVex(ufs, argv[2]->ptr));
    		c->flag_ufs = 1;
    		addReply(c,shared.ok);
    	}
   } 
}

void unionProc(client *c, robj **argv) {
	int i,j;
	int argv1len = strlen(argv[1]->ptr);
	int argv2len = strlen(argv[2]->ptr);
	cudGraph *ufs = getUfs(c->argv[c->argc-1]->ptr);
	serverLog(LL_LOG,"unionProc arguments num: %d %d ", argv1len, argv2len);
	
	/*case1: argv1 is a set and argv2 is a singleton element
	 *case2: argv2 is a set and argv1 is a singleton element
	 *case3: argv1 and argv2 are both sets
	 *case4: argv1 and argv2 are both singleton elements
	 *special case: argv1 or argv2 is NULL (generated by ot)*/
	if (!strcmp(argv[1]->ptr,"*") || !strcmp(argv[2]->ptr,"*")) {
		c->flag_ufs = 1;
		addReply(c,shared.ok); //this case only happens when receiving o from server or finishing OT process locally
	} else {
    	if (argv1len > 1 && argv2len == 1) {
    		sds *elements;
    		int len;
    		
    		elements = sdssplitlen(argv[1]->ptr,strlen(argv[1]->ptr),",",1,&len);
    		if (!existEdge(ufs,locateVex(ufs,elements[0]),locateVex(ufs,argv[2]->ptr))) {
   				for (i = 0; i < len; i++) 
   					createLinks(ufs,locateVex(ufs, elements[i]), locateVex(ufs, argv[2]->ptr));
   			}
   		    c->flag_ufs = 1;
   			addReply(c,shared.ok);
    		sdsfreesplitres(elements,len);
    	} else if (argv2len > 1 && argv1len == 1){
    		sds *elements;
    		int len;
    		
    		elements = sdssplitlen(argv[2]->ptr,strlen(argv[2]->ptr),",",1,&len);
   			if (!existEdge(ufs,locateVex(ufs,elements[0]),locateVex(ufs,argv[1]->ptr))) {
   				for (i = 0; i < len; i++) 
   					createLinks(ufs,locateVex(ufs, elements[i]), locateVex(ufs, argv[1]->ptr));
   			}					
   			c->flag_ufs = 1;
   			addReply(c,shared.ok);
    		sdsfreesplitres(elements,len);
    	} else if (argv1len > 1 && argv2len > 1) {
    		sds *elements1, *elements2;
    		int len1, len2;
    
    		elements1 = sdssplitlen(argv[1]->ptr,strlen(argv[1]->ptr),",",1,&len1);
    		elements2 = sdssplitlen(argv[2]->ptr,strlen(argv[2]->ptr),",",1,&len2);
       		
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
    		serverLog(LL_LOG,"union x x");   		
    	    createLinks(ufs,locateVex(ufs, argv[1]->ptr),locateVex(ufs, argv[2]->ptr));
    		c->flag_ufs = 1;
    		addReply(c,shared.ok);
    	}
   } 
}

void splitProcess(client *c, robj **argv) {
	cudGraph *ufs = getUfs(c->argv[c->argc-1]->ptr);
	list *space = getSpace(c->argv[c->argc-1]->ptr);
	if (strlen(argv[1]->ptr) > 1) {
		sds *elements;
		int len;
	
		elements = sdssplitlen(argv[1]->ptr,strlen(argv[1]->ptr),",",1,&len);
		
		//for (int i=0;i<len;i++) serverLog(LL_LOG,"splitcommand: elements %s ", elements[i]);
		
		if (!isConnected(ufs,elements,len)) {
			c->flag_ufs = 0;
			removeLastAddEdge(space);
			addReply(c,shared.argvsplitnosameclass);
		} else {
			for (int i = 0; i < len; i++) 
				removeAdjEdge(ufs,elements[i], elements, len);
			
			for (int i=0; i<len; i++) {
				edge *p = ufs->dlist[locateVex(ufs,elements[i])].firstedge;
				serverLog(LL_LOG,"finish split");
				while (p != NULL) {
					serverLog(LL_LOG,"%s->edges %s ",elements[i],ufs->dlist[p->adjvex].data);
					p = p->nextedge;
				}
			}
			//server.dirty++;
			c->flag_ufs = 1;
			addReply(c,shared.ok);
		}
		sdsfreesplitres(elements,len);
	} else {
	    //argument may empty, just do nothing locally for this case
		if (strcmp(argv[1]->ptr,"*")) removeAdjEdge(ufs,argv[1]->ptr, NULL, 0); 
		//server.dirty++;
		c->flag_ufs = 1;
		addReply(c,shared.ok);
	}	
}

void splitProc(client *c, robj **argv) {
	cudGraph *ufs = getUfs(c->argv[c->argc-1]->ptr);
	if (strlen(argv[1]->ptr) > 1) {
		sds *elements;
		int len;
	
		elements = sdssplitlen(argv[1]->ptr,strlen(argv[1]->ptr),",",1,&len);

		for (int i = 0; i < len; i++) 
			removeAdjEdge(ufs,elements[i], elements, len);
		
		c->flag_ufs = 1;
		addReply(c,shared.ok);
		sdsfreesplitres(elements,len);
	} else {
	    //argument may empty, just do nothing locally for this case
		if (strcmp(argv[1]->ptr,"*")) removeAdjEdge(ufs,argv[1]->ptr, NULL, 0); 
		c->flag_ufs = 1;
		addReply(c,shared.ok);
	}	
}

int checkArgv(client *c, robj **argv) {
    if (!strcmp(c->argv[0]->ptr,"unionot")) {
        return checkUnionArgv(c, argv);
    } else {
        return checkSplitArgv(c, argv);
    }
}

int checkUnionArgv(client *c, robj **argv) {
	int i,j;
	int argv1len = strlen(argv[1]->ptr);
	int argv2len = strlen(argv[2]->ptr);
	cudGraph *ufs = getUfs(c->argv[c->argc-1]->ptr);
	serverLog(LL_LOG,"check union argv");
	
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
	if (strlen(argv[1]->ptr) > 1) {
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
			sds oids,oid;
			vertice *v;
           
            v = locateVertice(space, NULL);
			oid = sdsempty();
			char buf1[Port_Num+1] = {0};
			char buf2[Max_Op_Num] = {0};
			oid = sdscat(oid,itos(server.port,buf1,10));
			oid = sdscat(oid,"_");
			oid = sdscat(oid,itos(op_num,buf2,10));
			oids = sdsnew(v->oids);
			oids = sdscat(oids,",");
			oids = sdscat(oids,oid);				
			vertice *v1 = createVertice(oids);
			listAddNodeTail(space,v1);
			
			robj **argv = zmalloc(sizeof(robj *)*c->argc);
			if (!strcmp(c->argv[0]->ptr,"unionot")) {
				argv[0] = shared.unionot; 
			} else {
				argv[0] = shared.splitot;
			}
		    for (int i = 1; i < c->argc; i++) 
		        argv[i] = createObject(OBJ_STRING, sdsnew(c->argv[i]->ptr));
					
			//v->ledge = createOpEdge(argv,oids,oid,v1);
			v->ledge = createOpEdge(argv,oid,v1);

			/*propagate to master: construct and rewrite cmd of c
			 *(propagatetomaster will call replicationfeedmaster(c->argv,c->argc))
			 *union x y oid ctx or split x oid ctx(just for propagation)
			 */
			if (c->argc == 4) {
			    //union x y oid ctx tag1
			    robj *argv6 = createObject(OBJ_STRING,sdsnew(c->argv[3]->ptr));
			    robj *argv4 = createObject(OBJ_STRING,v->ledge->oid);
			    robj *argv5 = createObject(OBJ_STRING,v->oids);
                rewriteClientCommandArgument(c,3,argv4);
                rewriteClientCommandArgument(c,4,argv5); 
                rewriteClientCommandArgument(c,5,argv6);
                //serverLog(LL_LOG,"rewriteClientCommandArgument, unionot, %s %s ",oid, oids);
                serverLogCmd(c);       
			} else {
                //split x oid ctx tag1	
                robj *argv5 = createObject(OBJ_STRING,sdsnew(c->argv[2]->ptr));
                robj *argv3 = createObject(OBJ_STRING,v->ledge->oid);
			    robj *argv4 = createObject(OBJ_STRING,v->oids);		
    			rewriteClientCommandArgument(c,2,argv3);
	    		rewriteClientCommandArgument(c,3,argv4);
	    		rewriteClientCommandArgument(c,4,argv5);  
	    		//serverLog(LL_LOG,"rewriteClientCommandArgument, splitot, %s %s ",oid, oids);
                serverLogCmd(c);      
			}
			sdsfree(oid);
			sdsfree(oids);
			
			serverLog(LL_LOG,"rewriteClientCommandArgument, argc %d ",c->argc);
            serverLogCmd(c); 
			
			//execution of local generated operation
			if (!strcmp(c->argv[0]->ptr, "unionot")) unionProc(c, c->argv); 
            if (!strcmp(c->argv[0]->ptr, "splitot")) splitProc(c, c->argv);
			
            //update ufs of vertice
            if (v->ledge) {
            	updateVerticeUfs(ufs,v1);
				serverLog(LL_LOG,"local processing: v1->oids: %s v1->content: %s ",v1->oids, v1->content);
			}
			
			//guarantee local operation firstly executed, sleep several seconds, then receive operation
            if (!strcmp(c->argv[0]->ptr, "unionot")) sleep(randSleepTime()); 
            if (!strcmp(c->argv[0]->ptr, "splitot")) sleep(randSleepTime());
		} else {
			/*remote processing: op argv1 (argv2) oid oids tag1*/
			sds ctx, oid;
			if (c->argc == 6) {            
			    ctx = c->argv[4]->ptr;
		    	oid = c->argv[3]->ptr;
			} else {			
    			ctx = c->argv[3]->ptr;
	    		oid = c->argv[2]->ptr;
			}
			serverLog(LL_LOG,"start remote processing: ctx: %s oid: %s",ctx, oid);
			
			vertice *u = locateVertice(space,ctx);
			serverLog(LL_LOG,"locate u: oids: %s content: %s",u->oids, u->content);
			sds oids = sdsempty();
			oids = sdscat(oids,ctx);
			oids = sdscat(oids,",");
			oids = sdscat(oids,oid);
			vertice *v = createVertice(oids);			
			listAddNodeTail(space,v);
			sdsfree(oids);

			robj **argv = zmalloc(sizeof(robj *)*(c->argc-3)); //flag todo 
			if (!strcmp(c->argv[0]->ptr,"unionot")) {
				argv[0] = shared.unionot; 
			} else {
				argv[0] = shared.splitot;
			}
		    for (int i = 1; i < c->argc-3; i++) 
		        argv[i] = createObject(OBJ_STRING, sdsnew(c->argv[i]->ptr));

			//u->redge = createOpEdge(argv,ctx,oid,v);
			u->redge = createOpEdge(argv,oid,v);
			updateVerticeUfsFromState(u->content,c->argv,v); 

			vertice *ul;
			vertice *p = v;
			oids = sdsnew(p->oids);
			

			while (u->ledge != NULL) {
				oids = sdscat(oids,",");
				oids = sdscat(oids,u->ledge->oid);
				vertice *pl = createVertice(oids);	
				listAddNodeTail(space,pl);
				//p->ledge = createOpEdge(NULL,p->oids,u->ledge->oid,pl);
				p->ledge = createOpEdge(NULL,u->ledge->oid,pl);
				
				ul = u->ledge->adjv;
				//ul->redge = createOpEdge(NULL,ul->oids,u->redge->oid,pl);
				ul->redge = createOpEdge(NULL,u->redge->oid,pl);

				serverLog(LL_LOG,"ot starting");
				//ufs->proc(u->content,u->ledge->argv,u->redge->argv,p->ledge, ul->redge, UNION);
                //ufs->proc(u->content,u->ledge->argv,u->redge->argv,p->ledge, ul->redge, SPLIT);
				//ufs->proc(u->content,u->ledge->argv,u->redge->argv,p->ledge, ul->redge, SPLIT);
				c->cmd->otproc(u->content,u->ledge->argv,u->redge->argv,p->ledge, ul->redge, SPLIT);
                serverLog(LL_LOG,"ot finished");
                updateVerticeUfsFromState(ul->content,ul->redge->argv,pl); 
                
				u = ul;
				p = pl;
			}
			sdsfree(oids);
			
			/*execute the trasformed remote operation (finish ot process)
			 *or original remote operation (u->ledge = NULL, no ot process)
			 */
            if (!strcmp(c->argv[0]->ptr, "unionot")) {
            	unionProc(c,u->redge->argv); 
            	sleep(randSleepTime());
            }	
            if (!strcmp(c->argv[0]->ptr, "splitot")) {
                splitProc(c,u->redge->argv);
                sleep(randSleepTime());
            }
		}
	} else {
		/*server processing, unionot x y oid ctx tag1*/
		sds ctx, oid;

        if (c->argc == 6) {            
            ctx = c->argv[4]->ptr;
            oid = c->argv[3]->ptr;
        } else {            
            ctx = c->argv[3]->ptr;
            oid = c->argv[2]->ptr;
        }
		serverLog(LL_LOG,"controlAlg: server: argc %d ctx %s oid %s ", c->argc, ctx, oid);
		
		verlist *s = locateVerlist(c->slave_listening_port,c->argv[c->argc-1]->ptr);
		vertice *u = locateVertice(s->vertices,ctx);
		
		sds oids = sdsempty();
		oids = sdscat(oids,ctx);
		oids = sdscat(oids,",");
		oids = sdscat(oids,oid);
		vertice *v = createVertice(oids);	
		listAddNodeTail(s->vertices,v);
		sdsfree(oids);

		robj **argv = zmalloc(sizeof(robj *)*(c->argc-2));
		if (!strcmp(c->argv[0]->ptr,"unionot")) {
			argv[0] = shared.unionot; 
		} else {
			argv[0] = shared.splitot;
		}		
		for (int i = 1; i < c->argc-2; i++) 
		    argv[i] = createObject(OBJ_STRING,sdsnew(c->argv[i]->ptr));
		u->ledge = createOpEdge(argv,oid,v);

		serverLog(LL_LOG,"create a new local operation finished, current state %s ", u->content); 		
        updateVerticeUfsFromState(u->content,c->argv,v);
        serverLog(LL_LOG,"updateVerticeUfsFromState finished, v->content %s ", v->content); 
        
		vertice *ur;
		vertice *p = v;
		oids = sdsnew(p->oids);
		while (u->redge != NULL) {
			//serverLog(LL_LOG,"ot finished, space: %d p->oids: %s, u->redge->oid: %s",space->id,oids,u->redge->oid);
			
			oids = sdscat(oids,",");
			oids = sdscat(oids,u->redge->oid);
			vertice *pr = createVertice(oids);	
			listAddNodeTail(s->vertices,pr);
			//p->redge = createOpEdge(NULL,p->oids,u->redge->oid,pr);
			p->redge = createOpEdge(NULL,u->redge->oid,pr);
			
			ur = u->redge->adjv;
			//ur->ledge = createOpEdge(NULL,ur->oids,u->ledge->oid,pr);
			ur->ledge = createOpEdge(NULL,u->ledge->oid,pr);
			
			//ufs->proc(u->content,u->redge->argv,u->ledge->argv,p->redge,ur->ledge,UNION);	
			//ufs->proc(u->content,u->redge->argv,u->ledge->argv,p->redge,ur->ledge,SPLIT);
			//ufs->proc(u->content,u->redge->argv,u->ledge->argv,p->redge,ur->ledge,SPLIT);
			c->cmd->otproc(u->content,u->redge->argv,u->ledge->argv,p->redge,ur->ledge,SPLIT);
			
			serverLog(LL_LOG,"p->redge: oids: %s oid: %s",p->oids,u->redge->oid);
			serverLogArgv(p->redge->argv);			

			serverLog(LL_LOG,"ur->ledge: oids: %s oid: %s",ur->oids,u->ledge->oid);
			serverLogArgv(ur->ledge->argv);
			
			serverLog(LL_LOG,"update pr->content, from state: %s",ur->content);
			updateVerticeUfsFromState(ur->content,ur->ledge->argv,pr); 
			
			serverLog(LL_LOG,"pr->content: %s ", pr->content);
			u = ur;
			p = pr;
	    }
	    sdsfree(oids);	
	    
	    //save argv2 to each other client's space  ,******************* todo: save to the key's space
	    listNode *ln;
        listIter li;
		int i, argv_len;
		
	    listRewind(sspacelist->spaces,&li);	    
        while((ln = listNext(&li))) {
            verlist *s = ln->value;
            if (sdscmp(s->key,c->argv[c->argc-1]->ptr) || s->id == c->slave_listening_port) continue;
			
			//serverLog(LL_LOG,"server processing: save to other space: port %d ",space->id);
            
            oids = sdsempty();
            vertice *loc = locateVertice(s->vertices,NULL);
            serverLog(LL_LOG,"locateVertice: loc->oids %s ",loc->oids);
            oids = sdscat(oids,loc->oids);
            oids = sdscat(oids,",");
            oids = sdscat(oids,u->ledge->oid);
            serverLog(LL_LOG,"locateVertice: u->ledge->oid %s ",u->ledge->oid);
            serverLog(LL_LOG,"oids: %s ",oids);
            vertice *locr = createVertice(oids);
            listAddNodeTail(s->vertices,locr);
			sdsfree(oids);
			
			serverLog(LL_LOG,"server processing: update other dss, port %d locr->oids: %s", s->id, locr->oids);
		
		
			//serverLog(LL_LOG,"save argv to other clients, memory (before): %d ", zmalloc_used_memory());
            //argv_len = (!strcmp(u->ledge->argv[0]->ptr,"unionot"))?3: 2;
            argv_len = (u->ledge->argv[0]==shared.unionot)?3: 2;
            robj **locr_argv = zmalloc(sizeof(robj *)*argv_len);
            if (!strcmp(c->argv[0]->ptr,"unionot")) {
				locr_argv[0] = shared.unionot; 
			} else {
				locr_argv[0] = shared.splitot;
			}
            for (i = 1; i < argv_len; i++)
            	locr_argv[i] = createObject(OBJ_STRING,u->ledge->argv[i]->ptr);
			loc->redge = createOpEdge(locr_argv,u->ledge->oid,locr); 

            updateVerticeUfsFromState(loc->content,loc->redge->argv,locr); 
            serverLogArgv(loc->redge->argv);
            serverLog(LL_LOG,"oids: %s oid: %s ufs: %s ", locr->oids, loc->redge->oid, locr->content);
        }
    	//server.dirty++; 
    	c->flag_ufs = 1;
    	//rewrite the command argv and transform it to other clients;
		if (c->argc == 6) {
	    	//union x y oid ctx tag1
	    	robj *argv2 = createObject(OBJ_STRING,u->ledge->argv[1]->ptr);
	    	robj *argv3 = createObject(OBJ_STRING,u->ledge->argv[2]->ptr);
		    robj *argv4 = createObject(OBJ_STRING,u->ledge->oid);
		    robj *argv5 = createObject(OBJ_STRING,u->oids);
		    rewriteClientCommandArgument(c,1,argv2);
		    rewriteClientCommandArgument(c,2,argv3);
	        rewriteClientCommandArgument(c,3,argv4);
	        rewriteClientCommandArgument(c,4,argv5); 
	        //serverLog(LL_LOG,"rewriteClientCommandArgument, unionot, %s %s ",oid, oids);
	        serverLogCmd(c);       
		} else {
	        //split x oid ctx tag1	
	        robj *argv2 = createObject(OBJ_STRING,u->ledge->argv[1]->ptr);
	        robj *argv3 = createObject(OBJ_STRING,u->ledge->oid);
			robj *argv4 = createObject(OBJ_STRING,u->oids);	
			rewriteClientCommandArgument(c,1,argv2);	
			rewriteClientCommandArgument(c,2,argv3);
		    rewriteClientCommandArgument(c,3,argv4);  
		    //serverLog(LL_LOG,"rewriteClientCommandArgument, splitot, %s %s ",oid, oids);
	        serverLogCmd(c);      
		}      
	}	
}

void unionotCommand(client *c) {
    controlAlg(c);
}

void splitotCommand(client *c) {
    controlAlg(c);
}

/*Union command is used to synchronize the initial states of clients
 *and synchrnize the state of the first vertice in the state space
 *suggest to use it at master server
 */
void unionCommand(client *c) {
	unionProcess(c, c->argv);
	cudGraph *ufs = getUfs(c->argv[3]->ptr);
	list *space = getSpace(c->argv[c->argc-1]->ptr);
	serverLog(LL_LOG,"unionprocess finished");
	//update the state of the first vertice in state spaces (clients' and server's)
	if (server.masterhost) {
        //just update its space
        vertice *v = space->head->value;
        
        updateVerticeUfs(ufs,v);
	} else {
        //update top vertice of each client's state space
        if (sspacelist != NULL) {
        	listNode *ln;
	        listIter li;
		
		    listRewind(sspacelist->spaces,&li);
	        while((ln = listNext(&li))) {
	            verlist *s = ln->value;
	            vertice *v = s->vertices->head->value;
	            updateVerticeUfs(ufs,v);
	        }
		}	
	}
}

void splitCommand(client *c) {
	splitProcess(c, c->argv);
	cudGraph *ufs = getUfs(c->argv[2]->ptr);
	list *space = getSpace(c->argv[c->argc-1]->ptr);
	/*update the state of the first vertice in state spaces (clients' and server's)*/
	if (server.masterhost) {
        //just update its space
        vertice *v = space->head->value;
        updateVerticeUfs(ufs,v);
	} else {
        //update top vertice of each client's state space
        if (sspacelist != NULL) {
        	listNode *ln;
        	listIter li;
	
		    listRewind(sspacelist->spaces,&li);
	        while((ln = listNext(&li))) {
    	        verlist *s = ln->value;
    	        vertice *v = s->vertices->head->value;
    	        updateVerticeUfs(ufs,v);
    	    }
		}
	}
}

/* example: uadd 1 1,2/3/4/5 */
void uaddCommand(client *c) {
    int eqcs_num,num;
    sds *eqcs, *elements;
    int i,k,m;
    int vertexnum = 0;
    int j = 0;

	char* key = (char*)c->argv[1]->ptr;
	
    eqcs = sdssplitlen(c->argv[2]->ptr,sdslen(c->argv[2]->ptr),"/",1,&eqcs_num);
    for (i = 0; i < eqcs_num; i++) {        
        elements = sdssplitlen(eqcs[i],sdslen(eqcs[i]),",",1,&num);
        vertexnum += num;
        sdsfreesplitres(elements,num);
    }
	
	cudGraph *ufs = zmalloc(sizeof(cudGraph)+sizeof(vertex)*(vertexnum));
    ufs->vertexnum = vertexnum;
    ufs->edgenum = 0;
    ufs->key = sdsnew(key);
        
    for (i = 0; i < eqcs_num; i++) {        
        elements = sdssplitlen(eqcs[i],sdslen(eqcs[i]),",",1,&num);
        for (k = 0; k < num; k++) {
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
	serverLog(LL_LOG,"update state of first node in DSS");
	if (server.masterhost) {
	    /**
	    if (!space) space = listCreate();
	    sds oids = sdsnew("init");
        vertice *v = createVertice(oids); //top vertice, state is empty
        updateVerticeUfs(ufs,v);
        listAddNodeTail(space,v);
		**/
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
			sds oids = sdsnew("init");
			vertice *top = createVertice(oids);
			sdsfree(oids);
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
    			sds oids = sdsnew("init");
				vertice *top = createVertice(oids);
				sdsfree(oids);
				//top->content = sdsempty();
				updateVerticeUfs(ufs,top);
				listAddNodeTail(v->vertices,top);
				listAddNodeTail(sspacelist->spaces,v);
				serverLog(LL_LOG,"create 2D state space for port: %d, key: %s", v->id, v->key);
			}
	    }	
	}
    
	//server.dirty++;
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
	
	serverLog(LL_LOG,"printufscommand: vertexnum: %d ",ufs->vertexnum);
	for (i = 0; i < ufs->vertexnum; i++) {
		serverLog(LL_LOG,"printufsCommand: i: %d ", i);
		for (j = 0; j < i; j++) {
			if (existEdge(ufs,i, j)) {
				skip++;	
				if (i == ufs->vertexnum-1) flag = 1;
				break;
			}
		}
		if (j < i) continue;
		list = sdsempty();
		list = sdscat(list, ufs->dlist[i].data);
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

		for (j=0;j<len;j++) serverLog(LL_LOG,"splitcommand: elements %s len %d ", elements[j], len);
		//bubbleSort(ufs,elements,len);
		//insertSort(ufs,elements,len);
		quickSort(ufs,elements,0,len-1);
		sdsclear(list);

		for (j = 0; j < len; j++) {
			list = sdscat(list,elements[j]);
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

void testCommand(client *c) {
	sds t = sdsnew("redis");
	serverLog(LL_LOG,"len: %d alloc: %d",SDS_HDR(8,t)->len,SDS_HDR(8,t)->alloc);
	t = sdscat(t,"cluster");
	serverLog(LL_LOG,"len: %d alloc: %d",SDS_HDR(8,t)->len,SDS_HDR(8,t)->alloc);
}


