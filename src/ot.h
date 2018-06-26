#include "server.h"

typedef struct verticelist {
	sds key;
	int id;
	list *vertices;
}verlist;

typedef struct cverticelist {
	sds key;
	list *vertices;
}cverlist;

typedef struct serverStateSpace2D {
	list *spaces;
}sdss;
 
typedef struct clientStateSpace2D {
	list *spaces;
}cdss;

//void ot(sds ufs, struct robj** op1, struct robj **op2, struct robj ***top1, struct robj ***top2, int flag);
//vertice *createVertice(sds oids);
verlist *createVerlist(int id,sds key);
cverlist *createCVerlist(sds key);
list *getSpace(sds key);
int existCVerlist(sds key);
//dedge *createOpEdge((struct robj) **argv, sds ctx, sds oid, vertice *v);
//vertice *locateVertice(list *space, sds ctx);
verlist *locateVerlist(int id,sds key);
