#include "server.h"

/*list: sds t: char**/
sds sdsDel(sds list, char *t) {
    //serverLog(LL_LOG,"sdsDel %s from %s",t,list);
    //if (strlen(t)>sdslen(list)) { serverLog(LL_LOG,"delete error");
    //serverLog(LL_LOG,"sdsDel %s(%d) from %s(%d)",t,(int)strlen(t),list,(int)sdslen(list));}
	
    int len,num,i,j,k;
    
	sds *elements = sdssplitlen(list,sdslen(list),",",1,&len);
    sds *del = sdssplitlen(t,strlen(t),",",1,&num);
    
    for (i = 0; i < len; i++) {
        for (j = 0; j < num; j++) {
	        if (!sdscmp(elements[i],del[j])) {
		        for (k = i; k < len-1; k++) {
			        sdsclear(elements[k]);
			        elements[k] = sdscat(elements[k],elements[k+1]);
			    }
			    for (k = j; k < num-1; k++) {
                    sdsclear(del[k]);
                    del[k] = sdscat(del[k],del[k+1]);
			    }
			    len--;
			    num--;
			    i--;
			    break;
        	}
    	}
   	}
   	
   	sdsfree(list);
   	list = sdsempty();
   	for (i = 0; i < len; i++) {
   		if (sdslen(elements[i]) != 0)   list = sdscat(list,elements[i]);
   		if (i != len-1) list = sdscat(list,",");
   	}

   	sdsfreesplitres(elements,len);
   	sdsfreesplitres(del,num);
    //serverLog(LL_LOG,"sdsDel result: %s",list);
   	return list;
}

/*v: sds/char* */
int find(sds *list, char *v, int len) {
    int result = 0;
	for (int i = 0; i < len; i++) {
	    //serverLog(LL_LOG,"find %s from %s",v,list[i]);
		if (!strcmp(v,list[i])) {
		    result = 1;
		    break;
		}	
	}
	//serverLog(LL_LOG,"find result: %d",result);
	return result;
}

/*
 *find a sds(element/oid) from sds*(eqc/oids) 
 *return 1(exist)
*/
int sdslistsds(sds *list, char *v, int len) {
    int result = 0;
	for (int i = 0; i < len; i++) {
	    //serverLog(LL_LOG,"find %s from %s",v,list[i]);
		if (list[i]) {
		    if (sdslen(list[i]) == sdslen(v)) { 
		      if (!memcmp(v,list[i],sdslen(list[i]))) {
		        sdsfree(list[i]);
		        list[i] = NULL;
		        result = 1;
		        break;
		      }
	        }
		}
	}
	return result;
}

int findSds(sds temp, char *s) {
    int result = 0;
    int i,len,num;
    //char *uf,*r;
    
    len = strchr(s,',')?1:0;
    num = strchr(temp,',')?1:0;
    
    /**
    int len_s = strlen(s);
    if (len == 1) {
        uf = (char*)zmalloc(len_s+1);
	    strcpy(uf,s);
        for (i = 0; i < len_s; i++)
		    if (uf[i] == ',') len++;
	}
    **/

	if (len != 0) {
	    if (num != 0){
	        sds list = sdsnew(",");
	        list = sdscat(list,temp);
	        list = sdscat(list,",");
	        
            //i = 0;
	        //r = strtok(uf,",");
            i = 0;
            char r[10] = "";
            while(s[i] != '\0') {
                if (s[i] == ',') break;
                else r[i] = s[i];
                i++;
            }
            r[i] = '\0';

            sds t = sdsnew(",");
            t = sdscat(t,r);
            t = sdscat(t,",");
            if (strstr(list,t)) result = 1;
            sdsfree(t);           
            sdsfree(list);
            /**
	        while (i<len && r != NULL) {
                sds t = sdsempty();
                t = sdscat(t,",");
                t = sdscat(t,r);
                t = sdscat(t,",");
                if (strstr(list,t)) result = 1;
                sdsfree(t);            
       		    r = strtok(NULL,",");
    		    i++;
	    	}
	    	sdsfree(list);
            **/
    	}
		//zfree(uf);
	} else {
	    if (num == 0) {
	        if (!strcmp(temp,s)) result = 1;
	    }else {
	        sds list = sdsnew(",");
	        list = sdscat(list,temp);
	        list = sdscat(list,",");
	        
    	    sds t = sdsnew(",");
    	    t = sdscat(t,s);
    	    t = sdscat(t,",");
            if (strstr(list,t)) result = 1;
            
            sdsfree(t);  
            sdsfree(list);
        }
	}	
    return result;
}

