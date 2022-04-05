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
extern int nblocks, ninodes, bmap, imap, iblk, inode_start;

extern char line[128], cmd[32], pathname[128];

int get_block();
int put_block();
int tokenize();
MINODE* iget();
int iput();
int search();
int getino();
int findmyname();
int findino();

/**** links.c ****/
int link();
int unlink();
int inode_truncate();
int symlink();
int readlink();

/**** mkdir_creat_rmdir.c ****/
int tst_bit();
int set_bit();
int devFreeInodes();
int ialloc();
int balloc();
int mkdir();
int kmkdir();
int enter_child();
int creat();
int kcreat();
int clr_bit();
int incFreeInodes();
int idalloc();
int bdalloc();
int rmdir();
int rm_child();

/**** cd_ls_pwd.c ****/
int cd();
int ls_file();
int ls_dir();
int ls();
int pwd();
int rpwd();

#endif