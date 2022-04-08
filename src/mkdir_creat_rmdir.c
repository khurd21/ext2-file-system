/************* mkdir_creat_rmdir.c file **************/

#include <sys/stat.h>
#include <ext2fs/ext2fs.h>
#include <fcntl.h>
#include <libgen.h>
#include <time.h>
#include <string.h>
#include "commands.h"
#include "type.h"
#include "util.h"

// tst_bit, set_bit functions
int tst_bit(char *buf, int bit)
{
    return buf[bit/8] & (1 << (bit % 8));
}

int set_bit(char *buf, int bit)
{
    buf[bit/8] |= (1 << (bit % 8));
}

int decFreeInodes(int dev)
{
    char buf[BLKSIZE]; // BUFFER MIGHT BE LOCAL NOT DEFINED IN BOOK
    // dev free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count--;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count--;
    put_block(dev, 2, buf);
}

int ialloc(int dev)
{
    int i;
    char buf[BLKSIZE];
    // use imap, ninodes in mount table of dev
    // MTABLE *mp = (MTABLE *)get_mtable(dev);
    // mp->imap;
    get_block(dev, imap, buf);
    for (i = 0; i < ninodes; i++)
    {
        if (tst_bit(buf, i) == 0)
        {
            set_bit(buf, i);
            put_block(dev, imap, buf);
            // update free inode count in SUPER and GD
            decFreeInodes(dev);
            return (i + 1);
        }
    }
    return 0; // out of FREE inodes
}

int balloc(int dev)
{
    // allocates a free disk block (number) from a device
    int i;
    char buf[(int)BLKSIZE];
    // MTABLE *mp = (MTABLE *)get_mtable(dev);
    // mp->bmap;
    get_block(dev, bmap, buf);
    for (i = 0; i < nblocks; i++)
    {
        if (tst_bit(buf, i) == 0) // order might differ as deallocation must happen
        {
            set_bit(buf, i);
            put_block(dev, bmap, buf);
            decFreeInodes(dev);
            return (i + 1);
        }
    }
    return 0; // out of FREE inodes
}

int mymkdir(char *pathname)
{
    // TODO: 0 - absolute vs relative ???
    dev = pathname[0] == '/' ? root->dev : running->cwd->dev;

    // 1 - divide the pathname into dirname and basename
    char string1[128];
    char string2[128];

    // Probably doesnt do what we think it will do
    strcpy(string1, pathname);
    char* dname = dirname(string1);
    strcpy(string2, pathname);
    char *bname = basename(string2);

    // 2 - dirname must exist and is a DIR
    int pino = getino(dname);
    if (pino == -1)
    {
        printf("No parent directory.\n");
        return -1;
    }

    // check if bname is "/"
    if (strcmp(bname, "/") == 0)
    {
        printf("Cannot create directory with name \"/\".\n");
        return -1;
    }

    MINODE *pmip = iget(dev, pino);
    if (S_ISDIR(pmip->INODE.i_mode) == 0)
    {
        printf("%s is not a directory\n", dname);
        return -1;
    }

    // 3 - basename must not exist in parent DIR, must return 0 
    if (search(pmip, bname) != 0)
    {
        printf("%s already exists\n", bname);
        return -1;
    }

    // 4 - call kmkdir(pmip, basename) to create a DIR;
    kmkdir(pmip, bname);
    ++pmip->INODE.i_links_count;
    pmip->INODE.i_atime = time(0L);
    pmip->dirty = 1; 
    iput(pmip);
    return 0;
}

