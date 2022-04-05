#ifndef TYPE_H
#define TYPE_H
/*************** type.h file for LEVEL-1 ****************/
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;   

#define FREE        0
#define READY       1

#define BLKSIZE  1024
#define NMINODE   128
#define NFD        10
#define NPROC       2

typedef struct minode{
  INODE INODE;           // INODE structure on disk
  int dev, ino;          // (dev, ino) of INODE
  int ref_count;          // in use count
  int dirty;             // 0 for clean, 1 for modified

  int mounted;           // for level-3
  struct mntable *mptr;  // for level-3
}MINODE;

typedef struct oft{
  int mode;
  int ref_count;
  MINODE* minode_ptr;
  int offset;
}OFT;

typedef struct proc{
  struct proc *next;
  int          pid;      // process ID  
  int          uid;      // user ID
  int          gid;
  MINODE      *cwd;      // CWD directory pointer  
  OFT         *fd[NFD];
}PROC;

typedef struct mtable {
  int dev;              // device number; 0 for FREE
  int ninodes;          // from superblock
  int nblocks;
  int free_blocks;      // from superblock and GD
  int free_inodes;
  int bmap;             // from group descriptor
  int imap;
  int iblock;           // inodes start block
  MINODE *mntDirPtr;    // mount point DIR pointer
  char devName[64];     // device name
  char mntName[64];     // mount point DIR name
} MTABLE;

#endif