#include <stdio.h>
#include <stdlib.h>

#include <set>

#include "list.h"
#include "skiplist.h"
#include "randomer/randomer.h"

int main(int argc, char* argv[])
{
    (void)argc;
    
    Randomer randomer;
    SkipList<int, int, 8> a;
    int nums = atoi(argv[1]);
    Printf("nums:%d\n", nums);
    for (int i = 0; i < nums; i++)
    {
        auto key = randomer.Rand(100);
        auto value = randomer.Rand(100);
        Printf("index:%d key:%d, value:%d\n", i, key, value);
        a.Put(key, value);
    }
    
    a.DisplaySpecifiedHeight<3>();
    a.DisplaySpecifiedHeight<2>();
    a.DisplaySpecifiedHeight<1>();
    a.DisplaySpecifiedHeight<0>();
}