int kmkdir(MINODE* pmip, char* bname)
{
    // 1 - allocate an INODE
    int ino = ialloc(dev);
    int blk = balloc(dev);

    // 2 - create/setup INODE
    MINODE *mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    ip->i_mode = 0x41ED;                                // 040755: DIR type and permissions
    ip->i_uid = running->uid;                           // owner uid
    ip->i_gid = running->gid;                           // group Id
    ip->i_size = BLKSIZE;                               // size in bytes
    ip->i_links_count = 2;                              // links count=2 because of . and .. 
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L); // set to current time
    ip->i_blocks = 2;                                   // LINUX: Blocks count in 512-byte chunks
    ip->i_block[0] = blk;                               // new DIR has one data block IN BOOK 'blk' is called 'bno'
    for (int i = 1; i < 15; i++)                        // ip->i_block[1] = 0 to ip->i_block[0] = 0;
        ip->i_block[i] = 0;
    mip->dirty = 1;                                     // mark minode dirty
    iput(mip);                                          // write INODE to disk 
    
    
    // 3 - Create data block for new DIR containing . and .. entries  
    char buf[BLKSIZE];
    bzero(buf, BLKSIZE);
    get_block(dev, blk, buf); // This was not called b4
    DIR *mydp = (DIR *)buf;
    char *cp = buf;

    // make . entry
    mydp->inode = ino;
    mydp->rec_len = 12;
    mydp->name_len = 1;
    strcpy(mydp->name, ".");
    cp += mydp->rec_len;
    mydp = (DIR*)cp;

    // make .. entry: pino=parent DIR ino, blk=allocated block
    // dp = (char *)dp + 12;
    mydp->inode = pmip->ino;       // IN BOOK dp->inode = pino;
    mydp->rec_len = BLKSIZE - 12;  // rec_len spans block
    mydp->name_len = 2;
    strcpy(mydp->name, "..");
    put_block(dev, blk, buf);

    // enter the child... yikes
    enter_child(pmip, ino, bname);

    return 0;
}

int enter_child(MINODE *pip, int ino, char *name) // kmkdir and creat will both use
{
    /*

    Algorithm of enter_name:

    for each data block of parent DIR do // assume: only 12 direct blocks
    {
        if (i_block[i]==0) BREAK; // to step (5) below
        1. Get parent's data block into a buf[ ];
        2. In a data block of the parent directory, each dir_entry has an 'ideal length'

        ideal_length = 4* [(8+ name_len + 3)/4]; // a multiple of 4

        All dir_entries rec_len = ideal_length, exccept the last entry. The
        rec_len of the LAST entry is to the end of the block, which may be
        larger then its ideal_length.

        3. In order to enter a new entry of name with n_len, the needed length is

        need_length = 4* [(8+ n_len + 3)/4]; // a multiple of 4

        4. Step to the last entry in the data block:

        get_block(parent->dev, parent->INODE.i_block[i], buf);
        dp = (DIR *)buf;
        cp = buf;
        while (cp + dp->rec_len < buf + BLKSIZE)
        {
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }

        dp NODE points at last entry in block

        remain = LAST entry's rec_len - ideal_length;

        if (remain >= need_length)
        {
            enter the new entry as the LAST entry and
            trim the previous entry rec_len to ideal_length
        }
        goto step (6);
    }
    refer to diagram on page 335 for more information
    
    5.  If no space in existing data block(s)
        Allocate a new data block; increment parent size by BLKSIZE
        Enter new entry as the first entry in the new data block with rec_len = BLKSIZE

    Refer to the diagram on page 336 for more information

    6.  Write data block to disk; 

    */
    char buf[BLKSIZE];

    // set the needed lengh for the new entry name
    int need_length = 4 * ((8 + strlen(name) + 3) / 4); // a multiple of 4
    INODE* ip = &pip->INODE;

    for (int i = 0; i < 12; i++)
    {

        // if the i_block[i] is 0, break
        if (ip->i_block[i] == 0)
            break;
        // get the data block into buf[ ]
        int blk = ip->i_block[i];
        get_block(pip->dev, ip->i_block[i], buf);

        // dp points to the last entry in the data block as DIR
        DIR *mydp = (DIR *)buf;

        // cp points to the last entry in the data block as char
        char* cp = buf;

        // traverse the data block to find the last entry
        printf("%d\n", mydp->rec_len);
        while (cp + mydp->rec_len < buf + BLKSIZE)
        {
            cp += mydp->rec_len;
            mydp = (DIR *)cp;
        }

        // remain is the remaining space in the last entry
        int ideal_length = 4 * ((8 + mydp->name_len + 3) / 4);
        int remain = mydp->rec_len - ideal_length;

        // if the remaining space is enough to enter the new entry
        if (remain >= need_length)
        {
            mydp->rec_len = ideal_length;
            cp += mydp->rec_len;
            mydp = (DIR *)cp;
            mydp->inode = ino;

            strcpy(mydp->name, name);
            mydp->rec_len = remain;
            mydp->name_len = strlen(name);
            put_block(pip->dev, ip->i_block[i], buf);
            printf("successfully created %s\n", name);
            return 0;
        }
        else  // if the remaining space is not enough to enter the new entry
        {
            // allocate a new data block; increment parent size by BLKSIZE
            blk = balloc(dev);
            ip->i_size = BLKSIZE;
            ip->i_block[i] = blk;
            pip->dirty = 1;

            get_block(pip->dev, blk, buf);
            mydp = (DIR *)buf;
            cp = buf;
            // CONFUSED ON THIS STEP

            // enter the new entry as the first entry in the new data block with rec_len = BLKSIZE
            mydp->inode = ino;
            mydp->rec_len = BLKSIZE;
            mydp->name_len = strlen(name);
            strcpy(mydp->name, name);
            put_block(pip->dev, blk, buf);
             printf("succesfully created %s\n", name);
            return 0;
        }
    }
}

