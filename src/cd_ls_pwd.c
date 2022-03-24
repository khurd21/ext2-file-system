/************* cd_ls_pwd.c file **************/

#include <sys/stat.h>
#include <ext2fs/ext2fs.h>
#include <fcntl.h>
#include <libgen.h>
#include "type.h"
#include "util.h"
#include "cd_ls_pwd.h"

int cd(char* pathname)
{
  // READ Chapter 11.7.3 HOW TO chdir
  const int ino = getino(pathname);
  if (ino < 0)
  {
    printf("inode not found.\n");
    return -1;
  }

  MINODE* mip = iget(dev, ino);
  printf("MINODE i_mode: %d\n", mip->INODE.i_mode);
  printf("MINODE i_block:%u\n", mip->INODE.i_blocks);
  if (!S_ISDIR(mip->INODE.i_mode))
  {
    printf("cd: %s is not a directory.\n", pathname);
    return -1;
  }
  iput(running->cwd);
  running->cwd = mip;
  return 0;
}

int ls_file(MINODE *mip, char *name)
{
    /*
    Additional Comments from Lecture:
    1- INODE *ip = &mip->INODE;
    2- use ip->i_mode, ip->i_size, ip->ctime, ... (to gather information about all files)
    3- to do ls -l on this INODE: show [dev, ino] after the name field
  */
 
  // Utilized Chapter 8.6.7 The ls Program

  char ftime[64];
  char *t1 = "xwrxwrxwr-------";
  char *t2 = "----------------";

  // step 1: get the INODE information
  INODE *ip = &mip->INODE;
  if (S_ISREG(ip->i_mode))
    printf("-");
  if (S_ISDIR(ip->i_mode))
    printf("d");
  if (S_ISLNK(ip->i_mode))
    printf("l");

  for (int i = 8; i >= 0; i--)
  {
    if (ip->i_mode & (1 << i)) // print r|w|x
      printf("%c", t1[i]);
    else
      printf("%c", t2[i]);
  }
 
  printf("%4d ", ip->i_links_count);
  printf("%4d ", ip->i_gid);
  printf("%4d ", ip->i_uid);
  printf("%4d ", ip->i_size);

  // print time: incompatible pointer type, debugger throws a
  // [-Wincompatible-pointer-type]
  // Ur seg fault here. You had i_ctime instead of mtime :)
  strcpy(ftime, ctime((time_t*)&ip->i_mtime));
  ftime[strlen(ftime) - 1] = '\0'; // kill \n at end
  printf("%s ", ftime);

  // print name
  printf("%s", name);

  // print -> linkname if symbolic file 
  // NOT SURE ABOUT THIS PART
  // I got u bro. zip zip
  if (S_ISLNK(ip->i_mode))  
  {
    printf(" -> %s", (char*)ip->i_block);
  }
  printf("\n");
  return 0;
}

int ls_dir(MINODE *mip)
{
  /*
    Additional Comments from Lecture:
    1- get data block (i_block[0]) of mip->INODE into buf[]
    2- step through DIR entries: [ino rec_len name_len name]
    3- temp[] = name string
    4- for each entry do: mip = iget(dev, ino)
    5- list_file(mip, temp)
    6- iput(mip)
  */


  // will utilize same concept as search function from lab5
  printf("ls_dir: list CWD's file names; YOU FINISH IT as ls -l\n");

  char buf[BLKSIZE];
  char temp[256];
  char *cp;
  DIR *dp;

  get_block(dev, mip->INODE.i_block[0], buf);
  dp = (DIR *)buf;
  cp = buf;

  // if there is a file, need to call ls_file()  ?
  while (cp < buf + BLKSIZE){
    MINODE* child_mip = iget(dev, dp->inode);
    // ADDED THESE TWO LINES
    // YOUR BUG IS HERE BRO :(
    ls_file(child_mip, dp->name);
    iput(child_mip);

    cp += dp->rec_len;
    dp = (DIR *)cp;
  }
  printf("\n");
  return 0;
}

int ls(char *pathname) // will use the inodes without the use of stat to obtain all information
{
  /*
  printf("ls: list CWD only! YOU FINISH IT for ls pathname\n");
  ls_dir(running->cwd);
  */

  // Added additional code below from lecture

  // check if pathname is empty
  if (strcmp(pathname, "") == 0)
  {
    ls_dir(running->cwd);
    return 0;
  }

  int ino = getino(pathname);
  if (ino < 0)
  {
    return 0;
  }
  dev = root->dev;
  MINODE *mip = iget(dev, ino);
  mode_t mode = mip->INODE.i_mode; 

  if (S_ISDIR(mode))
    ls_dir(mip);
  else
    ls_file(mip, basename(pathname));

  iput(mip);
  return 0;
}

int pwd(MINODE *wd) 
{
  if (wd == root)
  {
    printf("/\n");
    return 0;
  }
  rpwd(wd);
  printf("\n");
  return 0;
}

int rpwd(MINODE* wd)
{
  if (wd == root)
  {
    return 0;
  }

  char my_name[128];
  const int my_ino = wd->ino;
  const int parent_ino = findino(wd, &my_ino);

  MINODE* pip = iget(dev, parent_ino);

  findmyname(pip, my_ino, my_name);

  rpwd(pip);
  iput(pip);
  printf("/%s", my_name);
  return 0;
}
