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
MTABLE mtable[NMTABLE];
OFT    oft[NOFT];
PROC   proc[NPROC], *running;

int  nname;         // number of component strings
int nblocks, ninodes, bmap, imap, iblk;
char gline[25], *name[16];
int dev;

int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, SEEK_SET);
   int n = read(dev, buf, BLKSIZE);
   if (n < 0)
   {
      printf("get_block: read error\n");
      return -1;
   }
   return 0;
}   

int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, SEEK_SET);
   int n = write(dev, buf, BLKSIZE);
   if (n != BLKSIZE)
   {
      printf("put_block: write error\n");
      return -1;
   }
   return 0;
}   

int tokenize(char *pathname)
{
  int i;
  char *s;
  printf("tokenize %s\n", pathname);

  strcpy(gline, pathname);   // tokens are in global gpath[ ]
  nname = 0;

  s = strtok(gline, "/");
  while(s){
    name[nname] = s;
    nname++;
    s = strtok(0, "/");
  }
    printf("Tokenized pathname: \n");
  for (i= 0; i<nname; i++)
    printf("%s  ", name[i]);
  printf("\n");
  return 0;
}

MINODE* mialloc()
{
  for (int i=0; i<NMINODE; i++)
  {
     MINODE* mp = &minode[i];
    if (mp->ref_count == 0)
    {
      mp->ref_count = 1;
      return mp;
    }
  }
  printf("mialloc: no more free minodes\n");
  return 0;
}

int midealloc(MINODE *mip)
{
  mip->ref_count = 0;
  return 0;
}

// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino)
{
  MINODE *mip;
  MTABLE *mp;
  char buf[BLKSIZE];
  int blk, offset;
  INODE *ip;

  for (int i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->ref_count && mip->dev == dev && mip->ino == ino){
       mip->ref_count++;
       //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }
   mip = mialloc();    
   mip->dev = dev;
   mip->ino = ino;
   blk = (ino - 1) / 8 + iblk;
   offset = (ino - 1) % 8;
   get_block(dev, blk, buf);
   ip = (INODE *)buf + offset;
   mip->INODE = *ip;
   mip->dirty = 0;
   mip->mounted = 0;
   mip->ref_count = 1;
   return mip;
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

   printf("imap: %d\n", imap);
  block = ((mip->ino - 1) / 8) + iblk;
  offset = (mip->ino - 1) % 8;

  // first get the block containing this inode
  get_block(mip->dev, block, buf);

  ip = (INODE *)buf + offset;
  *ip = mip->INODE;

  put_block(mip->dev, block, buf);
  midealloc(mip);
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
      if (ip->i_block[i] == 0)
      {
         break;
      }

      get_block(mip->dev, ip->i_block[i], sbuf);
      dp = (DIR *)sbuf;
      cp = sbuf;
      printf("  ino   rlen  nlen  name\n");
      // We had a second condition in while loop: && dp->inode != 0
      while (cp < sbuf + BLKSIZE)
      {
         strncpy(temp, dp->name, dp->name_len);
         temp[dp->name_len] = '\0';
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
  int i, ino;
  MINODE *mip;

  printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/")==0)
  {
      return 2;
  }

  // TODO: ask if dev could be checked here
  // starting mip = root OR CWD
  if (pathname[0]=='/')
  {
     mip = root;
  }
  else
  {
     mip = running->cwd;
  }

  mip->ref_count++;         // because we iput(mip) later
  
  tokenize(pathname);
  for (i=0; i<nname; i++) {
     if (S_ISDIR(mip->INODE.i_mode == 0))
     {
        printf("%s is not a directory.\n", name[i]);
        iput(mip);
        return 0;
     }

      printf("===========================================\n");
      printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);
 
      ino = search(mip, name[i]);
      if (ino==0){
         printf("name %s does not exist\n", name[i]);
         iput(mip);
         return -1;
      }
      iput(mip);
      mip = iget(dev, ino);
   }

   // TODO: add code here to account for upward and downward traversal

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