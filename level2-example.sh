#! /bin/bash

GREEN='\033[0;32m'
NC='\033[0m' # No Color

function pause() {
    read -s -n 1 -p "Press any key to continue . . ."
    echo ""
    clear
}

./scripts/mkdisk-level2.sh
./scripts/mk.sh


printf "\n${GREEN}\t-- cat large;       show contents to LAST LINE${NC}\n"
pause

./scripts/run.sh << EOF
cat large
quit
EOF

printf "\n${GREEN}\t-- cat huge       show contents to LAST LINE${NC}\n"
pause

./scripts/run.sh << EOF
cat huge
quit
EOF

printf "\n${GREEN}\t-- cp  large newlarge; ls    show they are SAME size${NC}\n"
pause
./scripts/run.sh << EOF
cp large newlarge
ls
quit
EOF

printf "\n${GREEN}\t-- cp  huge  newhuge ; ls    show they are SAME size${NC}\n"
pause
./scripts/run.sh << EOF
cp huge newhuge
ls
quit
EOF

printf "\n${GREEN}\t-- open  small 0;   pfd      show fd=0 opened for R ${NC}\n"
printf "\n${GREEN}\t-- read 0 20;       pfd      show 20 chars read${NC}\n"
printf "\n${GREEN}\t-- open file1 1;    pfd      show fd=1 opened for W${NC}\n"
printf "\n${GREEN}\t-- write 1 abcde; ls       show file1 size=5${NC}\n"
printf "\n${GREEN}\t-- close 1; pfd              show fd=1 is closed${NC}\n"
pause
./scripts/run.sh << EOF
open small 0
read 0 20
open file1 1
write 1 abcde
ls
close 1
pfd
quit
EOF

