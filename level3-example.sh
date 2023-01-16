#! /bin/bash

GREEN='\033[0;32m'
NC='\033[0m' # No Color

function pause() {
    read -s -n 1 -p "Press any key to continue . . ."
    echo ""
    clear
}

printf "\n${GREEN}\t-- mount images/disk3 /mnt${NC}\n"
printf "\n${GREEN}\t-- mount images/disk3 /mnt${NC} [should fail]\n"
pause
./scripts/run.sh << EOF
mkdir /mnt
mount images/disk3 /mnt
mount images/disk3 /mnt
quit
EOF

printf "\n${GREEN}\t-- mount images/disk3 /mnt${NC}\n"
printf "\n${GREEN}\t-- umount images/disk3 ${NC}\n"
pause
./scripts/run.sh << EOF
mount images/disk3 /mnt
umount images/disk3
quit
EOF
