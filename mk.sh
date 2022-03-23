#! /bin/sh

rm a.out 2> /dev/null
gcc ./src/*.c -o bin/project-exe
./bin/project-exe
