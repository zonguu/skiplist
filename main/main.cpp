#include <stdio.h>
#include <stdlib.h>

#include <set>

#include "list.h"
#include "skiplist.h"
#include "randomer/randomer.h"

int main(int args, char* argv[])
{
    Randomer randomer;
    SkipList<int, int, 8> a;
    int nums = atoi(argv[1]);
    printf("nums:%d\n", nums);
    for (int i = 0; i < nums; i++)
    {
        printf("i:%d\n", i);
        a.Put1(randomer.Rand(100), randomer.Rand(100));
        a.DisplaySpecifiedHeight<0>();
        // a.DisplayReverse();
    }
    // a.Put(3, 2);
    // a.Put(1, 1);
    a.DisplaySpecifiedHeight<0>();
    // struct SkipList *a = new SkipList;
    // printf("%x %x\n", a, GET_LIST(a, struct SkipList, head));
}
