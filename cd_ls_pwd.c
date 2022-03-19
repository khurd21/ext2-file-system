/************* cd_ls_pwd.c file **************/
int cd()
{
  printf("cd: under construction READ textbook!!!!\n");

  // READ Chapter 11.7.3 HOW TO chdir
  
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
  
  while (cp < buf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len);
     temp[dp->name_len] = 0;
	
     printf("%s  ", temp);

     cp += dp->rec_len;
     dp = (DIR *)cp;
  }
  printf("\n");
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
}

char *pwd(MINODE *wd) 
{
  printf("pwd: READ HOW TO pwd in textbook!!!!\n");
  if (wd == root){
    printf("/\n");
    return;
  }
}



