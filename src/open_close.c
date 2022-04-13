/************* open_close.c file **************/

#include <sys/stat.h>
#include <ext2fs/ext2fs.h>
#include <fcntl.h>
#include <libgen.h>
#include <time.h>
#include <string.h>
#include "commands.h"
#include "type.h"
#include "util.h"


/******************* Algorithm of open *********************/
/*
(1). get file's minode:
    ino = getino(filename);
    if (ino==0){ // if file does not exist
        creat(filename); // creat it first, then
        ino = getino(filename); // get its ino
    }
    mip = iget(dev, ino);
(2). allocate an openTable entry OFT; initialize OFT entries:
    mode = 0(RD) or 1(WR) or 2(RW) or 3(APPEND)
    minodePtr = mip; // point to file’s minode
    refCount = 1;
    set offset = 0 for RD|WR|RW; set to file size for APPEND mode;
(3). Search for the first FREE fd[index] entry with the lowest index in PROC;
    fd[index] = &OFT; // fd entry points to the openTable entry;
(4). return index as file descriptor;
*/
int myopen(char *pathname, int mode)
{
    // 1 - get the in-memory INODE of a file 
    int ino = getino(pathname);
    if(ino == -1) 
    {
        // FILE MUST BE CREATED IF IT DOES NOT EXIST
        mycreat(pathname);
        ino = getino(pathname);
        if (ino == -1) // check if parent directory does not exist/creat failed
        {
            printf("myopen: mycreat failed.\n");
            return -1;
        }
    }
    MINODE *mip = iget(dev, ino);

    // 2 - check if the file is a regular file
    if(!S_ISREG(mip->INODE.i_mode))
    {
        printf("myopen: pathname is not a regular file.\n");
        return -1;
    }

    // 3 - check if the file is already opened and if 
    for(int i = 0; i < NFD; i++)
    {
        if(running->fd[i] && running->fd[i]->minode_ptr == mip && running->fd[i]->mode != READ)
        {
            printf("myopen: file is already opened.\n");
            return -1;
        }
    }

    // 4 - allocate an openTable entry OFT; initialize OFT entries:
    OFT *oftp = (OFT *)malloc(sizeof(OFT));
    oftp->mode = mode;
    oftp->ref_count = 1;
    oftp->minode_ptr = mip;
    
    switch(mode)
    {
        case READ:
            oftp->offset = 0;
            break;
        case WRITE:
            inode_truncate(mip); // inode_truncate() needs to be changed to account for indirect and double indirect blocks
            oftp->offset = 0;
            break;
        case READ_WRITE:
            oftp->offset = 0;
            break;
        case APPEND:
            oftp->offset = mip->INODE.i_size;
            break;
        default:
            printf("myopen: mode is not valid.\n");
            return -1;
    }

    // 5 - search for the first FREE fd[index] entry with the lowest index in PROC;
    //     fd[index] = &OFT; // fd entry points to the openTable entry;
    int fd = 0;
    for(int i = 0; i < NFD; i++)
    {
        if(running->fd[i] == NULL)
        {
            fd = i;
            running->fd[fd] = oftp;
            break;
        }
    }

    // 6 - update INODE's time field for READ: touch atime, for Another one else touch atime and mtime
    mip->INODE.i_atime = time(0L);
    if (mode != READ)
    {
        mip->INODE.i_mtime = time(0L);
    }
    mip->dirty = 1;

    // 7 - return index as file descriptor
    return fd; 
}

int inode_truncate(MINODE *pmip)
{
    // THIS FUNCTION WILL HAVE TO HAVE CHANAGES TO IT TO COUNT FOR INDIRECT BLOCKS
    // IN LEVEL 2 AS THIS ONLY COUNTS FOR 12 DIRECT BLOCKS
    char buf[BLKSIZE];
    int i; 
    INODE *ip = &pmip->INODE;
    for (i = 0; i < 12; i++)
    {
        if (ip->i_block[i] == 0)
            continue;
        bdalloc(dev, ip->i_block[i]);
        ip->i_block[i] = 0;
    }

    // TODO: CHECK IF INDIRECT BLOCKS AND DOUBLY INDIRECT BLOCKS ARE USED
}

/*
  Algorithm of myclose:
  1. verify fd is within range.

  2. verify running->fd[fd] is pointing at a OFT entry

  3. The following code segments should be fairly obvious:
     oftp = running->fd[fd];
     running->fd[fd] = 0;
     oftp->refCount--;
     if (oftp->refCount > 0) return 0;

     // last user of this OFT entry ==> dispose of the Minode[]
     mip = oftp->inodeptr;
     iput(mip);

     return 0; 
*/

