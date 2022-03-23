#ifndef UTIL_H
#define UTIL_H

#include "type.h"
// Confilcting with main.c type.h and util.h
// Move it somewhere?
/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, iblk;

extern char line[128], cmd[32], pathname[128];

int get_block();
int put_block();
int tokenize();
MINODE* iget();
void iput();
int search();
int getino();
int findmyname();
int findino();

#endif