/************* mount_unmount.c file **************/

#include <sys/stat.h>
#include <ext2fs/ext2fs.h>
#include <fcntl.h>
#include <libgen.h>
#include <time.h>
#include <string.h>
#include "commands.h"
#include "type.h"
#include "util.h"


/*
Algorithm for mount
1- Ask for a filesys (a virtual disk) and mount_point (a DIR pathname)
   If no parameter, display current mounted file systems;
2- Check whether the filesys is already mounted:
   The MOUNT table entries contain mounted file system (device) names
   and their mounting poiunts. Reject if the device is already mounted.
3. Open the filesys virtual disk (Under Linux) for RW; use (Linux) file
   descriptor as new dev. Read filesys superblock to vertify it is an 
   ext2 FS.
4. Find the ino, and then the minode of the mount_point:  
        ino - getino(pathname);  // get the ino of the mount point
        mip - iget(dev, ino);    // load its ino into memory
5. Check mount_point is a DIR and not busy, e.g. not someone's CWD.
6. Record new dev and filesys name in the MOUNT table entry, store its
   ninodes, nblocks, bmap, imap and inodes start block, etc. for quick access
7. Mark mount_point minode as mounted on (mounted flag = 1). and let it point
   at the MOUNT table entry, which points back to the mount_point minode.
 */
int mount(char *filesys, char *mount_point)
{
   // if no parameter, display current mounted file systems
   if(strcmp(filesys, "") == 0 && strcmp(mount_point, "") == 0)
   {
      printf("Currently mounted file systems:\n");
      for(int i = 0; i < NMTABLE; i++)
      {
        if(mtable[i].dev != 0) // if the device is not 0, it is mounted because it is not free
        {
           printf("%s on %s\n", mtable[i].dev_name, mtable[i].mnt_name);
        }
      }
      return 0;
   }

   // 2 - Check whether the filesys is already mounted:
   // The MOUNT table entries contain mounted file system (device) names
   // and their mounting poiunts. Reject if the device is already mounted.
   for(int i = 0; i < NMTABLE; i++)
   {
      if(strcmp(mtable[i].dev_name, filesys) == 0)
      {
         printf("%s is already mounted\n", filesys);
         return 0;
      }
   }

   // check if there is a dev spot in the mtable which is free
   for (int i = 0; i < NMTABLE; i++)
   {
      if(mtable[i].dev == 0)
      {
         break;
      }
      else if (i == (NMTABLE - 1))
      {
         printf("mount: No more free MOUNT table entries\n");
         return -1;
      }
   }
   
   // 3. Open the filesys virtual disk (Under Linux) for RW; use (Linux) file
   // descriptor as new dev. Read filesys superblock to vertify it is an
   // ext2 FS.
   int fd = open(filesys, O_RDWR);
   printf("fd = %d\n", fd);
   if(fd < 0 || fd > NFD)
   {
      printf("mount: cannot open %s because fd is invalid\n" , filesys);
      return -1;
   }

   // Read filesys superblock to vertify it is an ext2 FS.
   char buf[BLKSIZE];
   get_block(fd, 1, buf); // from LAB 5 - get the super block from the disk
   SUPER *sp = (SUPER *)buf;

   // Check if it is an ext2 FS
   if(sp->s_magic != MAGIC)
   {
      printf("mount: %s is not an ext2 FS\n", filesys);
      return -1;
   }

   // 4. Find the ino, and then the minode of the mount_point: (has to be mount on the current dir)
   int ino = getino(mount_point);
   MINODE *mip = iget(running->cwd->dev, ino);

   // 5. Check mount_point is a DIR and not busy, e.g. not someone's CWD.
   if(!S_ISDIR(mip->INODE.i_mode))
   {
      printf("mount: %s is not a directory\n", mount_point);
      return -1;
   }

   // check that the mount_point is not busy (and folders start at 2 because of . and ..)
   if(mip->ref_count > 2)
   {
      printf("mount: %s is busy\n", mount_point);
      return -1;
   }

   // 6. Record new dev and filesys name in the MOUNT table entry, store its
   // ninodes, nblocks, bmap, imap and inodes start block, etc. for quick access
   for(int i = 0; i < NMTABLE; i++)
   {
      if(mtable[i].dev == 0)
      {
         mtable[i].dev = fd;
         strcpy(mtable[i].dev_name, filesys);
         strcpy(mtable[i].mnt_name, mount_point);
         mtable[i].ninodes = sp->s_inodes_count;
         mtable[i].nblocks = sp->s_blocks_count;
         mtable[i].iblock = sp->s_first_data_block;
         mtable[i].dev = dev;
         mtable[i].mnt_dir_ptr = mip;

         mip->mptr = mtable[i].mnt_dir_ptr;
         mip->mounted = 1;
         break;
      }
   }

   return 0;
}

