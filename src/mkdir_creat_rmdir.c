/************* mkdir_creat_rmdir.c file **************/

// #include <sys/stat.h>
#include <ext2fs/ext2fs.h>
// #include <fcntl.h>
#include <libgen.h>
#include <time.h>
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
    MTABLE *mp = (MTABLE *)get_mtable(dev);
    get_block(dev, mp->imap, buf);
    for (i = 0; i < mp->ninodes; i++)
    {
        if (tst_bit(buf, i) == 0)
        {
            set_bit(buf, i);
            put_block(dev, mp->imap, buf);
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
    char buf[BLKSIZE];
    MTABLE *mp = (MTABLE *)get_mtable(dev);
    get_block(dev, mp->bmap, buf);
    for (i = 0; i < mp->nblocks; i++)
    {
        if (tst_bit(buf, i) == 0) // order might differ as deallocation must happen
        {
            set_bit(buf, i);
            put_block(dev, mp->bmap, buf);
            decFreeInodes(dev);
            return (i + 1);
        }
    }
    return 0; // out of FREE inodes
}

int mkdir (char *pathname)
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

    // 4 - call kmkdir(pmip, basename) to create a DIR;
    kmkdir(pmip, bname);

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
    DIR *dp = (DIR *)buf;

    // make . entry
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->name[0] = '.';

    // make .. entry: pino=parent DIR ino, blk=allocated block
    dp = (char *)dp + 12;
    dp->inode = pmip->ino;       // IN BOOK dp->inode = pino;
    dp->rec_len = BLKSIZE - 12;  // rec_len spans block
    dp->name_len = 2;
    dp->name[0] = dp->name[1] = '.';
    put_block(dev, blk, buf);

    // enter the child
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
    int i;
    char buf[BLKSIZE];

    // set the needed lengh for the new entry name
    int need_length = 4 * ((8 + strlen(name) + 3) / 4); // a multiple of 4

    for (i = 0; i < 12; i++)
    {

        // if the i_block[i] is 0, break
        if (pip->INODE.i_block[i] == 0)
            break;

        // get the data block into buf[ ]
        get_block(pip->dev, pip->INODE.i_block[i], buf);

        // dp points to the last entry in the data block as DIR
        DIR *dp = (DIR *)buf;

        // cp points to the last entry in the data block as char
        char *cp = buf;

        // traverse the data block to find the last entry
        while (cp + dp->rec_len < buf + BLKSIZE)
        {
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }

        // remain is the remaining space in the last entry
        int ideal_length = 4 * ((8 + dp->name_len + 3) / 4);
        int remain = dp->rec_len - ideal_length;

        // if the remaining space is enough to enter the new entry
        if (remain >= need_length)
        {
            dp->rec_len = ideal_length;
            cp += dp->rec_len;
            dp = (DIR *)cp;
            dp->inode = ino;

            dp->rec_len = remain;
            dp->name_len = strlen(name);
            strncpy(dp->name, name, dp->name_len);
            put_block(pip->dev, pip->INODE.i_block[i], buf);
            return 0;
        }
        else  // if the remaining space is not enough to enter the new entry
        {
            // allocate a new data block; increment parent size by BLKSIZE
            int blk = balloc(pip->dev);
            pip->INODE.i_size = BLKSIZE;
            pip->INODE.i_block[i] = blk;
            pip->dirty = 1;

            get_block(pip->dev, blk, buf);
            dp = (DIR *)buf;
            cp = buf;
            // CONFUSED ON THIS STEP

            // enter the new entry as the first entry in the new data block with rec_len = BLKSIZE
            dp->inode = ino;
            dp->rec_len = BLKSIZE;
            dp->name_len = strlen(name);
            strncpy(dp->name, name, dp->name_len);
            put_block(pip->dev, pip->INODE.i_block[i], buf);
            return 0;
        }
    }
}

int creat (char *pathname)
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

    return 0;
}

int kcreat(MINODE *pmip, int bname)
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
    MTABLE *mp = (MTABLE *)get_mtable(dev);
    if (mp->ninodes) // niodes global
    {
        printf("inumber %d out of range\n", ino);
        return -1;
    }
    // get inode bitmap block
    get_block(dev, mp->imap, buf);
    clr_bit(buf, ino-1);
    // write buf back 
    put_block(dev, mp->imap, buf);
    // update free inode count in SUPER and GD
    incFreeInodes(dev);

    return 0;
}

int bdalloc(int dev, int bno)
{
    // function deallocates a disk block (number) bno
    int i;
    char buf[BLKSIZE];
    MTABLE *mp = (MTABLE *)get_mtable(dev);
    if (mp->nblocks) // niodes global
    {
        printf("bnumber %d out of range\n", bno);
        return -1;
    }

    get_block(dev, mp->bmap, buf);
    clr_bit(buf, bno-1);
    put_block(dev, mp->bmap, buf);
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
    

    // verify DIR is empty (transverse data blocks for number of entries = 2)
    // WRITE CODE HERE

    // 3 - get parent's ino and inode
    int pino = findino();
    MINODE *pmip = iget(dev, pino);

    // 4 - get name from parent DIR's data block
    findmyname(pmip, ino, bname);

    // 5 - remove name from parent directory
    rm_child(pmip, ino);

    // 6 - dec parent link_count by 1; mark parent pmip dirty;
    pmip->INODE.i_links_count--;
    pmip->dirty = 1;

    // 7 - deallocate its data blocks and inode 
    bdalloc(mip->dev, mip->INODE.i_block[0]);
    idalloc(mip->dev, mip->ino);
    iput(pmip);

    return 0;
}

rm_child(MINODE *pmip, char *name)
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

    return 0;
}