int findClass(sds ufs, char *v, int len_ufs) {
    int len;
    int result = -1;
    //if (!len_ufs) len_ufs = strlen(ufs);
    sds *elements = sdssplitlen(ufs,len_ufs,"/",1,&len);

    for (int i = 0; i < len; i++) {
        if (findSds(elements[i], v)) {
            result = i;
            break;
        }
    }
    sdsfreesplitres(elements,len);
    return result;
}


/*Return the complement class of v in corresponding class
 *ufs is the state of disjoint set
 */
char* cpltClass(char *ufs, char *v, int len_ufs) {
	sds result = sdsempty();
	if (v == NULL) {
	    result = sdscat(result,"*");
	} else {
        //if (!len_ufs) len_ufs = strlen(ufs);
        int len;
        sds *elements = sdssplitlen(ufs,len_ufs,"/",1,&len);
        for (int i = 0; i < len; i++) {
            if (findSds(elements[i],v)) {
                if (sdslen(elements[i]) == strlen(v)) {
        			result = sdscat(result,"*");
                } else {
                    elements[i] = sdsDel(elements[i],v);
                	result = sdscat(result,elements[i]);
                }
                break;
            } 
        }
        sdsfreesplitres(elements,len);
    }
    char *c = (char*)zmalloc(sdslen(result)+1);
    memcpy(c,result,sdslen(result));
    c[sdslen(result)] = '\0';
    
    sdsfree(result);
  
    return c;
}

int countstr(char *s, char t) {
    int result = 0;
    int i = 0;
    while (s[i] != '\0') {
        if (s[i] == t) result++;
        i++;
    }
    return result;
}

int eqccmp(char *s1, char *s2) {
	int result;
	int elements_num_s1 = countstr(s1,',')+1;
    int elements_num_s2 = countstr(s2,',')+1;

    if (elements_num_s1 != elements_num_s2) {
        result = 1;
    } else if (elements_num_s1 == 1) {
        if (!strcmp(s1,s2)) result = 0;
        else result = 1;
    } else {
        int len1,len2,i;
    	sds *oids1 = sdssplitlen(s1,strlen(s1),",",1,&len1);
    	sds *oids2 = sdssplitlen(s2,strlen(s2),",",1,&len2);
       	if (len1 != len2) {
    		result = 1;
    	} else {
    		for (i = 0; i < len1; i++) {
    			if (!sdslistsds(oids2, oids1[i], len1)) break; 
    		}
    		if (i == len1) {
    		    result = 0;
    		} else {
    		    result = 1;
    		}
    	}
	
    	sdsfreesplitres(oids1,len1);
    	sdsfreesplitres(oids2,len2);
    }
  	return result;
}

/*Compare contents of two equivalent class
 *same content, return 0
 */
/***
int eqccmp(char *s1, char *s2) {
	int len1,len2,i,result;
	                                  
	sds *oids1 = sdssplitlen(s1,strlen(s1),",",1,&len1);
	sds *oids2 = sdssplitlen(s2,strlen(s2),",",1,&len2);
	if (len1 != len2) {
		result = 1;
	} else {
		for (i = 0; i < len1; i++) {
			if (!sdslistsds(oids2, oids1[i], len1)) break; 
		}
		if (i == len1) {
		    result = 0;
		} else {
		    result = 1;
		}
	}
	
	sdsfreesplitres(oids1,len1);
	sdsfreesplitres(oids2,len2);
  	return result;
}
***/

int oidscmp(sds s1, sds s2) {
	int len1,len2,i,result;
	                                  
	sds *oids1 = sdssplitlen(s1,sdslen(s1),",",1,&len1);
	sds *oids2 = sdssplitlen(s2,sdslen(s2),",",1,&len2);
	//if (len1 != len2) {
		//result = 1;
	//} else {
		for (i = 0; i < len1; i++) {
			if (!sdslistsds(oids2, oids1[i], len1)) break; 
		}
		if (i == len1) {
		    result = 0;
		} else {
		    result = 1;
		}
	//}
	
	sdsfreesplitres(oids1,len1);
	sdsfreesplitres(oids2,len2);
  	return result;
}

vertice *createVertice(){
	vertice *v = zmalloc(sizeof(vertice));
	//v->oids = sdsnew(oids);
	//v->oids = NULL;
	//if (oids) v->oids = sdsnew(oids);
	v->ledge = NULL;
	v->redge = NULL;
	return v;
}

verlist *createVerlist(int id, sds key) {
	verlist *list = zmalloc(sizeof(verlist));
	list->id = id;
	list->key = sdsnew(key);
	//list->key = sdsempty();
	//list->key = sdscat(list->key,key);
	list->vertices = listCreate();
	return list;
}

cverlist *createCVerlist(sds key){
	cverlist *list = zmalloc(sizeof(cverlist));
	list->key = sdsnew(key);
	//list->key = sdsempty();
	//list->key = sdscat(list->key,key);
	list->vertices = listCreate();
	return list;
}

