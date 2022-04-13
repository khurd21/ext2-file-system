/************* read_write.c file **************/

#include <sys/stat.h>
#include <ext2fs/ext2fs.h>
#include <fcntl.h>
#include <libgen.h>
#include <time.h>
#include <string.h>
#include "commands.h"
#include "type.h"
#include "util.h"

int myread(int fd, char* buf, int nbytes)
{
    char readbuf[BLKSIZE];
    int count = 0, blk = 0, remain = 0;
    // offset is set to byte set in file to read
    OFT *oftp = running->fd[fd];
    int offset = oftp->offset; 

    // compute bytes available in file: avil_bytes = fileSize - offset
    int avil_bytes = oftp->minode_ptr->INODE.i_size - offset;

    char *cq = buf;

    while(nbytes && avil_bytes)
    {
        int lbk = oftp->offset / BLKSIZE;
        int startByte = oftp->offset % BLKSIZE;

        if (lbk < 12)
        {
            blk = oftp->minode_ptr->INODE.i_block[lbk];
        }
        else if (lbk >= 12 && lbk < 256 + 12)
        {
            // Indirect Blocks
        }
        else
        {
            // Double Indirect Blocks
        }

        // get the data block into readbuf[BLKSIZE]
        get_block(oftp->minode_ptr->dev, blk, readbuf);

        // copy from startByte to buf[ ], at most remain bytes in this block
        char *cp = readbuf + startByte;
        remain = BLKSIZE - startByte;

        while(remain > 0)
        {
            *cq++ = *cp++;
            oftp->offset++;
            count++;
            avil_bytes--; nbytes--; remain--;
            if (nbytes <= 0 || avil_bytes <= 0)
                break; 
        }

        // if one data block is not enough, loop back to OUTER while for more ...
    }

    printf("myread: read %d char from file descriptor %d\n", count, fd);
    return count;
}

int mywrite(int fd, char buf[], int nbytes)
{
    char wbuf[BLKSIZE];
    char *cq = buf;
    int blk = 0, count = 0, remain = 0;
    OFT *oftp = running->fd[fd];
    MINODE *mip = running->fd[fd]->minode_ptr;
    while (nbytes > 0)
    {
        int lbk = oftp->offset / BLKSIZE;
        int startByte = oftp->offset % BLKSIZE;

        if (lbk < 12)
        {
            if(mip->INODE.i_block[lbk] == 0)
            {
                // allocate a new data block
                mip->INODE.i_block[lbk] = balloc(mip->dev);
            }
            int blk = mip->INODE.i_block[lbk];
        }
        else if (lbk >= 12 && lbk < 256 + 12)
        {
            // Indirect Blocks
            if(mip->INODE.i_block[12] == 0)
            {
                // allocate a new block for it
                // zero out the block on the disk
            }
            // get i_block[12] into an int ibuf[256]
            char ibuf[BLKSIZE];
            get_block(mip->dev, mip->INODE.i_block[12], ibuf);
            blk = ibuf[lbk - 12];
            if (blk == 0)
            {
                // allocate a disk block
                // record it in i_block[12]
            }
            // ....... more to do after???

        }
        else
        {
            // Double Indirect Blocks
        }
        // for all cases come here to write to the data block

        get_block(mip->dev, blk, wbuf);
        // copy from startByte to buf[ ], at most remain bytes in this block
        char *cp = wbuf + startByte;
        remain = BLKSIZE - startByte;

        while(remain > 0)
        {
            *cp++ = *cq++;
            oftp->offset++;
            count++;
            oftp->offset++;   
        }
        put_block(mip->dev, blk, wbuf);
        // loop back to outer while to write more ... until nbytes are written.
    }

    mip->dirty = 1;
    printf("wrote %d char into file descriptor %d\n", nbytes, fd);
    return nbytes;
}

int map(MINODE *mip, int lbk)
{
    int ibuf[256];
    int blk = 0;
    if (lbk < 12)
    {
        blk = mip->INODE.i_block[lbk];
    }
    else if (blk >= 12 && blk < 256 + 12)
    {
        // read INODE.i_block[12] into ibuf[256]
        get_block(mip->dev, ip->i_block[12], ibuf);
        blk = ibuf[lbk - 12];
    }
    else
    {
        // TODO: Double Indirect Blocks
        // this algorithm needs to be completed by using mailman's algorithm
        // to convert double indirect logical blocks to physical blocks

    }
    return blk;
}