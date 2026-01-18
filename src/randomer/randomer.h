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

    uint32_t Rand()
    {
        return rand_r(&mSeed);
    }

    uint32_t Rand(uint32_t range)
    {
        return rand_r(&mSeed) % range;
    }
};

#endif /* RANDOMER_H */