dedge *createOpEdge(int type, char *t1, char *t2, char *t3, vertice *v) {
    //serverLog(LL_LOG,"createOpedge: %d %s %s %s",type,t1,t2,t3);
	dedge *e = zmalloc(sizeof(dedge)); 
	e->optype = type;
	e->argv1 = t1;
	e->argv2 = t2;
    e->oid = t3;
	e->adjv = v;
	//serverLog(LL_LOG,"createOpedge: %d %s %s %s",e->optype,e->argv1,e->argv2,e->oid);
	return e;
}

void setOp(dedge *e, int t, sds t1, sds t2) {
    e->optype = t;
    e->argv1 = t1;
    e->argv2 = t2;
}

void removeOpEdge(dedge *edge) {
	if (edge->optype == OPTYPE_UNION) {
		sdsfree(edge->argv1);
		sdsfree(edge->argv2);
	} else {
		sdsfree(edge->argv1);
	}
	sdsfree(edge->oid);
	//listDelNode(space,edge->adjv);
	//sdsfree(edge->adjv->oids);
	//sdsfree(edge->adjv->content); //content is NULL
	//zfree(edge->adjv);
	//zfree(edge);
	//serverLog(LL_LOG,"removeOpEdge finish");
	
}

void removeLastAddEdge(list *space) {
	op_num--;
	
	listNode *ln, *lnl;
	listIter li;
		
	listRewind(space,&li);
	vertice *v;		
	while((ln = listNext(&li))) {
		v = ln->value;
		if (v->ledge && !v->redge) {
			//remove ledge and free memory
			removeOpEdge(v->ledge);
			lnl = listNext(&li);
			listDelNode(space,lnl);
			zfree(v->ledge);
			v->ledge = NULL; 
			//if (!v->ledge) serverLog(LL_LOG,"removeLastAddEdge finish");
		}	
	}	
}


