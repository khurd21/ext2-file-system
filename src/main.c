/****************************************************************************
*                   KCW: mount root file system                             *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"
#include "util.h"
#include "commands.h"

char *disk = "images/diskimage";

int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;

  printf("init()\n");

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->ref_count = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  for (i=0; i<NOFT; ++i){
    oft[i].ref_count = 0;
    oft[i].offset = 0;
    oft[i].mode = 0;
    oft[i].minode_ptr = NULL;
  }
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->pid = i;
    p->uid = i;
    p->status = FREE;
    p->gid = 0;
    p->cwd = 0;
    for (j=0; j<NFD; j++)
      p->fd[j] = 0;
    p->next = &proc[i+1];
  }
  for (i=0; i<NMTABLE;++i){
    mtable[i].dev = mtable[i].ninodes = mtable[i].nblocks = 0;
    mtable[i].free_blocks = mtable[i].free_inodes = 0;
    mtable[i].bmap = mtable[i].imap = mtable[i].iblock = 0;
  }
  proc[NPROC-1].next = &proc[0];
  running = &proc[0];
  return 0;
}

// load root INODE and set root pointer to it
int mount_root()
{  
  printf("mount_root()\n");
  MTABLE* mp;
  SUPER * sp;
  GD    * gp;
  char buf[BLKSIZE];

  dev = open(disk, O_RDWR);
  if (dev < 0){
    printf("open %s failed\n", disk);
    return -1;
  }
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  if (sp->s_magic != 0xEF53){
    printf("%s is not an EXT2 FS\n", disk);
    exit(0);
  }
  mp = &mtable[0];
  mp->dev = dev;
  ninodes = mp->ninodes = sp->s_inodes_count;
  nblocks = mp->nblocks = sp->s_blocks_count;
  strcpy(mp->dev_name, disk);
  strcpy(mp->mnt_name, "/");
  get_block(dev, 2, buf);
  gp = (GD *)buf;
  bmap = mp->bmap = gp->bg_block_bitmap;
  imap = mp->imap = gp->bg_inode_bitmap;
  iblk = mp->iblock = gp->bg_inode_table;
  printf("ninodes = %d, nblocks = %d\n", ninodes, nblocks);
  printf("bmap = %d, imap = %d, iblk = %d\n", bmap, imap, iblk);
  root = iget(dev, 2);
  mp->mnt_dir_ptr = root;
  root->mptr = mp;
  for (int i=0; i < NPROC; ++i){
    proc[i].cwd = iget(dev, 2);
  }
  printf("Mount: %s mounted on %s\n", disk, "/");
  return 0;
}

int menu() // can get rid of this if you want
{
  printf("All Commands:\n");
  printf("[ls|cd|pwd|mkdir|rmdir|creat|link|unlink|symlink|readlink|stat|chmod|utime|quit]\n\n");
}

int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->ref_count > 0)
      iput(mip);
  }
  exit(0);
}

int main(int argc, char *argv[ ])
{
  int ino;
  char buf[BLKSIZE];
  int fd;

  printf("checking EXT2 FS ....");
  init();  
  mount_root();
  printf("EXT2 FS OK\n");

  // WRTIE code here to create P1 as a USER process
  char line[128], cmd[16], pathname[64], pathname2[64]; // K.C What are you doing?

  while(1){
    printf("input command [enter \"menu\" to view all commands]: "); 
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    if (line[0]==0)
       continue;
    pathname[0] = 0;
    pathname2[0] = 0;

    sscanf(line, "%s %s %s", cmd, pathname, pathname2); // PATHNAME2 is for link, symlink, and chmod
    printf("cmd=%s, pathname=%s, pathname2=%s\n", cmd, pathname, pathname2);
  
    if (strcmp(cmd, "ls")==0)
      ls(pathname);
    else if (strcmp(cmd, "cd")==0)
      cd(pathname);
    else if (strcmp(cmd, "pwd")==0)
      pwd(running->cwd);
    else if (strcmp(cmd, "quit")==0)
      quit();
    else if (strcmp(cmd, "mkdir") == 0)
      mymkdir(pathname);
    else if (strcmp(cmd, "rmdir") == 0)
      rmdir(pathname);
    else if (strcmp(cmd, "creat") == 0)
      mycreat(pathname);
    else if (strcmp(cmd, "link") == 0)
      link(pathname, pathname2);
    else if (strcmp(cmd, "unlink") == 0)
      unlink(pathname);
    else if (strcmp(cmd, "symlink") == 0)
      symlink(pathname, pathname2);
    else if (strcmp(cmd, "readlink") == 0)
      readlink(pathname, buf); // ASSUMED THE BUF IS DEFINED IN MAIN FUNCTION AS IT IS ABOVE
    else if (strcmp(cmd, "stat") == 0)
      mystat(pathname);
    else if (strcmp(cmd, "chmod") == 0)
      mychmod(atoi(pathname), pathname2);
    else if (strcmp(cmd, "utime") == 0)
      utime(pathname);
    else if (strcmp(cmd, "menu") == 0)
      menu();
  }
  return 0;
}
