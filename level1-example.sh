#! /bin/bash


GREEN='\033[0;32m'
NC='\033[0m' # No Color

function pause() {
    read -s -n 1 -p "Press any key to continue . . ."
    echo ""
    clear
}

printf "\n${GREEN}\t-- mkdisk: create a virtual disk diskimage\n${NC}"
./scripts/mkdisk-level1.sh
./scripts/mk.sh


printf "\n${GREEN}\t-- menu: show the menu of the program\n${NC}"
printf "\n${GREEN}\t-- ls: show contents of / directory\n${NC}"
pause
./scripts/run.sh << EOF
menu
ls
quit
EOF


printf "\n${GREEN}\t-- mkdir /a: show contents of root directory\n${NC}"
printf "\n${GREEN}\t-- mkdir /a/b: show contents of /a directory\n${NC}"
pause
./scripts/run.sh << EOF
mkdir /a
ls
mkdir /a/b
ls /a
quit
EOF


printf "\n${GREEN}\t-- cd /a/b and pwd: cd to a pathname, show cwd\n${NC}"
printf "\n${GREEN}\t-- cd    ../../ ; pwd       cd upward, show CWD\n${NC}"
pause
./scripts/run.sh << EOF
cd /a/b
pwd
cd ../../
pwd
quit
EOF


printf "\n${GREEN}\t-- creat f1     ; ls        creat file, show f1 is a file${NC}\n"
printf "\n${GREEN}\t-- link  f1 f2;   ls        hard link, both linkCount=2${NC}\n"
printf "\n${GREEN}\t-- unlink   f1;   ls        unlink f1; f2 linkCount=1${NC}\n"
pause
./scripts/run.sh << EOF
creat f1
ls
link f1 f2
ls
unlink f1
ls
quit
EOF

printf "\n${GREEN}\t-- symlink f2 f3; ls        symlink; ls show f3 -> f2${NC}\n"
pause
./scripts/run.sh << EOF
symlink f2 f3
ls
quit
EOF

printf "\n${GREEN}\t-- rmdir /a/b;    ls        rmdir and show results${NC}\n"
pause
./scripts/run.sh << EOF
rmdir /a/b
ls /a
quit
EOF

printf "\n${GREEN}\tDone.${NC}\n"