int myclose(int fd)
{
    // 1 - verify that fd is in range
    if(fd < 0 || fd > NFD)
    {
        printf("myclose: fd is out of range.\n");
        return -1;
    }

    // 2 - verify that running->fd[fd] is pointing at a OFT entry
    if(running->fd[fd] == NULL)
    {
        printf("myclose: fd is not open.\n");
        return -1;
    }

    // 3 - The following code segments should be fairly obvious:
    OFT *oftp = running->fd[fd];
    running->fd[fd] = 0;
    oftp->ref_count--;
    if(oftp->ref_count > 0)
    {
        return 0;
    }

    // 4 - last user of this OFT entry ==> dispose of the Minode[]
    MINODE *mip = oftp->minode_ptr;
    iput(mip);

    // 5 - deallocate the OFT entry (because we allocated it in myopen)
    free(oftp);

    return 0;
}

/*
    Algorithm of lseek:

    From fd, find the OFT entry. 

    change OFT entry's offset to position but make sure NOT to over run either end
    of the file.

    return originalPosition
*/
int mylseek(int fd, int position) 
{
    if (position < 0)
    {
        printf("mylseek: position is negative.\n");
        return -1;
    }

    // 1 - verify that fd is in range
    if(fd < 0 || fd > NFD)
    {
        printf("lseek: fd is out of range.\n");
        return -1;
    }

    // 2 - verify that running->fd[fd] is pointing at a OFT entry
    if(running->fd[fd] == NULL)
    {
        printf("lseek: fd is not open.\n");
        return -1;
    }

    // 3 - change OFT entry's offset to position but make sure NOT to over run either end of the file
    OFT *oftp = running->fd[fd];

    printf("name: %d\n", oftp->minode_ptr->INODE.i_dtime);
    // check to make sure the position does not over run the file
    printf("size: %d\n", oftp->minode_ptr->INODE.i_size);
    if(position > oftp->minode_ptr->INODE.i_size)
    {
        printf("lseek: position is out of range.\n");
        return -1;
    }

    // 4 - change and return originalPosition
    int originalPosition = oftp->offset;
    oftp->offset = position;
    printf("originalPosition: %d\n", originalPosition);
    return originalPosition;
}

/*
    Algorithm of pfd:

    This function displays the currently opened files as follows:

        fd     mode    offset    INODE
       ----    ----    ------   --------
         0     READ    1234   [dev, ino]  
         1     WRITE      0   [dev, ino]
      --------------------------------------

    to help the user know what files has been opened.
*/

int pfd()
{
    printf("fd\tmode\toffset\tINODE\n");
    printf("----\t----\t------\t--------\n");
    char *mode_name;
    for(int i = 0; i < NFD; i++)
    {
        if(running->fd[i] != NULL)
        {
            OFT *oftp = running->fd[i];
            switch(oftp->mode)
            {
                case READ:
                    mode_name = "READ";
                    break;
                case WRITE:
                    mode_name = "WRITE";
                    break;
                case READ_WRITE:
                    mode_name = "READ_WRITE";
                    break;
                case APPEND:
                    mode_name = "APPEND";
                    break;
                default:
                    mode_name = "UNKNOWN MODE";
                    break;
            }
            printf("%d\t%s\t%d\t[%d, %d]\n", i, mode_name, oftp->offset, 
            oftp->minode_ptr->dev, oftp->minode_ptr->ino);
        }
    }

    return 0;
}

/*
    algorithm of dup:

    1 - verify fd is an opened descriptor;
    2 - duplicates (copy) fd[fd] into FIRST empty fd[ ] slot
    3 - increment OFT's refcount by 1;
*/
int dup(int fd)
{
    // 1 - verify fd is an opened descriptor
    if(fd < 0 || fd > NFD)
    {
        printf("dup: fd is out of range.\n");
        return -1;
    }

    // 2 - checks if fd is open
    if(running->fd[fd] == NULL)
    {
        printf("dup: fd is not open.\n");
        return -1;
    }

    // 3 - duplicates (copy) fd[fd] into FIRST empty fd[ ] slot
    OFT *oftp = running->fd[fd];
    for (int i = 0; i < NFD; i++)
    {
        if(running->fd[i] == NULL)
        {
            running->fd[i] = oftp;
            oftp->ref_count++;
            return 0;
        }
    }

    return -1;
}

/*
    algorithm of dup2:

    1 - CLOSE gd first if it's already opened
    2 - duplicates fd[fd] into fd[gd]
*/
int dup2(int fd, int gd)
{
    if (fd < 0 && fd > NFD || gd < 0 && gd > NFD)
    {
        printf("dup2: fd or gd is out of range.\n");
        return -1;
    }

    // 1 - CLOSE gd first if it's already opened
    if(running->fd[gd] != NULL)
    {
        int flag = myclose(gd);
        if (flag < 0)
        {
            printf("dup2: close gd failed.\n");
            return -1;
        }
    }

    // 2 - duplicates fd[fd] into fd[gd]
    OFT *oftp = running->fd[fd];
    running->fd[gd] = oftp;
    oftp->ref_count++;

    return 0;
}