int mycreat(char *pathname)
{
    // 1 - divide the pathname into dirname and basename
    char temp[128];
    strcpy(temp, pathname);
    char *dname = dirname(temp);
    strcpy(temp, pathname);
    char *bname = basename(temp);

    // 2 - dirname must exist and is a DIR
    int pino = getino(dname);
    MINODE *pmip = iget(dev, pino);
    if (S_ISDIR(pmip->INODE.i_mode) == 0)
    {
        printf("%s is not a directory\n", dname);
        return -1;
    }

    // 3 - basename must not exist in parent DIR, must return 0 
    if (search(pmip, bname) != 0)
    {
        printf("%s already exists\n", bname);
        return -1;
    }

    // 4 - call kcreat(pmip, basename) to create a FILE;
    kcreat(pmip, bname);
    pmip->ref_count++;
    pmip->dirty = 1;
    iput(pmip);

    return 0;
}

void print_inode_stuff(INODE *ip)
{
    printf("i_mode: %o\n", ip->i_mode);
    printf("i_uid: %d\n", ip->i_uid);
    printf("i_size: %d\n", ip->i_size);
    printf("i_gid: %d\n", ip->i_gid);
    printf("i_links_count: %d\n", ip->i_links_count);
    printf("i_atime: %d\n", ip->i_atime);
    printf("i_ctime: %d\n", ip->i_ctime);
    printf("i_mtime: %d\n", ip->i_mtime);
    printf("i_dtime: %d\n", ip->i_dtime);
    printf("i_blocks: %d\n", ip->i_blocks);
    printf("i_block[0]: %d\n", ip->i_block[0]);
    printf("i_block[1]: %d\n", ip->i_block[1]);
    printf("i_block[2]: %d\n", ip->i_block[2]);
    printf("i_block[3]: %d\n", ip->i_block[3]);
    printf("i_block[4]: %d\n", ip->i_block[4]);
}

