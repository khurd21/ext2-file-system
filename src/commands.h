#ifndef COMMANDS_H
#define COMMANDS_H
#include "type.h"

/**** cd_ls_pwd.c ****/

int cd(char* pathname);
int ls_file(MINODE *mip, char *name);
int ls_dir(MINODE *mip);
int ls(char *pathname);
int pwd(MINODE *wd);
int rpwd(MINODE *wd);

/**** mkdir_creat_rmdir.c ****/

int tst_bit(char *buf, int bit);
int set_bit(char *buf, int bit);
int decFreeInodes(int dev);
int ialloc(int dev);
int balloc(int dev);
int mymkdir(char *pathname);
int kmkdir(MINODE *pmip, char *bname);
int enter_child(MINODE *pip, int ino, char *child);
int mycreat(char *pathname);
int kcreat(MINODE *pmip, char *bname);
int clr_bit(char *buf, int bit);
int incFreeInodes(int dev);
int idalloc(int dev, int ino);
int bdalloc(int dev, int ino);
int rmdir(const char *pathname);
int rm_child(MINODE *pmip, char *name);

/**** links.c ****/

int link(char *old_file, char *new_file);
int unlink(char *pathname);
int symlink(char *old_file, char *new_file);
int readlink(char *file, char *buf);

/**** stat_chmod_utime.c ****/

int mystat(char *pathname);
int mychmod(int mode, char *pathname);
int utime(char *pathname);

/**** open_close.c ****/

int myopen(char *pathname, int mode);
int inode_truncate(MINODE *pmip);
int myclose(int fd);
int mylseek(int fd, int position);
int pfd();
int dup(int fd);
int dup2(int fd, int gd);

/**** read_write.c ****/

int myread(int fd, int nbytes, void* buf);
int mywrite(int fd, int nbytes, const void* buf);
int map(MINODE *mip, int lbk);

/**** cat_cp_mv.c ****/

int cat(char *pathname);
int cp(const char *src_file, char *dest_file);
int mv(char *src_file, char *dest_file);

/**** mount_unmount.c ****/
int mount(const char *filesys, const char *mount_point);
int unmount(char *filesys);
int access(char *filename, char mode);
int switch_user();

#endif