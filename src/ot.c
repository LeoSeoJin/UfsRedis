#include "server.h"

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
	list->vertices = listCreate();
	return list;
}

cverlist *createCVerlist(sds key){
	cverlist *list = zmalloc(sizeof(cverlist));
	list->key = sdsnew(key);
	list->vertices = listCreate();
	return list;
}

dedge *createOpEdge(robj **argv, sds oid, vertice *v) {
	dedge *e = zmalloc(sizeof(dedge)); 
	e->argv = argv;
	//e->ctx = sdsnew(ctx);
	e->oid = sdsnew(oid);
	e->adjv = v;

	return e;
}

void removeOpEdge(dedge *edge) {
	if (!strcmp(edge->argv[0]->ptr,"unionot")) {
		zfree(edge->argv[1]);
		zfree(edge->argv[2]);
	} else {
		zfree(edge->argv[1]);
	}
	zfree(edge->argv);
	
	sdsfree(edge->oid);
	//listDelNode(space,edge->adjv);
	//sdsfree(edge->adjv->oids);
	//sdsfree(edge->adjv->content); //content is NULL
	//zfree(edge->adjv);
	//zfree(edge);
	serverLog(LL_LOG,"removeOpEdge finish");
	
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
			if (!v->ledge) serverLog(LL_LOG,"removeLastAddEdge finish");
		}	
	}	
}

