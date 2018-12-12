#include "server.h"

typedef struct verticelist {
	sds key;
	int id;
	int len_ufs;
	list *vertices;
}verlist;

typedef struct cverticelist {
	sds key;
	int len_ufs;
	list *vertices;
}cverlist;

typedef struct serverStateSpace2D {
	list *spaces;
}sdss;
 
typedef struct clientStateSpace2D {
	list *spaces;
}cdss;

verlist *createVerlist(int id,sds key);
cverlist *createCVerlist(sds key);
list *getSpace(sds key);
list *getCverlistVertices(cverlist *c);
void setCverlistUfsLen(cverlist *c, int len);
void setVerlistUfsLen(verlist *v, int len);
int existCVerlist(sds key);
verlist *locateVerlist(int id,sds key);
