#ifndef COMMANDS_H
#define COMMANDS_H

/**** cd_ls_pwd.c ****/

int cd();
int ls_file();
int ls_dir();
int ls();
int pwd();
int rpwd();

/**** mkdir_creat_rmdir.c ****/

int tst_bit();
int set_bit();
int decFreeInodes();
int ialloc();
int balloc();
int mkdir();
int kmkdir();
int enter_child();
int creat();
int kcreat();
int clr_bit();
int incFreeInodes();
int idalloc();
int bdalloc();
int rmdir();
int rm_child();

/**** links.c ****/

int link();
int unlink();
int symlink();
int readlink();

#endif