/************* cat_cp_mv.c file **************/

#include <sys/stat.h>
#include <ext2fs/ext2fs.h>
#include <fcntl.h>
#include <libgen.h>
#include <time.h>
#include <string.h>
#include "commands.h"
#include "type.h"
#include "util.h"

int cat(char *pathname)
{
    char mybuf[BLKSIZE];
    int n;

    int fd = myopen(pathname, READ);
    if (fd < 0)
    {
        printf("cat: cannot open %s\n", pathname);
        return -1;
    }

    while ((n = myread(fd, BLKSIZE, mybuf)) > 0) // -1 is error.
    {
        mybuf[n] = '\0'; // Icky infinite loop
        // must spit out chars from mybuf[ ] \n properly; ??
        char *cp = mybuf; // check char by char for NULL or \n
        while (*cp)
        {
            putchar(*cp++);
        }
    }
    myclose(fd);
    return 0;
}

/*
Algorithm for cp:
1. fd = open src for READ;
2. gd = open dest for READ_WRITE OR WRITE;
NOTE:  In the project, you may have to creat the dst file first, then open it 
        for WR, OR  if open fails due to no file yet, creat it and then open it
        for WR.

3. while (n = read(fd, buf, BLKSIZE))
    {
        write(gd, buf, n); // notioice the n in write()
    }
*/

int cp(const char *src_file, char *dest_file)
{
    char buf[BLKSIZE];
    unsigned int total_bytes = 0;
    int fd = myopen(src_file, READ);
    int gd = myopen(dest_file, WRITE);
    int n;

    if (fd < 0 || gd < 0)
    {
        printf("cp: cannot open %s or %s\n", src_file, dest_file);
        return -1;
    }

    printf("cp: copying %s to %s\n", src_file, dest_file);
    printf("cp: copying %d to %d\n", fd, gd);
    while ((n = myread(fd, BLKSIZE, buf)) > 0)
    {
        mywrite(gd, n, buf);
        memset(buf, 0, BLKSIZE);
        printf("TOTAL BYTES: %d\n", total_bytes);
        total_bytes += n;
    }

    printf("total bytes copied: %d\n", total_bytes);
    myclose(fd);
    myclose(gd);

    return 0;
}

/*
Algorithm for mv:
1. verify src exists; get its INODE in ==> you already know its dev
2. check whether src is on the same dev as src

              CASE 1: same dev:
3. Hard link dst with src (i.e. same INODE number)
4. unlink src (i.e. rm src name from its parent directory and reduce INODE's
               link count by 1).
                
              CASE 2: not the same dev:
3. cp src to dst
4. unlink src
*/
int mv(char *src_file, char *dest_file) // NOT FINISHED
{
    // THIS IS A MINOR FUNCTION LEAVE IT TILL LATER
    // 1. verify src exists; get its INODE in ==> you already know its dev
    int ino = getino(src_file);
    if (ino < 0)
    {
        printf("mv: cannot find %s\n", src_file);
        return -1;
    }
    // 2. check whether src is on the same dev as src
    // ???

    // 3. Hard link dst with src (i.e. same INODE number)
    symlink(dest_file, src_file);
    // OR
    // cp src to dst
    cp(src_file, dest_file);

    // 4. unlink src (i.e. rm src name from its parent directory and reduce INODE's link count by 1).
    unlink(src_file);
    return 0;
}