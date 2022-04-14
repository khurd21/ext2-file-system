#! /bin/sh

mkdir -p bin
rm bin/* 2> /dev/null
gcc ./src/*.c -o bin/project-exe
./bin/project-exe
