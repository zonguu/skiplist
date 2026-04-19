#ifndef RANDOMER_H

#define RANDOMER_H

#include <stdlib.h>
#include <stddef.h>

class Randomer
{
private:
    /* data */
    unsigned int mSeed;

public:
    Randomer()
    {
        mSeed = time(NULL);
        // srand(mSeed);
    }

    ~Randomer() {}

    int Rand()
    {
        return rand_r(&mSeed);
    }

    int Rand(int range)
    {
        return rand_r(&mSeed) % range;
    }
};

#endif /* RANDOMER_H */