void otUfs(char *ufs, dedge *op1, dedge *op2, dedge *e1, dedge *e2, int flag, int len_ufs) {
	int otop1type = 0;
	int otop2type = 0;
	char *otop1argv1 = NULL;
	char *otop1argv2 = NULL;
	char *otop2argv1 = NULL;
	char *otop2argv2 = NULL;

	int op1type = op1->optype;
    int op2type = op2->optype;

    //serverLog(LL_LOG,"************************");
	//serverLog(LL_LOG,"entering ot: ufs: %s",ufs);	
	//serverLogArgv(op1,op2);

	if (flag == 2) {
		//serverLog(LL_LOG,"transform UNION operations");
		if (op1type == OPTYPE_UNION && op2type == OPTYPE_SPLIT) {
			//serverLog(LL_LOG,"case: union and split");
			char *uargv1 = op1->argv1;
			char *uargv2 = op1->argv2;
			char *sargv = op2->argv1;
			
			//serverLog(LL_LOG,"OT process(change union): ufs: %s union %s %s, split %s ", ufs, uargv1, uargv2,sargv);

            otop1type = OPTYPE_UNION;
            otop2type = OPTYPE_SPLIT;
            otop2argv1 = sargv;
			int uargv1len = strchr(uargv1,',')==NULL?1:2;
			int uargv2len = strchr(uargv2,',')==NULL?1:2;
			
			if (uargv1len == 1 && uargv2len == 1) {
				if (!strcmp(uargv1,uargv2)) {
					otop1argv1 = uargv1;
					otop1argv2 = uargv2;
				} else if (!strcmp(uargv1,"*") || !strcmp(uargv2,"*")) {
					otop1argv1 = uargv1;
					otop1argv2 = uargv2;					
				} else if (!strcmp(uargv1,sargv)) {
					otop1argv1 = cpltClass(ufs,sargv,len_ufs);
					otop1argv2 = uargv2;
				} else if (!strcmp(uargv2,sargv)) {
					otop1argv1 = uargv1;
					otop1argv2 = cpltClass(ufs,sargv,len_ufs);
				} else {
					otop1argv1 = uargv1;
					otop1argv2 = uargv2;	
				}
			}
			if (uargv1len > 1 && uargv2len == 1) {
				int len;
				sds *elements = sdssplitlen(uargv1,strlen(uargv1),",",1,&len);
				if (find(elements,uargv2,len)) {
					//b in a
					if (find(elements,sargv,len)) {
					    //c in a
						otop1argv1 = cpltClass(ufs,sargv,len_ufs);
						if (strcmp(uargv2,sargv)) {
						    //b != c
							otop1argv2 = uargv2;
						} else {
						    //b == c 
						    otop1argv2 = cpltClass(ufs,sargv,len_ufs);
						}
					} else {
					    //c not in a
					    otop1argv1 = uargv1;
					    otop1argv2 = uargv2;
					}
				} else {
				    //b not in a
					if (find(elements,sargv,len)) {
					    //c in a
						otop1argv1 = cpltClass(ufs,sargv,len_ufs);
					} else {
					    //c not in a
						otop1argv1 = uargv1;
					}
					if (!strcmp(uargv2,sargv)){
						otop1argv2 = cpltClass(ufs,sargv,len_ufs);
					} else {
						otop1argv2 = uargv2;
					}
				}
				sdsfreesplitres(elements,len);
			}
			if (uargv1len == 1 && uargv2len > 1) {
				int len;
				sds *elements = sdssplitlen(uargv2,strlen(uargv2),",",1,&len);
				if (find(elements,uargv1,len)) {
					if (find(elements,sargv,len)) {
						otop1argv2 = cpltClass(ufs,sargv,len_ufs);
						if (strcmp(uargv1,sargv)) {
							otop1argv1 = uargv1;
						} else {
							otop1argv1 = cpltClass(ufs,sargv,len_ufs);
						}
					} else {
						otop1argv1 = uargv1;
						otop1argv2 = uargv2;
					}
				} else {
					if (find(elements,sargv,len)) {
						otop1argv2 = cpltClass(ufs,sargv,len_ufs);
					} else {
						otop1argv2 = uargv2;
					}
					if (!strcmp(uargv1,sargv)){
						otop1argv1 = cpltClass(ufs,sargv,len_ufs);
					} else {
						otop1argv1 = uargv1;
					}
				}
				sdsfreesplitres(elements,len);
			}
			if (uargv1len > 1 && uargv2len > 1) {
				if (!eqccmp(uargv1,uargv2)) { 
				    //a and b are sets, a == b 
					int len;
					sds *elements = sdssplitlen(uargv1,strlen(uargv1),",",1,&len); 
					if (find(elements,sargv,len)) {
					    //c in a/b
						otop1argv1 = cpltClass(ufs,sargv,len_ufs);
						otop1argv2 = cpltClass(ufs,sargv,len_ufs);
					} else {
						otop1argv1 = uargv1;
						otop1argv2 = uargv2;
					}
					sdsfreesplitres(elements,len);
				} else {
				    //a,b are sets, a != b
					int len1,len2;
					sds *elements1 = sdssplitlen(uargv1,strlen(uargv1),",",1,&len1);
					sds *elements2 = sdssplitlen(uargv2,strlen(uargv2),",",1,&len2);
					
					if (find(elements1,sargv,len1)) {
						otop1argv1 = cpltClass(ufs,sargv,len_ufs);
					} else {
						otop1argv1 = uargv1;
					}
					if (find(elements2,sargv,len2)){
						otop1argv2 = cpltClass(ufs,sargv,len_ufs);
					} else {
						otop1argv2 = uargv2;
					}
					sdsfreesplitres(elements1,len1);
					sdsfreesplitres(elements2,len2);
				}
			}
		} else if (op1type == OPTYPE_SPLIT && op2type == OPTYPE_UNION) {
			char *uargv1 = op2->argv1;
			char *uargv2 = op2->argv2;
			char *sargv = op1->argv1;
			
            otop1type = OPTYPE_SPLIT;
            otop1argv1 = sargv;
            otop2type = OPTYPE_UNION;
            
			int uargv1len = strchr(uargv1,',')==NULL?1:2;
			int uargv2len = strchr(uargv2,',')==NULL?1:2;
			if (uargv1len == 1 && uargv2len == 1) {
				if (!strcmp(uargv1,uargv2)) {
					otop2argv1 = uargv1;
					otop2argv2 = uargv2;
				} else if (!strcmp(uargv1,"*") || !strcmp(uargv2,"*")) {
					otop2argv1 = uargv1;
					otop2argv2 = uargv2;					
				} else if (!strcmp(uargv1,sargv)) {
					otop2argv1 = cpltClass(ufs,sargv,len_ufs);
					otop2argv2 = uargv2;
				} else if (!strcmp(uargv2,sargv)) {
					otop2argv1 = uargv1;
					otop2argv2 = cpltClass(ufs,sargv,len_ufs);
				} else {
					otop2argv1 = uargv1;
					otop2argv2 = uargv2;
				}
			}
			if (uargv1len > 1 && uargv2len == 1) {
				int len;
				sds *elements = sdssplitlen(uargv1,strlen(uargv1),",",1,&len);
				if (find(elements,uargv2,len)) { 
					//b in a
					if (find(elements,sargv,len)) {
						otop2argv1 = cpltClass(ufs,sargv,len_ufs);
						if (strcmp(uargv2,sargv)) {
							otop2argv2 = uargv2;
						} else {
							otop2argv2 = cpltClass(ufs,sargv,len_ufs);
						}
					} else {
						otop2argv1 = uargv1;
						otop2argv2 = uargv2;
					}
				} else {
					if (find(elements,sargv,len)) {
						otop2argv1 = cpltClass(ufs,sargv,len_ufs);
					} else {
						otop2argv1 = uargv1;
					}
					if (!strcmp(uargv2,sargv)){
						otop2argv2 = cpltClass(ufs,sargv,len_ufs);
					} else {
						otop2argv2 = uargv2;
					}
				}
				sdsfreesplitres(elements,len);
			}
			if (uargv1len == 1 && uargv2len > 1) {
				int len;
				sds *elements = sdssplitlen(uargv2,strlen(uargv2),",",1,&len);
				if (find(elements,uargv1,len)) {
					if (find(elements,sargv,len)) {
						otop2argv2 = cpltClass(ufs,sargv,len_ufs);
						if (strcmp(uargv1,sargv)) {
							otop2argv1 = uargv1;
						} else {
							otop2argv1 = cpltClass(ufs,sargv,len_ufs);
						}
					} else {
						otop2argv1 = uargv1;
						otop2argv2 = uargv2;
					}
				} else {
					if (find(elements,sargv,len)) {
						otop2argv2 = cpltClass(ufs,sargv,len_ufs);
					} else {
						otop2argv2 = uargv2;
					}
					if (!strcmp(uargv1,sargv)){
						otop2argv1 = cpltClass(ufs,sargv,len_ufs);
					} else {
						otop2argv1 = uargv1;
					}
				}
				sdsfreesplitres(elements,len);
			}
			if (uargv1len > 1 && uargv2len > 1) {
				if (!eqccmp(uargv1,uargv2)) { 
				    //a and b are sets, a == b
					int len;
					sds *elements = sdssplitlen(uargv1,strlen(uargv1),",",1,&len);
					if (find(elements,sargv,len)) {
						otop2argv1 = cpltClass(ufs,sargv,len_ufs);
						otop2argv2 = cpltClass(ufs,sargv,len_ufs);
					} else {
						otop2argv1 = uargv1;
						otop2argv2 = uargv2;
					}
					sdsfreesplitres(elements,len);
				} else {
					int len1,len2;
					sds *elements1 = sdssplitlen(uargv1,strlen(uargv1),",",1,&len1);
					sds *elements2 = sdssplitlen(uargv2,strlen(uargv2),",",1,&len2);
					
					//a,b are sets, a != b
					if (find(elements1,sargv,len1)) {
						otop2argv1 = cpltClass(ufs,sargv,len_ufs);
					} else {
						otop2argv1 = uargv1;
					}
					if (find(elements2,sargv,len2)){
						otop2argv2 = cpltClass(ufs,sargv,len_ufs);
					} else {
						otop2argv2 = uargv2;
					}
					sdsfreesplitres(elements1,len1);
					sdsfreesplitres(elements2,len2);
				}
			}		
		} else {
			//no transformation
			if (op1type==OPTYPE_UNION && op2type==OPTYPE_UNION) {
				otop1type = OPTYPE_UNION;
				otop2type = OPTYPE_UNION;
				otop1argv1 = op1->argv1;
				otop1argv2 = op1->argv2;
				otop2argv1 = op2->argv1;
				otop2argv2 = op2->argv2;				
			} 
			if (op1type==OPTYPE_SPLIT && op2type==OPTYPE_SPLIT) {
				otop1type = OPTYPE_SPLIT;
				otop2type = OPTYPE_SPLIT;
				otop1argv1 = op1->argv1;
				otop2argv1 = op2->argv1;
			}
		}
	} 
	if (flag == 1) {
		//transform split operations
		if ((op1type == OPTYPE_UNION && op2type == OPTYPE_SPLIT) 
			  || (op2type == OPTYPE_UNION && op1type == OPTYPE_SPLIT)) {
				 char *uargv1,*uargv2,*sargv;
				 int sargvlen;
			if (op1type == OPTYPE_UNION && op2type == OPTYPE_SPLIT) {
				uargv1 = op1->argv1;
				uargv2 = op1->argv2;
				sargv = op2->argv1;
                //serverLog(LL_LOG,"union %s %s(%s) split %s(%s)",uargv1,uargv2,op1->oid,sargv,op2->oid);
				sargvlen = strchr(sargv,',')==NULL?1:2;

				otop1type = OPTYPE_UNION;
				otop2type = OPTYPE_SPLIT;
				otop1argv1 = uargv1;
				otop1argv2 = uargv2;
				
				if (!strcmp(uargv1,uargv2)) {
					otop2argv1 = sargv;
				} else {
					if (sargvlen == 1) {
						if (!strcmp(sargv,"*")) {
							otop2argv1 = sargv;							
						} else {
						if (findClass(ufs,uargv1,len_ufs) == findClass(ufs,uargv2,len_ufs)) {
					        //a and b in the same class, c is set to NULL
					        if (!strcmp(uargv1,sargv) || !strcmp(uargv2,sargv)) {					            
							    otop2argv1 = shared.star;
						    } else {
							    otop2argv1 = sargv;
						    }
					    } else {
					    	//a and b from different classes, consider c is equal to a/b
					    	if (!strcmp(uargv1,sargv) || !strcmp(uargv2,sargv)) {
					    	    //a/b == c, c = c.class-c
					    		otop2argv1 = cpltClass(ufs,sargv,len_ufs);
					    	} else {
								otop2argv1 = sargv;
					    	}
					    }
					    }						
					} else {
						int len;
						sds *elements = sdssplitlen(sargv,strlen(sargv),",",1,&len);
					   	if (findClass(ufs,uargv1,len_ufs) == findClass(ufs,uargv2,len_ufs)) {
					   		if (find(elements,uargv1,len) && find(elements,uargv2,len)) {
					   			otop2argv1 = sargv;
					   		} else if (find(elements,uargv1,len) || find(elements,uargv2,len)) {				    		    
			    				otop2argv1 = shared.star;
		    				} else {
	    						otop2argv1 = sargv;
    						}
						} else {
							//a and b from different classes, consider a/b is in c(set) 
							if (find(elements,uargv1,len) || find(elements,uargv2,len)) {			    						   	   
					    		otop2argv1 = cpltClass(ufs,sargv,len_ufs);
					    	} else {
								//otop2[1] = createObject(OBJ_STRING,sargv);
								otop2argv1 = sargv;
					    	}
						}
						sdsfreesplitres(elements,len);
					}
				}
			} 
			if (op1type == OPTYPE_SPLIT && op2type == OPTYPE_UNION) {
				uargv1 = op2->argv1;
				uargv2 = op2->argv2;
				sargv = op1->argv1;
                //serverLog(LL_LOG,"split %s(%s)  union %s %s(%s)",sargv,op1->oid,uargv1,uargv2,op2->oid);
				sargvlen = strchr(sargv,',')==NULL?1:2;

				otop1type = OPTYPE_SPLIT;
				otop2type = OPTYPE_UNION;
				otop2argv1 = uargv1;
				otop2argv2 = uargv2;
				
				if (!strcmp(uargv1,uargv2)) {
					otop1argv1 = sargv;
				} else {
					if (sargvlen == 1) {
						if (!strcmp(sargv,"*")) {
							otop1argv1 = sargv;
						} else if (findClass(ufs,uargv1,len_ufs) == findClass(ufs,uargv2,len_ufs)) {
					        //a and b in the same class, c is set to NULL
					        if (!strcmp(uargv1,sargv) || !strcmp(uargv2,sargv)) {
							    otop1argv1 = shared.star;
						    } else {
							    otop1argv1 = sargv;
						    }
					    } else {
					    	//a and b from different classes, consider c is equal to a/b
					    	if (!strcmp(uargv1,sargv) || !strcmp(uargv2,sargv)) {
					    	    //a/b == c, c = c.class-c			    						   	   
					    		otop1argv1 = cpltClass(ufs,sargv,len_ufs);
					    	} else {
								otop1argv1 = sargv;
					    	}
					    }
					} else {
					    int len;
                        sds *elements = sdssplitlen(sargv,strlen(sargv),",",1,&len);
                        if (findClass(ufs,uargv1,len_ufs) == findClass(ufs,uargv2,len_ufs)) {
					   		if (find(elements,uargv1,len) && find(elements,uargv2,len)) {
					   			otop1argv1 = sargv;
					   		} else if (find(elements,uargv1,len) || find(elements,uargv2,len)) {
                                otop1argv1 = shared.star;
                            } else {
                                otop1argv1 = sargv;
                            }
                        } else {
							//a and b from different classes, consider a/b is in c(set) 
							if (find(elements,uargv1,len) || find(elements,uargv2,len)) {			    						   	   
					    		otop1argv1 = cpltClass(ufs,sargv,len_ufs);
					    	} else {
								otop1argv1 = sargv;
					    	}
						}
						sdsfreesplitres(elements,len);				
			        }
			   }
		    } 
	    } else if (op1type == OPTYPE_SPLIT && op2type == OPTYPE_SPLIT) {
	        char *s1,*s2;
			int s1len,s2len;
			
			s1 = op1->argv1;
			s2 = op2->argv1;
            
            //serverLog(LL_LOG,"++++split %s(%s)  split %s(%s)",s1,op1->oid,s2,op2->oid);
			s1len = strchr(s1,',')==NULL?1:2;
			s2len = strchr(s2,',')==NULL?1:2;
			
			otop1type = OPTYPE_SPLIT;
			otop2type = OPTYPE_SPLIT;
			
			if (s1len == 1 && s2len == 1) {
			    otop1argv1 = s1;
			    otop2argv1 = s2;
			} else if (s1len == 1 && s2len > 1) {
			    if (!strcmp(s1,"*")) {
			        otop1argv1 = s1;
			        otop2argv1 = s2;
			    } else {
				    int len;
                    sds *elements = sdssplitlen(s2,strlen(s2),",",1,&len);  
                
                    if (find(elements,s1,len)) otop2argv1 = cpltClass(ufs,s2,len_ufs);
                    else otop2argv1 = s2;
                    otop1argv1 = s1;
                
                    sdsfreesplitres(elements,len);
                }
			} else if (s1len > 1 && s2len == 1) {
			    if (!strcmp(s2,"*")) {
			        otop1argv1 = s1;
			        otop2argv1 = s2;
			    } else {
				    int len;
                    sds *elements = sdssplitlen(s1,strlen(s1),",",1,&len);  

                    if (find(elements,s2,len)) otop1argv1 = cpltClass(ufs,s1,len_ufs);
                    else otop1argv1 = s1;
                    otop2argv1 = s2;
                                
                    sdsfreesplitres(elements,len);
                }
			} else {
				int len1,len2,i;
                sds *elements1 = sdssplitlen(s1,strlen(s1),",",1,&len1);  
                sds *elements2 = sdssplitlen(s2,strlen(s2),",",1,&len2); 

                sds overlap = NULL;
                sds s1nos2 = NULL;
                sds s2nos1 = NULL;
                
                int overlap_num = 0;
                
                for (i = 0; i < len1; i++) {
                    if (find(elements2,elements1[i],len2)) {
                       if (overlap == NULL) overlap = sdsempty();
                       else overlap = sdscat(overlap,",");
                       overlap = sdscat(overlap,elements1[i]);                             
                       overlap_num++;
                    } else {
                       if (s1nos2 == NULL) s1nos2 = sdsempty();
                       else s1nos2 = sdscat(s1nos2,",");
                       s1nos2 = sdscat(s1nos2,elements1[i]);
                    }
                }
                if (overlap_num != 0) { 
                    for (i = 0; i < len2; i++) {
                        if (!find(elements1,elements2[i],len1)) {
                           if (s2nos1 == NULL) s2nos1 = sdsempty();
                           else s2nos1 = sdscat(s2nos1,",");
                           s2nos1 = sdscat(s2nos1,elements2[i]);
                        }
                    }
                }
                
                //serverLog(LL_LOG,"overlap: %s %d s1nos2: %s s2nos1: %s",overlap,overlap_num,s1nos2,s2nos1);
                
                if (overlap_num == 0) {
                    otop1argv1 = s1;
                    otop2argv1 = s2;
                } else if (overlap_num == len1 || overlap_num == len2) {
                    if (len1 == len2) {
                        otop1argv1 = cpltClass(ufs,NULL,len_ufs);
                        otop2argv1 = cpltClass(ufs,NULL,len_ufs);
                    } else {
                        if (overlap_num == len1) {
                            otop2argv1 = (char*)zmalloc(sdslen(s2nos1)+1);
                            strcpy(otop2argv1,s2nos1);
                            otop1argv1 = s1;                                                       
                        } else {
                            otop1argv1 = (char*)zmalloc(sdslen(s1nos2)+1);
                            strcpy(otop1argv1,s1nos2);
                            otop2argv1 = s2;
                        }                        
                    }
                } else {
				    int l1,l2;
                    sds *e1 = sdssplitlen(s1nos2,sdslen(s1nos2),",",1,&l1);  
                    sds *e2 = sdssplitlen(s2nos1,sdslen(s2nos1),",",1,&l2);
                    
                    otop1type = OPTYPE_UNION;
                    otop2type = OPTYPE_UNION;
                    otop1argv1 = (char*)zmalloc(sdslen(e1[0])+1);
                    strcpy(otop1argv1,e1[0]);
                    otop1argv2 = (char*)zmalloc(sdslen(e2[0])+1);
                    strcpy(otop1argv2,e2[0]);
                    otop2argv1 = otop1argv1;
                    otop2argv2 = otop1argv2;
                    
                    sdsfreesplitres(e1,l1);
                    sdsfreesplitres(e2,l2);
                }             
               
                sdsfree(overlap);
                sdsfree(s1nos2);
                sdsfree(s2nos1);
                sdsfreesplitres(elements1,len1);
                sdsfreesplitres(elements2,len2);
			}
	    } else {
				otop1type = OPTYPE_UNION;
				otop2type = OPTYPE_UNION;
				otop1argv1 = op1->argv1;
				otop1argv2 = op1->argv2;
				otop2argv1 = op2->argv1;
				otop2argv2 = op2->argv2;
		}
	}
	//serverLog(LL_LOG,"ot function finished");
	setOp(e1,otop1type,otop1argv1,otop1argv2);
	setOp(e2,otop2type,otop2argv1,otop2argv2);
	//serverLogArgv(e1,e2);
	//serverLog(LL_LOG,"************************");
	//serverLog(LL_LOG,"ot successed!!");
}

