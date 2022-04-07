#ifndef UTIL_H
#define UTIL_H

#include "type.h"
// Confilcting with main.c type.h and util.h
// Move it somewhere?
/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MTABLE mtable[NMTABLE];
extern PROC   proc[NPROC], *running;
extern OFT    oft[NOFT];
extern MINODE *root;

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, iblk, inode_start;

extern char line[128], cmd[32], pathname[128];

int get_block();
int put_block();
int tokenize();
MINODE* mialloc()
int midealloc(MINODE *);
MINODE* iget();
int iput();
int search();
int getino();
int findmyname();
int findino();

#endif