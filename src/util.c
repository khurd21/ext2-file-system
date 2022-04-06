/*********** util.c file ****************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "type.h"

MINODE *root;
MINODE minode[NMINODE];
PROC   proc[NPROC], *running;

char gpath[128]; // global for tokenized components
char *name[64];  // assume at most 64 components in pathname
int  n;         // number of component strings

int nblocks, ninodes, bmap, imap, iblk, inode_start;
char line[128], cmd[32], pathname[128];
int fd, dev;

int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
   return 0;
}   

int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
   return 0;
}   

int tokenize(char *pathname)
{
  int i;
  char *s;
  printf("tokenize %s\n", pathname);

  strcpy(gpath, pathname);   // tokens are in global gpath[ ]
  n = 0;

  s = strtok(gpath, "/");
  while(s){
    name[n] = s;
    n++;
    s = strtok(0, "/");
  }
  name[n] = 0;
  
  for (i= 0; i<n; i++)
    printf("%s  ", name[i]);
  printf("\n");
  return 0;
}

// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino)
{
  int i;
  MINODE *mip;
  char buf[BLKSIZE];
  int blk, offset;
  INODE *ip;

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->ref_count && mip->dev == dev && mip->ino == ino){
       mip->ref_count++;
       //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }
    
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->ref_count == 0){
       //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->ref_count = 1;
       mip->dev = dev;
       mip->ino = ino;

       // get INODE of ino to buf    
       blk    = (ino-1)/8 + iblk;
       offset = (ino-1) % 8;

       //printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

       get_block(dev, blk, buf);
       ip = (INODE *)buf + offset;
       // copy INODE to mp->INODE
       mip->INODE = *ip; // memcopy
       return mip;
    }
  }   
  printf("PANIC: no more free minodes\n");
  return NULL;
}

int iput(MINODE *mip)
{
 int i, block, offset;
 char buf[BLKSIZE];
 INODE *ip;

 if (mip==0) 
     return -1;

 mip->ref_count--;

 if (mip->ref_count > 0)
 {
    return -1;
 }
 if (mip->dirty == 0)
 {
   return -1;
 }
 
 /* write INODE back to disk */
  block = ((mip->ino - 1) / 8) + inode_start;
  offset = (mip->ino - 1) % 8;

  // first get the block containing this inode
  get_block(mip->dev, block, buf);

  ip = (INODE *)buf + offset;
  *ip = mip->INODE;

  put_block(mip->dev, block, buf);
 /**************** NOTE ******************************
  For mountroot, we never MODIFY any loaded INODE
                 so no need to write it back
  FOR LATER WORK: MUST write INODE back to disk if refCount==0 && DIRTY

  Additional comments from Lecture:
  ********WRITE BACK***********
  printf("iput: dev=%d ino=%d\n", mip->dev, mip->ino);

  block = ((mip->ino - 1) / 8) + inode_start;
  offset = (mip->ino - 1) % 8;

  // first get the block containing this inode
  get_block(mip->dev, block, buf);

  ip = (INODE *)buf + offset;
  *ip = mip->INODE;

  put_block(mip->dev, block, buf);
   
  ******************************/
 return 0;
} 

int search(MINODE *mip, char *name)
{
   int i; 
   char *cp, c, sbuf[BLKSIZE], temp[256];
   DIR *dp;
   INODE *ip;

   printf("search for %s in MINODE = [%d, %d]\n", name,mip->dev,mip->ino);
   ip = &(mip->INODE);

   /*** search for name in mip's data blocks: ASSUME i_block[0] ONLY ***/

   for (int i = 0; i < ip->i_blocks; ++i)
   {
      get_block(dev, ip->i_block[i], sbuf);
      dp = (DIR *)sbuf;
      cp = sbuf;
      printf("  ino   rlen  nlen  name\n");

      while (cp < sbuf + BLKSIZE && dp->inode != 0){
         strncpy(temp, dp->name, dp->name_len);
         temp[dp->name_len] = 0;
         printf("%4d  %4d  %4d    %s\n", 
           dp->inode, dp->rec_len, dp->name_len, dp->name);
         if (strcmp(temp, name)==0){
            printf("found %s : ino = %d\n", temp, dp->inode);
            return dp->inode;
         }
         cp += dp->rec_len;
         dp = (DIR *)cp;
      }
   }
   return 0;
}

int getino(char *pathname)
{
  int i, ino, blk, offset;
  char buf[BLKSIZE];
  INODE *ip;
  MINODE *mip;

  printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/")==0)
      return 2;
  
  // starting mip = root OR CWD
  if (pathname[0]=='/')
     mip = root;
  else
     mip = running->cwd;

  mip->ref_count++;         // because we iput(mip) later
  
  tokenize(pathname);
  for (i=0; i<n; i++){
      printf("===========================================\n");
      printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);
 
      ino = search(mip, name[i]);

      if (ino==0){
         iput(mip);
         printf("name %s does not exist\n", name[i]);
         return -1;
      }
      iput(mip);
      mip = iget(dev, ino);
   }
   iput(mip);
   return ino;
}

// These 2 functions are needed for pwd()
// TODO:
int findmyname(MINODE *parent, u32 myino, char myname[ ]) 
{
  // WRITE YOUR code here
  // search parent's data block for myino; SAME as search() but by myino
  // copy its name STRING to myname[ ]
  // same methods as search, use inode to find name
  // i blocks treat files and dirs the same. We can utilize search
  // but instead of searching for the name, we compare inodes.
  char buf[BLKSIZE];
  MINODE* mip = parent;
  for (int i = 0; i < parent->INODE.i_blocks; ++i)
  {
     get_block(dev, parent->INODE.i_block[i], buf);
     DIR* dp = (DIR*)buf;
     char* cp = buf;
     while (cp < buf + BLKSIZE)
     {
        if (dp->inode == myino)
        {
           strncpy(myname, dp->name, dp->name_len);
           myname[dp->name_len] = '\0';
           return 0;
        }
        cp += dp->rec_len;
        dp = (DIR*)cp;
     }
  }
  return -1;
}

// TODO:
int findino(MINODE *mip, u32 *myino) // myino = i# of . return i# of ..
{
  // mip points at a DIR minode
  // WRITE your code here: myino = ino of .  return ino of ..
  // all in i_block[0] of this DIR INODE.

  // Additional code from lecture below:
  
  char buf[BLKSIZE], *cp;
  DIR *dp;

  get_block(mip->dev, mip->INODE.i_block[0], buf);
  cp = buf;
  dp = (DIR *)cp;
  *myino = dp->inode;
  cp += dp->rec_len;
  dp = (DIR *)cp;
  return dp->inode;
}


/*
   general pattern of using an inode is 

   ino = getino(pathname);
   mip = iget(dev, ino);
      use mip->INODE, which may modify the INODE;
   iput(mip);
*/