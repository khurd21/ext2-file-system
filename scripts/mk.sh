#! /bin/sh

rm bin/* 2> /dev/null
gcc ./src/*.c -g -o bin/project-exe
