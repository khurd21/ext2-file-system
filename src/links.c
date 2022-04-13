/************* links.c file **************/

#include <sys/stat.h>
#include <ext2fs/ext2fs.h>
#include <fcntl.h>
#include <libgen.h>
#include <time.h>
#include "commands.h"
#include "type.h"
#include "util.h"

int link(char *pathname, char* pathname2) // or ln
{
    // intialize old_file to pathname and new_file to pathname2
    char *old_file = pathname;
    char *new_file = pathname2;

    printf("old_file: %s and new_file: %s\n", old_file, new_file);
    // 1 - verify old_file exists and is not a DIR
    int oino = getino(old_file);
    MINODE *omip = iget(dev, oino);
    if (S_ISDIR(omip->INODE.i_mode))
    {
        printf("link: cannot link a directory.\n");
        return -1;
    }

    // 2 - new_file must not exist yet:
    if (getino(new_file) != -1)
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
    // 0 - like others, we need to check if path is `/` or `./`
    if (pathname[0] == '/')
    {
        dev = root->dev;
    }
    else
    {
        dev = running->cwd->dev;
    }

    // 1 - get filename's minode
    int ino = getino(pathname);
    if (ino < 0)
    {
        printf("unlink: file does not exist.\n");
        return -1;
    }
    MINODE *mip = iget(dev, ino);

    // check it's a REG or symbolic LNK file; can not be a DIR
    if (S_ISDIR(mip->INODE.i_mode))
    {
        printf("unlink: cannot unlink a directory.\n");
        return -1;
    }
    /*
    Do we want to not unlink hard links?
    Condition here prevents this.
    if (!S_ISLNK(mip->INODE.i_mode))
    {
        printf("unlink: cannot unlink a symbolic link.\n");
        return -1;
    }
    */

    // 2 - remove name entry from paren DIR's data block:

    // get parent and child's name from pathname
    // same issue for others. Can we verify temp is in a different address?
    // We are probably accessing same memory here.
    char child[128];
    char parent[128];
    strcpy(parent, dirname(pathname));
    strcpy(child, basename(pathname));

    // 3 - decrement INODE's link_count by 1
    mip->INODE.i_links_count--;

    // 4 - if INODE's link_count is 0, then delete INODE from disk
    if (mip->INODE.i_links_count == 0 && S_ISLNK(mip->INODE.i_mode) == 0)
    {
        // if link count is 0, and it's a symbolic link,
        // we need to free the data block of the symbolic link
        // and then free the INODE
        // we can use truncate() to do this
        inode_truncate(mip);
    }
    mip->dirty = 1;
    iput(mip);

    int pino = getino(parent);
    if (pino < 0)
    {
        printf("unlink: parent does not exist.\n");
        return -1;
    }
    MINODE *pmip = iget(dev, pino);
    rm_child(pmip, child);
    return 0;
}

int symlink(char *pathname, char *pathname2)
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
    char *old_file = pathname;
    char *new_file = pathname2;

    printf("old_file: %s and new_file: %s\n", old_file, new_file);

    // 2 - old_file must exist and new_file must not exist yet:
    dev = old_file[0] == '/' ? root->dev : running->cwd->dev;
    int oino = getino(old_file);
    if (oino == -1)
    {
        printf("symlink: old_file does not exist.\n");
        return -1;
    }

    dev = new_file[0] == '/' ? root->dev : running->cwd->dev;
    // 3 - creat new_file; change new_file to LNK (symbolic link) type;
    int res = mycreat(new_file);
    if (res == -1)
    {
        printf("symlink: mycreat failed.\n");
        return -1;
    }

    int nino = getino(new_file);
    if (nino == -1)
    {
        printf("symlink: new_file already exists.\n");
        return -1;
    }

    MINODE *mip = iget(dev, nino);
    mip->INODE.i_mode = 0120777;
    // store old_file name in new_file's INODE.i_block[ ] area.
    strncpy(mip->INODE.i_block, old_file, 60);
    mip->INODE.i_size = strlen(old_file) + 1;
    mip->dirty = 1;
    iput(mip);
    return 0;
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

    printf("mip->INODE.i_mode: %d\n", mip->INODE.i_mode);
    if (!S_ISLNK(mip->INODE.i_mode))
    {
        printf("readlink: file is not a symbolic link.\n");
        return -1;
    }

    // 2 - copy link's content to buf
    // do we need to load all?
    printf("before strcpy\n");

    strcpy(buf, (char *)mip->INODE.i_block);

    printf("readlink buf content: %s\n", buf);
    printf("readlink: file size: %d\n", mip->INODE.i_size);
    // 3 - return file size
    return mip->INODE.i_size;
}
