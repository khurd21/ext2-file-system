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

    while ((n = myread(fd, BLKSIZE, mybuf)))
    {
        mybuf[n] = 0;
        printf("%s", mybuf); // THIS works but it is not good
        // must spit out chars from mybuf[ ] \n properly;
        
    }
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

int cp(char *src_file, char *dest_file)
{
    char buf[BLKSIZE];
    int total_bytes = 0;
    int fd = myopen(src_file, READ);
    int gd = myopen(dest_file, READ_WRITE);
    int n;

    if (fd < 0 || gd < 0)
    {
        printf("cp: cannot open %s or %s\n", src_file, dest_file);
        return -1;
    }

    while (n = myread(fd, BLKSIZE, buf))
    {
        mywrite(gd, n, buf);
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
int mv(char *src_file, char *dest_file)
{
    // THIS IS A MINOR FUNCTION LEAVE IT TILL LATER
    return 0;
}