int kcreat(MINODE *pmip, char* bname)
{

    // This is similar to kmkdir() except
    // 1 - the INODE.i_mode field is set to RED file type, permission bits set to 0644 = rw-r--r-- = 0x81A4
    // 2 - no data block is allocated for it, so the file size is 0.
    // 3 - links_count = 1; Do not increment parent INODE's links_count.
    // for more info on link count: 
    // https://www.theunixschool.com/2012/10/link-count-file-vs-directory.html#:~:text=By%20default%2C%20a%20file%20will,have%20a%20link%20count%201.&text=The%20file%2C%20test.

    // 1 - allocate an INODE
    int ino = ialloc(dev);
    int blk = balloc(dev);

    // 2 - create/setup INODE
    MINODE *mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    ip->i_mode = 0x81A4;                                // 00644: RED file type and permissions = rw-r--r-- = 0x81A4
    ip->i_uid = running->uid;                           // owner uid
    ip->i_gid = running->gid;                           // group Id
    ip->i_size = BLKSIZE;                               // size in bytes
    ip->i_links_count = 1;                              // links count = 1 not incremented
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L); // set to current time
    ip->i_blocks = 2;                                   // LINUX: Blocks count in 512-byte chunks
    ip->i_block[0] = 0;                                 // new FILE has one data block not incremented
    for (int i = 1; i < 15; i++)                        // ip->i_block[1] = 0 to ip->i_block[0] = 0;
        ip->i_block[i] = 0;
    mip->dirty = 1;                                     // mark minode dirty
    iput(mip);                                          // write INODE to disk 

    // print_inode_stuff(ip); will probably use for state

    // 3 - enter the child 
    // TODO: bname should be char* type? Should it be something else?
    enter_child(pmip, ino, bname);
    return 0;
}

int clr_bit(char *buf, int bit)
{
    buf[bit/8] &= ~(1 << (bit%8));
}

int incFreeInodes(int dev)
{
    char buf[BLKSIZE];
    // inc free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count++;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count++;
    put_block(dev, 2, buf);
    return 0;
}

int idalloc(int dev, int ino)
{
    int i;
    char buf[BLKSIZE];
    // MTABLE *mp = (MTABLE *)get_mtable(dev);
    if (ninodes) // niodes global
    {
        printf("inumber %d out of range\n", ino);
        return -1;
    }
    // get inode bitmap block
    get_block(dev, imap, buf);
    clr_bit(buf, ino-1);
    // write buf back 
    put_block(dev, imap, buf);
    // update free inode count in SUPER and GD
    incFreeInodes(dev);

    return 0;
}

int bdalloc(int dev, int bno)
{
    // function deallocates a disk block (number) bno
    int i;
    char buf[BLKSIZE];
    // MTABLE *mp = (MTABLE *)get_mtable(dev);
    if (nblocks) // niodes global
    {
        printf("bnumber %d out of range\n", bno);
        return -1;
    }

    get_block(dev, bmap, buf);
    clr_bit(buf, bno-1);
    put_block(dev, bmap, buf);
    incFreeInodes(dev);
    return 0;
}

int rmdir (char *pathname)
{
    // divide the pathname into dirname and basename
    char temp[128];
    strcpy(temp, pathname);
    char *dname = dirname(temp);
    strcpy(temp, pathname);
    char *bname = basename(temp);

    // 1 - get ino in memory INODE of pathname;
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino);

    // 2 - verify INODE is and DIR (by INODE.i_mode field)
    if (S_ISDIR(mip->INODE.i_mode) == 0)
    {
        printf("%s is not a directory\n", pathname);
        return -1;
    }
    // minode is not BUSY (refcount = 1);
    if (mip->ref_count != 1)
    {
        printf("%s is busy\n", pathname);
        return -1;
    }
    // if more than 2 links, then we have something besides `.` and `..`
    if (mip->INODE.i_links_count > 2)
    {
        printf("Links count greater than two\n");
        return -1;
    }

    if (mip->INODE.i_mode & 040755 == 0)
    {
        printf("Incorrect access permissions: %o\n", mip->INODE.i_mode);
        printf("Expected: %o | %o\n", 0777, 0755);
        return -1;
    }

    if (running->uid != mip->INODE.i_uid && running->uid != 0)
    {
        printf("You do not have permission to delete this directory\n");
        return -1;
    }

    for (int i = 0; i < mip->INODE.i_blocks; ++i)
    {
        if (mip->INODE.i_block[i] == 0)
        {
            break;
        }
        bdalloc(mip->dev, mip->INODE.i_block[i]);
    }
    idalloc(mip->dev, mip->ino);
    iput(mip);
    mip->ref_count = 0;
    mip->dirty = 1;

    // verify DIR is empty (transverse data blocks for number of entries = 2)
    // WRITE CODE HERE

    // 3 - get parent's ino and inode
    int pino = getino(dname);
    if (pino == -1)
    {
        printf("parent directory does not exist\n");
        return -1;
    }
    MINODE *pmip = iget(mip->dev, pino);

    // 4 - get name from parent DIR's data block
    findmyname(pmip, ino, bname);

    // 5 - remove name from parent directory
    // Here is SEG FAULT
    rm_child(pmip, bname);

    // 6 - dec parent link_count by 1; mark parent pmip dirty;
    pmip->INODE.i_links_count--;
    pmip->dirty = 1;

    // 7 - deallocate its data blocks and inode 
    bdalloc(mip->dev, mip->INODE.i_block[0]);
    idalloc(mip->dev, mip->ino);
    iput(pmip);

    return 0;
}

