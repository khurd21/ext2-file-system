/************* stat_chmod_utime.c file **************/

#include <sys/stat.h>
#include <ext2fs/ext2fs.h>
#include <fcntl.h>
#include <libgen.h>
#include <time.h>
#include "commands.h"
#include "type.h"
#include "util.h"

/*

All of the following functions follow the same pattern:

1 - get the in-memory INODE of a file by
    ino = getino(pathname);
    mip = iget(dev, ino);

2 - get information from INODE or modify the INODE;

3 - if INODE is modified, set mip->dirty to nonzero for writing back to disk

4 - iput(mip);

*/

int mystat(char* pathname)
{
    // 1 - get the in-memory INODE of a file by
    int ino = getino(pathname);
    if(ino == -1)
    {
        printf("mystat: pathname does not exist.\n");
        return -1;
    }

    char * filename = basename(pathname);
    MINODE *mip = iget(dev, ino);

    // 2 - get information from INODE or modify the INODE; 
    // YOU CAN MODIFY THE INFORMATION AND PRINTING FORMAT IF YOU WANT
    char myatime[64], mymtime[64], myctime[64];
    time_t t_atime = mip->INODE.i_atime;
    time_t t_mtime = mip->INODE.i_mtime;
    time_t t_ctime = mip->INODE.i_ctime;
    
    strcpy(myatime, ctime(&t_atime));
    strcpy(mymtime, ctime(&t_mtime));
    strcpy(myctime, ctime(&t_ctime));

    myatime[strlen(myatime) - 1] = '\0';
    mymtime[strlen(mymtime) - 1] = '\0';
    myctime[strlen(myctime) - 1] = '\0';

    printf("MYSTAT INFOMATION:\n");
    printf("FILE:   %s\n", filename);
    printf("INODE:  %d\n", ino);
    printf("MODE:   %o/0x%x\n", mip->INODE.i_mode, mip->INODE.i_mode);
    printf("LINKS:  %d\n", mip->INODE.i_links_count);
    printf("UID:    %d\n", mip->INODE.i_uid);
    printf("GID:    %d\n", mip->INODE.i_gid);
    printf("SIZE:   %d\n", mip->INODE.i_size);
    printf("BLOCKS: %d\n", mip->INODE.i_blocks);
    printf("MTIME:  %s\n", mymtime);
    printf("CTIME:  %s\n", myctime);
    printf("ATIME:  %s\n", myatime);

    // 4 - iput(mip);
    iput(mip);

    return 0;
}

int mychmod(int mode, char *pathname)
{
    // 1 - get the in-memory INODE of a file by
    int ino = getino(pathname);
    if(ino == -1)
    {
        printf("mychmod: pathname does not exist.\n");
        return -1;
    }
    MINODE *mip = iget(dev, ino);

    // 2 - get information from INODE or modify the INODE;
    INODE *ip = &mip->INODE;
    ip->i_mode = mode; // CHECK MODE HERE

    // 3 - if INODE is modified, set mip->dirty to nonzero for writing back to disk
    mip->dirty = 1;

    // 4 - iput(mip);
    iput(mip);

    return 0;
}

int utime(char* pathname)
{
    // 1 - get the in-memory INODE of a file by
    int ino = getino(pathname);
    if(ino == -1)
    {
        printf("utime: pathname does not exist.\n");
        return -1;
    }
    MINODE *mip = iget(dev, ino);

    // 2 - get information from INODE or modify the INODE;
    INODE *ip = &mip->INODE;
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L); // set to current time

    // 3 - if INODE is modified, set mip->dirty to nonzero for writing back to disk
    mip->dirty = 1;

    // 4 - iput(mip);
    iput(mip);

    return 0;
}