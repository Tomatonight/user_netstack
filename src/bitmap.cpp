#include "bitmap.h"
u_int32_t bitmap::alloc_value()
{
    char *tmp = base;
    for (int i = 0; i < len; i++)
    {
        u_int8_t value = *tmp;
        if (value== 0xff)
        {
            tmp++;
            continue;
        }
        for (int j = 0; j < 8; j++)
        {
            if (!(value & (0x80 >> j)))
            {
                set_value(i * 8 + j + start_value, true);
                return i * 8 + j + start_value;
            }
        }

    }
    return -1;
}
void bitmap::free_value(u_int32_t value)
{
    if (value < start_value || value >= start_value + len * 8)
    {
        printf("bitmap free err\n");
    }
    set_value(value, 0);
}
void bitmap::set_value(uint32_t value, bool flag)
{
    
    uint32_t idx = value - start_value;
   // printf(" set %d %d %d %d\n",value,flag,idx,start_value);
    char *tmp = base + (idx) / 8;
    int index = idx % 8;
    if (flag)
        *tmp = (*tmp) | (0x80 >> index);
    else
        *tmp = (*tmp) & (~(0x80 >> index));
}
bool bitmap::test(uint32_t value)
{
    uint32_t idx = value - start_value;
    return (*(base + idx / 8)) & (0x80 >> (idx % 8));
}