vertice *locateLVertice(vertice* v){
    vertice *q = v;
    while (q) {
        if (!q->ledge && !q->redge) {
            return q;
        } 
        if (q->ledge) {
             q = q->ledge->adjv;
        } else {
            q = q->redge->adjv;
        }
    }
    serverLog(LL_LOG,"locate to the last vertice fail"); 
    return NULL;
}

sds calculateOids(vertice* v, sds oids){
    vertice *q = v;
    while (q) {
        if (!q->ledge && !q->redge) {
            //serverLog(LL_LOG,"locateLVertice return q: %s",t);
            return oids;
        } 
        if (q->ledge) {
             oids = sdscat(oids,",");
             oids = sdscat(oids,q->ledge->oid);
             q = q->ledge->adjv;
        } else {
            oids = sdscat(oids,",");
            oids = sdscat(oids,q->redge->oid);
            q = q->redge->adjv;
        }
    }
    return NULL;
}

int findOid(sds list, char *s) {
    int result = 0;	        
    sds t = sdsnew(",");
    t = sdscat(t,s);

    if (strstr(list,t)) result = 1;            

    sdsfree(t);  
    return result;
}

vertice *locateRVertice(vertice* q, sds ctx) {
    sds t = sdsnew("init");
	while (q) {
		if (sdslen(t) == sdslen(ctx)) {
		    if (sdslen(t) == 4) {
		      if (!strcmp(ctx,"init")) {
		        sdsfree(t);
		        return q;
		      } else {
		        sdsfree(t);
		        return NULL;
		      }
		    } else if (!oidscmp(t,ctx)) {
                sdsfree(t); 
        		return q;		    
            } else {
                sdsfree(t);
                return NULL;
            }
		} else if (q->ledge && findOid(ctx,q->ledge->oid)) {
		    t = sdscat(t,",");
		    t = sdscat(t,q->ledge->oid);
		    q = q->ledge->adjv;
	        //serverLog(LL_LOG,"%s",t);
		} else if (q->redge && findOid(ctx,q->redge->oid)) {
		    t = sdscat(t,",");
		    t = sdscat(t,q->redge->oid);
		    q = q->redge->adjv;
	        //serverLog(LL_LOG,"%s",t);
		} else {
		    sdsfree(t);
		    return NULL;
		}
	} 
	sdsfree(t);
	return NULL;
}

