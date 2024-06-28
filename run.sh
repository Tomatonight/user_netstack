#!/bin/bash
make clean
make
sudo valgrind --tool=memcheck  --show-leak-kinds=all ./build/stack