int rm_child(MINODE *pmip, char *name)
{
    /*

    Algorithm of rm_child
    1- Search parent INODE's data block(s) for the entry of name

    2- Delete name entry from parent directory by (For all cases below refer to page 339)

        utilize memcpy 

        2.1- if (first and only entry in a data block)
                deallocate the data block; reduce parent's file size by BLKSIZE;
                compact parent's i_block[ ] array to eliminate the deleted entry if it's
                between nonzero entries;
        2.2- else if (LAST entery in block)
                Absorb its rec_len to the predecessor entry, as shown in the following diagrams
        2.3- else
                move all trailing entries LEFT to overlay the deleted entry;
                add deleted rec_len to the LAST entry; do not change parent's file size;
        
    */
    // WRITE CODE HERE
    DIR* lastdp = NULL;
    DIR* prevdp = NULL;
    for (int i = 0; i < 12; ++i)
    {
        if (pmip->INODE.i_block[i] == 0)
        {
            break;
        }
        char buf[BLKSIZE];
        // char temp[BLKSIZE >> 2];
        get_block(pmip->dev, pmip->INODE.i_block[i], buf);
        DIR* mydp = (DIR*)buf;
        char* cp = buf;
        while (cp < buf + BLKSIZE)
        {
            // strncpy(temp, mydp->name, mydp->name_len);
            // printf("TEMP: %s\n", temp);
            // temp[mydp->name_len] = '\0';
            if (strncmp(mydp->name, name, mydp->name_len) == 0)
            {
                if (mydp->rec_len == BLKSIZE)
                {
                    if (cp == buf)
                    {
                        bdalloc(pmip->dev, pmip->INODE.i_block[i]);
                        pmip->INODE.i_size -= BLKSIZE;
                        while (i < 12 && pmip->INODE.i_block[i] == 0)
                        {
                            ++i;
                            get_block(pmip->dev, pmip->INODE.i_block[i], buf);
                            put_block(pmip->dev, pmip->INODE.i_block[i], buf);
                        }
                    }
                    else
                    {
                        lastdp->rec_len += mydp->rec_len; 
                        put_block(pmip->dev, pmip->INODE.i_block[i], buf);
                    }
                }
                else if (cp + mydp->rec_len >= BLKSIZE + buf)
                {
                    prevdp->rec_len += mydp->rec_len; 
                    put_block(pmip->dev, pmip->INODE.i_block[i], buf);
                }
                else
                {
                    DIR* finaldp = (DIR*)buf;
                    char* finalcp = buf;
                    while (finalcp + finaldp->rec_len < buf + BLKSIZE)
                    {
                        finalcp += finaldp->rec_len;
                        finaldp = (DIR*)finalcp;
                    }
                    finaldp->rec_len += mydp->rec_len;
                    memmove(cp, cp + mydp->rec_len, (buf + BLKSIZE) - (cp + mydp->rec_len));
                    put_block(pmip->dev, pmip->INODE.i_block[i], buf);
                }
                pmip->dirty = 1;
                iput(pmip);
                printf("successfully deleted %s\n", name);
                return 0;
            }
            lastdp = mydp;
            cp += mydp->rec_len;
            prevdp = mydp;
            mydp = (DIR*)cp;
        }
    }
    return -1;
}