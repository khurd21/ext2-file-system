/************* cd_ls_pwd.c file **************/

#include <sys/stat.h>
#include <ext2fs/ext2fs.h>
#include <fcntl.h>
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
 
  printf("ls_file: to be done: READ textbook!!!!\n");
  // READ Chapter 11.7.3 HOW TO ls
  // utilize the INODE struct information to display all information from the file
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

  char buf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;

  get_block(dev, mip->INODE.i_block[0], buf);
  dp = (DIR *)buf;
  cp = buf;

  // if there is a file, need to call ls_file()  ?
  while (cp < buf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len);
     temp[dp->name_len] = 0;

    MINODE* child_mip = iget(dev, dp->inode);
    // TODO: STOPPED HERE 
     printf("%s  ", temp);

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

  int ino = getino(pathname);
  MINODE *mip = iget(dev, ino);

  mode_t mode = mip->INODE.i_mode; 

  if (S_ISDIR(mode))
    ls_dir(mip);
  else
    ls_file(mip, pathname);

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