/*
Algorithm for unmount
1- Search the MOUNT table to check filesys is indeed mounted. 
2- Check whether any file is active in the mounted filesys; if so, reject.
3- Find the mount_point in-memory inode, which should be in memory while it is
   mounted on. Reset the MINODE's mounted flag to 0; then iput() the minode. 
*/
int unmount(char *filesys)
{
   printf("in unmount\n");
   // 1 - Search the MOUNT table to check filesys is indeed mounted. 
   for(int i = 0; i < NMTABLE; i++)
   {
      // checks if the filesys is mounted and dev is not 0 (not free)
      if(strcmp(mtable[i].dev_name, filesys) == 0 && mtable[i].dev != 0)
      {
         // 2 - Check whether any file is active in the mounted filesys; if so, reject.
         for(int j = 0; j < NMINODE; j++)
         {
            if(minode[j].dev == mtable[i].dev && minode[j].mounted == 1)
            {
               printf("unmount: %s has active files\n", filesys);
               return -1;
            }
         }

         // 3 - Find the mount_point in-memory inode, which should be in memory while it is
         // mounted on. Reset the MINODE's mounted flag to 0; then iput() the minode. 
         MINODE *mip = mtable[i].mnt_dir_ptr;
         mip->mounted = 0;
         iput(mip);
         printf("unmounted: %s\n", filesys);
         return 0;
      }
   }

   printf("unmount: %s is not mounted\n", filesys);
   return -1;
}
/*
Algorithm for access
1 - int r;
2 - if (SUPERuser: running->uid == 0)
      return 1;
3 - // NOT SUPERuser: get file's INODE
    ino = getino(filename);
    mip = iget(dev, ino);
4 - if (OWNER: mip->INODE.i_uid == running->uid)
      r = (check owner's rwx with mode); // by tst_bit()
    else
      r = (check other's rwx with mode); // by tst_bit()
5 - iput(mip);
6 - return r;

returns 1 if the user has the mode's permission to the file, 0 otherwise
*/
int access(char *filename, char mode) // mode = r|w|x:
{
   // 1 - int r;
   int r = 0;

   // 2 - if (SUPERuser: running->uid == 0)
   if(running->uid == 0)
      return 1;

   // 3 - // NOT SUPERuser: get file's INODE
   int ino = getino(filename);
   MINODE *mip = iget(dev, ino);
   // 4 - if (OWNER: mip->INODE.i_uid == running->uid)
   if(mip->INODE.i_uid == running->uid)
   {
      // 4.1 - r = (check owner's rwx with mode); // by tst_bit()
   }
   else
   {
      // 4.2 - r = (check other's rwx with mode); // by tst_bit()
   }

   // 5 - iput(mip);
   iput(mip);
   
   return r;
}

int switch_user()
{
   if(running->uid == 0)
      running->uid = 1;
   else
      running->uid = 0;
   return 0;
}

/*
cd_ls_pwd:   must have x and r permissions
mkdir_creat: must have rwx permissions to perent dir
rmdir_unlink: must be the owner
link old_file new_file: must have rwx on old_file, and rwx to the parent dir of new_file
open file mode: must have the mode permission to the file

Then Switch process to run P1 whose uid = 1, so it's not a superuser process.
and test to see if it can access the file.

*/