# Cpts360-Final-Project

Implementation of the EXT2 Filesystem

## Links Related to Project

Project Specification: https://eecs.wsu.edu/~cs360/proj22.html

Lab 6 Link: https://eecs.wsu.edu/~cs360/mountroot.html

HOW TO MKDIR AND CREAT: https://eecs.wsu.edu/~cs360/mkdir_creat.html


## EXT2 Filesystem Components

```
-------  LEVEL 1 ------------ 
        REQUIRED:
mount_root, ls, cd, pwd,
mkdir, creat, rmdir,
link,  unlink, symlink, readlink

         MINOR                      
stat,  chmod, utime;

-------  LEVEl 2 -------------
	      REQUIRED:
open, close, read, write, cat, cp,

		     MINOR:
lseek, mv

-------  LEVEl 3 ------------ 
        REQUIRED:
mount, umount, cross mounting points
File permission checking
-----------------------------
```
