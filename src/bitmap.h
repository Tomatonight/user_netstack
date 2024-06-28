#pragma once
#include <sys/types.h>
#include <memory.h>
#include<iostream>
#include <stdio.h>
class bitmap
{
public:
    bitmap(u_int32_t bits_len, u_int32_t base_value)
    {
        len = bits_len / 8;
        base = new char[len];
        memset(base, 0, len);
        start_value = base_value;
    }
    ~bitmap()
    {
        if (base)
            delete[] base;
    }
    u_int32_t alloc_value();
    void free_value(u_int32_t value);
    bool test(uint32_t value);
   void set_value(uint32_t, bool);
private:
    char *base;
    u_int32_t len;
    u_int32_t start_value;
};