vertice *locateVertice(list* space, sds ctx) {
	if (listLength(space)) {
		if (!ctx) {
            return locateLVertice(space->head->value);
		}
		else {
			vertice* result = locateRVertice(space->head->value,ctx);
			if (!result) serverLog(LL_LOG,"2D state space does not exist the vertex");
			return result;    
	    }
	} else {
	    serverLog(LL_LOG,"space has no elements");
		return NULL;
	}
}

verlist *locateVerlist(int id,sds key) {
    listNode *ln;
    listIter li;
	
	listRewind(sspacelist->spaces,&li);
    while((ln = listNext(&li))) {
        verlist *s = ln->value;
        if (s->id == id && !sdscmp(s->key,key)) return s;
    }
    return NULL;
}

cverlist *locateCVerlist(sds key) {
    listNode *ln;
    listIter li;
	
	listRewind(cspacelist->spaces,&li);
    while((ln = listNext(&li))) {
        cverlist *s = ln->value;
        //serverLog(LL_LOG,"locateCVerlist: s->key: %s",s->key);
        if (!sdscmp(s->key,key)) return s;
    }
    return NULL;
}

void setCverlistUfsLen(cverlist *c, int len) {
    c->len_ufs = len;
}

void setVerlistUfsLen(verlist *v, int len) {
    v->len_ufs = len;
}

int existCVerlist(sds key) {
    listNode *ln;
    listIter li;
	
	listRewind(cspacelist->spaces,&li);
    while((ln = listNext(&li))) {
        cverlist *s = ln->value;
        //serverLog(LL_LOG,"locateCVerlist: s->key: %s",s->key);
        if (!sdscmp(s->key,key)) return 1;
    }
    return 0;
}

list* getCverlistVertices(cverlist *c) {
    return c->vertices;
}

list* getSpace(sds key) {
    listNode *ln;
    listIter li;
	
	listRewind(cspacelist->spaces,&li);
    while((ln = listNext(&li))) {
        cverlist *s = ln->value;
        //serverLog(LL_LOG,"locateCVerlist: s->key: %s",s->key);
        if (!sdscmp(s->key,key)) return s->vertices;
    }
    return NULL;
}

