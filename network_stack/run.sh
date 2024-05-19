#!/bin/bash
make
sudo valgrind --tool=memcheck  --show-leak-kinds=all ./build/stack