int findClass(sds ufs, sds v) {
    int len;
    int result = -1;
    sds *elements = sdssplitlen(ufs,sdslen(ufs),"/",1,&len);

    for (int i = 0; i < len; i++) {
        if (strstr(elements[i], v)) {
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
sds cpltClass(sds ufs, sds v) {
    int len;
    sds *elements = sdssplitlen(ufs,sdslen(ufs),"/",1,&len);
	sds result;
    for (int i = 0; i < len; i++) {
        if (findSds(elements[i],v)) {
            elements[i] = sdsDel(elements[i],v);
            if (sdslen(elements[i])==0) {
    			//serverLog(LL_LOG,"*********************cpltclass of element is NULL");
    			//sdsfree(result);
    			result = sdsnew("*");
    		} else {
            	result = sdsnew(elements[i]);
            }
            break;
        } 
    }

    sdsfreesplitres(elements,len);
    return result;
}

//void ot(sds ufs, robj **op1, robj **op2, dedge *e1, dedge *e2, int flag) {
void otUfs(sds ufs, robj **op1, robj **op2, dedge *e1, dedge *e2, int flag) {
	robj **otop1, **otop2;
	//sds op1name = op1[0]->ptr;
	//sds op2name = op2[0]->ptr;
	sds op1name, op2name;
	if (op1[0] == shared.unionot) {
		op1name = shared.sdsunionot;
	} else {
		op1name = shared.sdssplitot;
	}
	
	if (op2[0] == shared.unionot) {
		op2name = shared.sdsunionot;
	} else {
		op2name = shared.sdssplitot;
	}
	serverLog(LL_LOG,"entering ot function");
	serverLogArgv(op1);
	serverLogArgv(op2);

	if (flag == 2) {
		serverLog(LL_LOG,"transform UNION operations");
		//transform union operations
		if (!strcmp(op1name,"unionot") && !strcmp(op2name,"splitot")) {
			//serverLog(LL_LOG,"case: union and split");
			sds uargv1 = op1[1]->ptr;
			sds uargv2 = op1[2]->ptr;
			sds sargv = op2[1]->ptr;
			
			serverLog(LL_LOG,"OT process: ufs: %s union %s %s, split %s ", ufs, uargv1, uargv2,sargv);

            otop1 = zmalloc(sizeof(robj *)*3);
            otop2 = zmalloc(sizeof(robj *)*2);
            //otop1[0] = createStringObject("unionot",7);
            //otop2[0] = createStringObject("splitot",7);
            serverLog(LL_LOG,"shared.unionot.refcount: %d, shared.splitot.refcount: %d",shared.unionot->refcount,shared.splitot->refcount);
            otop1[0] = shared.unionot;
            otop2[0] = shared.splitot;
            otop2[1] = createObject(OBJ_STRING,sargv);
            serverLog(LL_LOG,"shared.unionot.refcount: %d, shared.splitot.refcount: %d",shared.unionot->refcount,shared.splitot->refcount);
            
			//int uargv1len = sdslen(uargv1);
			//int uargv2len = sdslen(uargv2);
			int uargv1len = strchr(uargv1,',')==NULL?1:2;
			int uargv2len = strchr(uargv2,',')==NULL?1:2;
			if (uargv1len == 1 && uargv2len == 1) {
				if (!sdscmp(uargv1,uargv2)) {
					otop1[1] = createObject(OBJ_STRING,uargv1);
					otop1[2] = createObject(OBJ_STRING,uargv2);			
				} else if (!strcmp(uargv1,"*") || !strcmp(uargv2,"*")) {
					otop1[1] = createObject(OBJ_STRING,uargv1);
					otop1[2] = createObject(OBJ_STRING,uargv2);
				} else if (!sdscmp(uargv1,sargv)) {
					//serverLog(LL_LOG,"uargv1 == sargv");
					otop1[1] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
					otop1[2] = createObject(OBJ_STRING,uargv2);
				} else if (!sdscmp(uargv2,sargv)) {
					otop1[1] = createObject(OBJ_STRING,uargv1);
					otop1[2] = createObject(OBJ_STRING,cpltClass(ufs,sargv));	
				} else {
					otop1[1] = createObject(OBJ_STRING,uargv1);
					otop1[2] = createObject(OBJ_STRING,uargv2);		
				}
			}
			if (uargv1len > 1 && uargv2len == 1) {
				int len;
				sds *elements = sdssplitlen(uargv1,sdslen(uargv1),",",1,&len);
				if (find(elements,uargv2,len)) {
					//b in a
					if (find(elements,sargv,len)) {
					    //c in a
						otop1[1] = createObject(OBJ_STRING,cpltClass(ufs,sargv));		
						if (sdscmp(uargv2,sargv)) {
						    //b != c
							otop1[2] = createObject(OBJ_STRING,uargv2);
						} else {
						    //b == c 
						    otop1[2] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
						}
					} else {
					    //c not in a
						otop1[1] = createObject(OBJ_STRING,uargv1);
						otop1[2] = createObject(OBJ_STRING,uargv2);
					}
				} else {
				    //b not in a
					if (find(elements,sargv,len)) {
					    //c in a
						otop1[1] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
					} else {
					    //c not in a
						otop1[1] = createObject(OBJ_STRING,uargv1);
					}
					if (!sdscmp(uargv2,sargv)){
						otop1[2] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
					} else {
						otop1[2] = createObject(OBJ_STRING,uargv2);
					}
				}
				sdsfreesplitres(elements,len);
			}
			if (uargv1len == 1 && uargv2len > 1) {
				int len;
				sds *elements = sdssplitlen(uargv2,sdslen(uargv2),",",1,&len);
				if (find(elements,uargv1,len)) {
					if (find(elements,sargv,len)) {
						otop1[2] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
						if (sdscmp(uargv1,sargv)) {
							otop1[1] = createObject(OBJ_STRING,uargv1);	
						} else {
							otop1[1] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
						}
					} else {
						otop1[1] = createObject(OBJ_STRING,uargv1);
						otop1[2] = createObject(OBJ_STRING,uargv2);
					}
				} else {
					if (find(elements,sargv,len)) {
						otop1[2] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
					} else {
						otop1[2] = createObject(OBJ_STRING,uargv2);
					}
					if (!sdscmp(uargv1,sargv)){
						otop1[1] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
					} else {
						otop1[1] = createObject(OBJ_STRING,uargv1);
					}
				}
				sdsfreesplitres(elements,len);
			}
			if (uargv1len > 1 && uargv2len > 1) {
				if (!sdscntcmp(uargv1,uargv2)) { 
				    //a and b are sets, a == b 
					int len;
					sds *elements = sdssplitlen(uargv1,sdslen(uargv1),",",1,&len); 
					if (find(elements,sargv,len)) {
					    //c in a/b
						otop1[1] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
						otop1[2] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
					} else {
						otop1[1] = createObject(OBJ_STRING,uargv1);
						otop1[2] = createObject(OBJ_STRING,uargv2);
					}
					sdsfreesplitres(elements,len);
				} else {
				    //a,b are sets, a != b
					int len1,len2;
					sds *elements1 = sdssplitlen(uargv1,sdslen(uargv1),",",1,&len1);
					sds *elements2 = sdssplitlen(uargv2,sdslen(uargv2),",",1,&len2);
					
					if (find(elements1,sargv,len1)) {
						otop1[1] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
					} else {
						otop1[1] = createObject(OBJ_STRING,uargv1);
					}
					if (find(elements2,sargv,len2)){
						otop1[2] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
					} else {
						otop1[2] = createObject(OBJ_STRING,uargv2);
					}
					sdsfreesplitres(elements1,len1);
					sdsfreesplitres(elements2,len2);
				}
			}
		} else if (!strcmp(op1name,"splitot") && !strcmp(op2name,"unionot")) {
			sds uargv1 = op2[1]->ptr;
			sds uargv2 = op2[2]->ptr;
			sds sargv = op1[1]->ptr;
			
            otop1 = zmalloc(sizeof(robj *)*2);
            otop2 = zmalloc(sizeof(robj *)*3);
            //otop1[0] = createStringObject("splitot",7);
            otop1[0] = shared.splitot;
			otop1[1] = createObject(OBJ_STRING,sargv);
            //otop2[0] = createStringObject("unionot",7);
            otop2[0] = shared.unionot;

			//int uargv1len = sdslen(uargv1);
			//int uargv2len = sdslen(uargv2);
			int uargv1len = strchr(uargv1,',')==NULL?1:2;
			int uargv2len = strchr(uargv2,',')==NULL?1:2;
			if (uargv1len == 1 && uargv2len == 1) {
				if (!sdscmp(uargv1,uargv2)) {
					otop2[1] = createObject(OBJ_STRING,uargv1);
					otop2[2] = createObject(OBJ_STRING,uargv2);
				} else if (!strcmp(uargv1,"*") || !strcmp(uargv2,"*")) {
					otop2[1] = createObject(OBJ_STRING,uargv1);
					otop2[2] = createObject(OBJ_STRING,uargv2);
				} else if (!sdscmp(uargv1,sargv)) {
					otop2[1] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
					otop2[2] = createObject(OBJ_STRING,uargv2);
				} else if (!sdscmp(uargv2,sargv)) {
					otop2[1] = createObject(OBJ_STRING,uargv1);
					otop2[2] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
				} else {
					otop2[1] = createObject(OBJ_STRING,uargv1);
					otop2[2] = createObject(OBJ_STRING,uargv2);
				}
			}
			if (uargv1len > 1 && uargv2len == 1) {
				int len;
				sds *elements = sdssplitlen(uargv1,sdslen(uargv1),",",1,&len);
				if (find(elements,uargv2,len)) { 
					//b in a
					if (find(elements,sargv,len)) {
						otop2[1] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
						if (sdscmp(uargv2,sargv)) {
							otop2[2] = createObject(OBJ_STRING,uargv2);	
						} else {
							otop2[2] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
						}
					} else {
						otop2[1] = createObject(OBJ_STRING,uargv1);
						otop2[2] = createObject(OBJ_STRING,uargv2);
					}
				} else {
					if (find(elements,sargv,len)) {
						otop2[1] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
					} else {
						otop2[1] = createObject(OBJ_STRING,uargv1);
					}
					if (!sdscmp(uargv2,sargv)){
						otop2[2] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
					} else {
						otop2[2] = createObject(OBJ_STRING,uargv2);
					}
				}
				sdsfreesplitres(elements,len);
			}
			if (uargv1len == 1 && uargv2len > 1) {
				int len;
				sds *elements = sdssplitlen(uargv2,sdslen(uargv2),",",1,&len);
				if (find(elements,uargv1,len)) {
					if (find(elements,sargv,len)) {
						otop2[2] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
						if (sdscmp(uargv1,sargv)) {
							otop2[1] = createObject(OBJ_STRING,uargv1);	
						} else {
							otop2[1] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
						}
					} else {
						otop2[1] = createObject(OBJ_STRING,uargv1);
						otop2[2] = createObject(OBJ_STRING,uargv2);
					}
				} else {
					if (find(elements,sargv,len)) {
						otop2[2] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
					} else {
						otop2[2] = createObject(OBJ_STRING,uargv2);
					}
					if (!sdscmp(uargv1,sargv)){
						otop2[1] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
					} else {
						otop2[1] = createObject(OBJ_STRING,uargv1);
					}
				}
				sdsfreesplitres(elements,len);
			}
			if (uargv1len > 1 && uargv2len > 1) {
				if (!sdscntcmp(uargv1,uargv2)) { 
				    //a and b are sets, a == b
					int len;
					sds *elements = sdssplitlen(uargv1,sdslen(uargv1),",",1,&len);
					if (find(elements,sargv,len)) {
						otop2[1] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
						otop2[2] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
					} else {
						otop2[1] = createObject(OBJ_STRING,uargv1);
						otop2[2] = createObject(OBJ_STRING,uargv2);
					}
					sdsfreesplitres(elements,len);
				} else {
					int len1,len2;
					sds *elements1 = sdssplitlen(uargv1,sdslen(uargv1),",",1,&len1);
					sds *elements2 = sdssplitlen(uargv2,sdslen(uargv2),",",1,&len2);
					
					//a,b are sets, a != b
					if (find(elements1,sargv,len1)) {
						otop2[1] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
					} else {
						otop2[1] = createObject(OBJ_STRING,uargv1);
					}
					if (find(elements2,sargv,len2)){
						otop2[2] = createObject(OBJ_STRING,cpltClass(ufs,sargv));
					} else {
						otop2[2] = createObject(OBJ_STRING,uargv2);
					}
					sdsfreesplitres(elements1,len1);
					sdsfreesplitres(elements2,len2);
				}
			}		
		} else {
			//no transformation
			if (!strcmp(op1name,"unionot") && !strcmp(op2name,"unionot")) {
				otop1 = zmalloc(sizeof(robj *)*3);
				otop2 = zmalloc(sizeof(robj *)*3);
				//otop1[0] = createStringObject("unionot",7);
				otop1[0] = shared.unionot;
				otop1[1] = createObject(OBJ_STRING,op1[1]->ptr);
				otop1[2] = createObject(OBJ_STRING,op1[2]->ptr);
				//otop2[0] = createStringObject("unionot",7);
				otop2[0] = shared.unionot;
				otop2[1] = createObject(OBJ_STRING,op2[1]->ptr);
				otop2[2] = createObject(OBJ_STRING,op2[2]->ptr);			
			} 
			if (!strcmp(op1name,"splitot") && !strcmp(op2name,"splitot")) {
				otop1 = zmalloc(sizeof(robj *)*2);
				otop2 = zmalloc(sizeof(robj *)*2);
				//otop1[0] = createStringObject("splitot",7);
				otop1[0] = shared.splitot;
				otop1[1] = createObject(OBJ_STRING,op1[1]->ptr);
				//otop2[0] = createStringObject("splitot",7);
				otop2[0] = shared.splitot;
				otop2[1] = createObject(OBJ_STRING,op2[1]->ptr);				
			}
		}
	} 
	if (flag == 1) {
		//transform split operations
		if ((!strcmp(op1name,"unionot") && !strcmp(op2name,"splitot")) 
			  || (!strcmp(op2name,"unionot") && !strcmp(op1name,"splitot"))) {
				 sds uargv1,uargv2,sargv;
				 int sargvlen;
			if (!strcmp(op1name,"unionot") && !strcmp(op2name,"splitot")) {
				uargv1 = op1[1]->ptr;
				uargv2 = op1[2]->ptr;
				sargv = op2[1]->ptr;
				//sargvlen = sdslen(sargv);	
				sargvlen = strchr(sargv,',')==NULL?1:2;
				
				otop1 = zmalloc(sizeof(robj *)*3);
				otop2 = zmalloc(sizeof(robj *)*2);
				//otop1[0] = createStringObject("unionot",7);
				otop1[0] = shared.unionot;
				otop1[1] = createObject(OBJ_STRING,uargv1);
				otop1[2] = createObject(OBJ_STRING,uargv2);
				//otop2[0] = createStringObject("splitot",7);
				otop2[0] = shared.splitot;
				if (!sdscmp(uargv1,uargv2)) {
					otop2[1] = createObject(OBJ_STRING,sargv);
				} else {
					if (sargvlen == 1) {
						if (!strcmp(sargv,"*")) {
							otop2[1] = createObject(OBJ_STRING,sargv);
						} else if (findClass(ufs,uargv1) == findClass(ufs,uargv2)) {
					        //a and b in the same class, c is set to NULL
					        if (!strcmp(uargv1,sargv) || !strcmp(uargv2,sargv)) {
					            sds argv = sdsnew("*");
							    otop2[1] = createObject(OBJ_STRING,argv);
						    } else {
							    otop2[1] = createObject(OBJ_STRING,sargv);
						    }
					    } else {
					    	//a and b from different classes, consider c is equal to a/b
					    	if (!sdscmp(uargv1,sargv) || !sdscmp(uargv2,sargv)) {
					    	    //a/b == c, c = c.class-c
					    		otop2[1] = createObject(OBJ_STRING,cpltClass(ufs,sargv));					    						   	   
					    	} else {
								otop2[1] = createObject(OBJ_STRING,sargv);
					    	}
					    }						
					} else {
						int len;
						sds *elements = sdssplitlen(sargv,sdslen(sargv),",",1,&len);
					   	if (findClass(ufs,uargv1) == findClass(ufs,uargv2)) {
					   		if (find(elements,uargv1,len) && find(elements,uargv2,len)) {
					   			otop2[1] = createObject(OBJ_STRING,sargv);
					   		} else if (find(elements,uargv1,len) || find(elements,uargv2,len)) {
				    		    sds argv = sdsnew("*");
			    				otop2[1] = createObject(OBJ_STRING,argv);
		    				} else {
	    						otop2[1] = createObject(OBJ_STRING,sargv);
    						}
						} else {
							//a and b from different classes, consider a/b is in c(set) 
							if (find(elements,uargv1,len) || find(elements,uargv2,len)) {
					    		otop2[1] = createObject(OBJ_STRING,cpltClass(ufs,sargv));					    						   	   
					    	} else {
								otop2[1] = createObject(OBJ_STRING,sargv);
					    	}
						}
					}
				}
			} 
			if (!strcmp(op1name,"splitot") && !strcmp(op2name,"unionot") ) {
				uargv1 = op2[1]->ptr;
				uargv2 = op2[2]->ptr;
				sargv = op1[1]->ptr;
				//sargvlen = sdslen(sargv);
				sargvlen = strchr(sargv,',')==NULL?1:2;
					
				otop1 = zmalloc(sizeof(robj *)*2);
				otop2 = zmalloc(sizeof(robj *)*3);
				//otop2[0] = createStringObject("unionot",7);
				otop2[0] = shared.unionot;
				otop2[1] = createObject(OBJ_STRING,uargv1);
				otop2[2] = createObject(OBJ_STRING,uargv2);
				//otop1[0] = createStringObject("splitot",7);
				otop1[0] = shared.splitot;
				if (!sdscmp(uargv1,uargv2)) {
					otop1[1] = createObject(OBJ_STRING,sargv);
				} else {
					if (sargvlen == 1) {
						if (!strcmp(sargv,"*")) {
							otop1[1] = createObject(OBJ_STRING,sargv);
						} else if (findClass(ufs,uargv1) == findClass(ufs,uargv2)) {
					        //a and b in the same class, c is set to NULL
					        if (!sdscmp(uargv1,sargv) || !sdscmp(uargv2,sargv)) {
					            sds argv = sdsnew("*");
							    otop1[1] = createObject(OBJ_STRING,argv);
						    } else {
							    otop1[1] = createObject(OBJ_STRING,sargv);
						    }
					    } else {
					    	//a and b from different classes, consider c is equal to a/b
					    	if (!sdscmp(uargv1,sargv) || !sdscmp(uargv2,sargv)) {
					    	    //a/b == c, c = c.class-c
					    		otop1[1] = createObject(OBJ_STRING,cpltClass(ufs,sargv));					    						   	   
					    	} else {
								otop1[1] = createObject(OBJ_STRING,sargv);
					    	}
					    }
					} else {
					    int len;
                        sds *elements = sdssplitlen(sargv,sdslen(sargv),",",1,&len);
                        if (findClass(ufs,uargv1) == findClass(ufs,uargv2)) {
					   		if (find(elements,uargv1,len) && find(elements,uargv2,len)) {
					   			otop1[1] = createObject(OBJ_STRING,sargv);
					   		} else if (find(elements,uargv1,len) || find(elements,uargv2,len)) {
                                sds argv = sdsnew("*");
                                otop1[1] = createObject(OBJ_STRING,argv);
                            } else {
                                otop1[1] = createObject(OBJ_STRING,sargv);
                            }
                        } else {
							//a and b from different classes, consider a/b is in c(set) 
							if (find(elements,uargv1,len) || find(elements,uargv2,len)) {
					    		otop1[1] = createObject(OBJ_STRING,cpltClass(ufs,sargv));					    						   	   
					    	} else {
								otop1[1] = createObject(OBJ_STRING,sargv);
					    	}
						}				
			        }
			   }
		    } 
	    } else {
			//no transformation
			if (!strcmp(op1name,"unionot") && !strcmp(op2name,"unionot")) {
				otop1 = zmalloc(sizeof(robj *)*3);
				otop2 = zmalloc(sizeof(robj *)*3);
				//otop1[0] = createStringObject("unionot",7);
				otop1[0] = shared.unionot;
				otop1[1] = createObject(OBJ_STRING,op1[1]->ptr);
				otop1[2] = createObject(OBJ_STRING,op1[2]->ptr);
				//otop2[0] = createStringObject("unionot",7);
				otop2[0] = shared.unionot;
				otop2[1] = createObject(OBJ_STRING,op2[1]->ptr);
				otop2[2] = createObject(OBJ_STRING,op2[2]->ptr);			
			} 
			if (!strcmp(op1name,"splitot") && !strcmp(op2name,"splitot")) {
				otop1 = zmalloc(sizeof(robj *)*2);
				otop2 = zmalloc(sizeof(robj *)*2);
				//otop1[0] = createStringObject("splitot",7);
				otop1[0] = shared.splitot;
				otop1[1] = createObject(OBJ_STRING,op1[1]->ptr);
				//otop2[0] = createStringObject("splitot",7);
				otop2[0] = shared.splitot;
				otop2[1] = createObject(OBJ_STRING,op2[1]->ptr);			
			}
		}
	}
	serverLog(LL_LOG,"ot function finished");
	serverLogArgv(otop1);
	serverLogArgv(otop2);
	e1->argv = otop1;
	e2->argv = otop2;
}

/***
vertice *locateVertice(list* space, sds ctx) {
	if (listLength(space)) {
		//local processing (new generated operation)
		if (!ctx) {
			listNode *ln;
			listIter li;
			
			listRewind(space,&li);			
			while((ln = listNext(&li))) {
				vertice *v = ln->value;
				if (!v->ledge && !v->redge) {
					//serverLog("locateVertice: oids: %s ufs: %s ",v->oids,v->content);
					return v;
				}	
			}
		}
		else {
			//remote processing (received operation)
			listNode *ln;
			listIter li;
	
			listRewind(space,&li);
			while((ln = listNext(&li))) {
				vertice *v = ln->value;
				//serverLog(LL_LOG,"locateVerticeFunction: v: %s %s, ctx: %s",v->oids, v->content,ctx);
				//only require v->oids and ctx has the same operations, not need the same order
				if (!sdscntcmp(v->oids,ctx)) return v; 
			}			
		}
	} else {
		return NULL;
	}
}
***/

/***used for local processing
vertice *locateLVertice(list* space, sds ctx){
    listNode *ln;
	listIter li;
	
	listRewind(space,&li);			
	while((ln = listNext(&li))) {
		vertice *v = ln->value;
		if (!v->ledge && !v->redge) {
			//serverLog("locateVertice: oids: %s ufs: %s ",v->oids,v->content);
			return v;
		}	
	}
}

vertice *locateLVertice(vertice* q, sds ctx){
    vertice* t = NULL;
    if (q) {
        if (!q->ledge && !q->redge) {
            return q;
        } 
        if (q->ledge) {
            t = locateLVertice(q->ledge->adjv,oids);
        } 
        if (t) return t;
        if (q->redge) t = locateLVertice(q->redge->adjv,oids);
        return t;
    } else {
        return NULL;
    }
}
***/

/*oids is used to construct the oids of vertice*/
/****
vertice *locateLVertice(vertice* q, sds oids){
    vertice* t = NULL;
    sds ltemp = NULL;
    sds rtemp = NULL;
    if (oids) {
        ltemp = sdsempty();
        rtemp = sdsempty();
        ltemp = sdscat(ltemp,oids);
        rtemp = sdscat(rtemp,oids);
    }
    //serverLog(LL_LOG,"locatevertice: oids: %s ",oids);
    if (q) {
        if (!q->ledge && !q->redge) {
            return q;
        } 
        if (q->ledge) {
            if (ltemp) {
                ltemp = sdscat(ltemp,",");
                ltemp = sdscat(ltemp,q->ledge->oid);
                //serverLog(LL_LOG,"locatevertice: ledge loids: %s ",ltemp);
            }
            t = locateLVertice(q->ledge->adjv,ltemp);
        } 
        if (t) {
            sdsfree(oids);
            oids = sdsempty();
            oids = sdscat(oids,ltemp);
            if (!ltemp) sdsfree(ltemp);
            if (!rtemp) sdsfree(rtemp);
            serverLog(LL_LOG,"locatevertice: ledge return: %s ",oids);
            return t;
        }    
        if (q->redge) {
            if (rtemp) {
                rtemp = sdscat(rtemp,",");        
                rtemp = sdscat(rtemp,q->redge->oid);
                //serverLog(LL_LOG,"locatevertice: redge roids: %s ",rtemp);
            }
            t = locateLVertice(q->redge->adjv,rtemp);
        }
        sdsfree(oids);
        oids = sdsempty();
        oids = sdscat(oids,rtemp);
        if (!ltemp) sdsfree(ltemp);
        if (!rtemp) sdsfree(rtemp);
        serverLog(LL_LOG,"locatevertice: redge return: %s ",oids);
        return t;
    } else {
        return NULL;
    }
}
***/

//***
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
    if (!q) serverLog(LL_LOG,"locate to the last vertice fail"); 
}
/**/

sds calculateOids(vertice* v, sds oids){
    //serverLog(LL_LOG,"locatevertice: oids: %s ",oids);
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
}


vertice *locateRVertice(vertice* q, sds ctx, sds t) {
    sds temp = sdsnew(t); 
    sds ltemp = sdsnew(temp);
    sds rtemp = sdsnew(temp);
    vertice* lresult = NULL;
    vertice* rresult = NULL;
	if (q) {
	    if ((!strcmp(ctx,"init") && !strcmp(t,"init")) || !sdscntcmp(temp,ctx)) {
            sdsfree(temp);
            sdsfree(ltemp);
            sdsfree(rtemp);
            serverLog(LL_LOG,"locateRVertice: return vertex q: %s %s",t,q->content);
		    return q;
		}			
		if (q->ledge) {
		    ltemp = sdscat(ltemp,",");
		    ltemp = sdscat(ltemp,q->ledge->oid);
            lresult = locateRVertice(q->ledge->adjv,ctx,ltemp);
        }
        if (lresult) {
            sdsfree(temp);
            sdsfree(ltemp);
            sdsfree(rtemp);
            return lresult;
        } 
		if (q->redge) {
		    rtemp = sdscat(rtemp,",");
		    rtemp = sdscat(rtemp,q->redge->oid);
            rresult = locateRVertice(q->redge->adjv,ctx,rtemp);
        }
        
        sdsfree(temp);
        sdsfree(ltemp);
        sdsfree(rtemp);
        if (!rresult){
            serverLog(LL_LOG,"2D state space does not have the vertice");
		}
		return rresult;
	} else {
	    return NULL;
	}
}

//each vertice does not have oids
vertice *locateVertice(list* space, sds ctx) {
	if (listLength(space)) {
		//local processing (new generated operation)
		//serverLog(LL_LOG,"locatevertice local processing");
		if (!ctx) {
            return locateLVertice(space->head->value);
		}
		else {
			//remote processing (received operation)
			//serverLog(LL_LOG,"locatevertice remote processing");
			sds temp = sdsnew("init");
			vertice* result = locateRVertice(space->head->value,ctx,temp);
			sdsfree(temp);
			if (!result) serverLog(LL_LOG,"2D state space does not exist the vertex");
			//if (result) serverLog(LL_LOG,"locateRVertice: return result: %s %s",result->oids,result->content);
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
        serverLog(LL_LOG,"locateVerlist: s->id: %d, s->key: %s",s->id,s->key);
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
        serverLog(LL_LOG,"locateCVerlist: s->key: %s",s->key);
        if (!sdscmp(s->key,key)) return s;
    }
    return NULL;
}

int existCVerlist(sds key) {
    listNode *ln;
    listIter li;
	
	listRewind(cspacelist->spaces,&li);
    while((ln = listNext(&li))) {
        cverlist *s = ln->value;
        serverLog(LL_LOG,"locateCVerlist: s->key: %s",s->key);
        if (!sdscmp(s->key,key)) return 1;
    }
    return 0;
}

list* getSpace(sds key) {
    listNode *ln;
    listIter li;
	
	listRewind(cspacelist->spaces,&li);
    while((ln = listNext(&li))) {
        cverlist *s = ln->value;
        serverLog(LL_LOG,"locateCVerlist: s->key: %s",s->key);
        if (!sdscmp(s->key,key)) return s->vertices;
    }
    return NULL;
}

