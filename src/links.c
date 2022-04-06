/************* links.c file **************/

#include <sys/stat.h>
#include <ext2fs/ext2fs.h>
#include <fcntl.h>
#include <libgen.h>
#include <time.h>
#include "commands.h"
#include "type.h"
#include "util.h"

int link(char *pathname) // or ln
{
    // split pathname with old_file and new_file
    char old_file[128], new_file[128];
    char *s = strtok(pathname, " ");
    strcpy(old_file, s);
    s = strtok(0, " ");
    strcpy(new_file, s);

    // 1 - verify old_file exists and is not a DIR
    int oino = getino(old_file);
    MINODE *omip = iget(dev, oino);
    if (S_ISDIR(omip->INODE.i_mode))
    {
        printf("link: cannot link a directory.\n");
        return -1;
    }

    // 2 - new_file must not exist yet:
    if (getino(new_file) != 0)
    {
        printf("link: new_file already exists.\n");
        return -1;
    }

    // 3 - creat new_file with the same inode number of old_file:
    
    // split new_file into dirname and basename
    char temp[128];
    strcpy(temp, new_file);
    char *parent = dirname(temp);
    strcpy(temp, new_file);
    char *child = basename(temp);

    int pino = getino(parent);
    MINODE *pmip = iget(dev, pino);
    // create entry in new parent DIR with same inode number of old_file
    enter_child(pmip, oino, child);

    // 4 - inc link count of old_file and write back by iput(omip)
    omip->INODE.i_links_count++;
    omip->dirty = 1;
    iput(omip);
    iput(pmip);

    return 0;
}

int unlink(char *pathname)
{
    // 1 - get filename's minode
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino);

    // check it's a REG or symbolic LNK file; can not be a DIR
    if (S_ISDIR(mip->INODE.i_mode))
    {
        printf("unlink: cannot unlink a directory.\n");
        return -1;
    }

    if (!S_ISLNK(mip->INODE.i_mode))
    {
        printf("unlink: cannot unlink a symbolic link.\n");
        return -1;
    }

    // 2 - remove name entry from paren DIR's data block:

    // get parent and child's name from pathname
    char temp[128];
    strcpy(temp, pathname);
    char *parent = dirname(temp);
    strcpy(temp, pathname);
    char *child = basename(temp);

    // remove name entry from parent DIR's data block
    int pino = getino(parent);
    MINODE *pmip = iget(dev, pino);
    rm_child(pmip, ino, child);
    pmip->dirty = 1;
    iput(pmip);

    // 3 - decrement INODE's link_count by 1
    mip->INODE.i_links_count--;

    // 4 - if INODE's link_count is 0, remove INODE from disk
    if (mip->INODE.i_links_count > 0)
    {
        mip->dirty = 1; // for write INODE back to disk
    }
    else
    {
        // deallocate all data blocks in INODE;
        // deallocate INODE;
        inode_truncate(pmip); // could be mip instead
    }

    iput(mip); // release mip

    return 0;
}

int inode_truncate(MINODE *pmip)
{
    // THIS FUNCTION WILL HAVE TO HAVE CHANAGES TO IT TO COUNT FOR INDIRECT BLOCKS
    // IN LEVEL 2 AS THIS ONLY COUNTS FOR 12 DIRECT BLOCKS
    int i;
    INODE *ip = &pmip->INODE;
    for (i = 0; i < 12; i++)
    {
        if (ip->i_block[i] == 0)
            continue;
        bdalloc(dev, ip->i_block[i]);
        ip->i_block[i] = 0;
    }
}

int symlink(char *pathname)
{
    /*

    Algorithm of symlink pathname (symlink old_file new_file)
    1 - divide pathname into old_file and new_file
    2 - check: old_file must exist and new_file must not exist yet;
    3 - creat new_file; change new_file to LNK (symbolic link) type;
    4 - // assume length of old_file name <= 60 chars
        store old_file name in new_file's INODE.i_block[ ] area.
        set file size to length of old_file name
        mark new_file's minode dirty;
        iput(new_file's minode);
    5 - mark new_file parent minode dirty;
    6 - iput(new_file's parent minode);

    */

    // 1 - split pathname with old_file and new_file
    char old_file[128], new_file[128];
    char *s = strtok(pathname, " ");
    strcpy(old_file, s);
    s = strtok(0, " ");
    strcpy(new_file, s);

    // WRITE CODE HERE
}

int readlink(char *file, char *buf)
{
    /* 
    Algorithm of realink (file, buffer)
    1 - get file's INODE in memory; verify it's a LNK file;
    2 - copy target filename from INODE.i_block[ ] into buffer;
    3 - return file size
    */


    // 1 - get file's INODE in memory; verify it's a LNK file;
    int ino = getino(file);
    MINODE *mip = iget(dev, ino);

    if (!S_ISLNK(mip->INODE.i_mode))
    {
        printf("readlink: file is not a symbolic link.\n");
        return -1;
    }

    // 2 - copy link's content to buf
    // do we need to load all?
    for (int i = 0; mip->INODE.i_blocks; ++i)
    {
        strcpy(buf, mip->INODE.i_block[0]);
    }

    // 3 - return file size
    return mip->INODE.i_size;

}
