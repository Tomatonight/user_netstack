#pragma once
#include "ip.h"
class icmp
{
public:
    char type;
    char code;
    uint16_t csum;
} __attribute__((packed));
class icmp_echo
{
public:
}__attribute__((packed));
class icmp_timestamp
{
public:
}__attribute__((packed));
class icmp_unreachable
{
public:
}__attribute__((packed));
