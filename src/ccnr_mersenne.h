/******************************************
Copyright (c) 2019, Shaowei Cai

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

#ifndef CCNR_MERSENNE_H
#define CCNR_MERSENNE_H

namespace CCNR {

class Mersenne
{
    static const int N = 624;
    unsigned int mt[N];
    int mti;
    const int M = 397;
    const unsigned int MATRIX_A = 0x9908b0dfUL;
    const unsigned int UPPER_MASK = 0x80000000UL;
    const unsigned int LOWER_MASK = 0x7fffffffUL;

   public:
    Mersenne();         // seed with time-dependent value
    Mersenne(int seed); // seed with int value; see comments for the seed() method
    Mersenne(unsigned int *array, int count); // seed with array
    Mersenne(const Mersenne &copy);
    Mersenne &operator=(const Mersenne &copy);
    void seed(int s);
    void seed(unsigned int *array, int len);
    unsigned int next32(); // generates random integer in [0..2^32-1]
    int next31();          // generates random integer in [0..2^31-1]
    double nextClosed();   // generates random float in [0..1], 2^53 possible values
    double nextHalfOpen(); // generates random float in [0..1), 2^53 possible values
    double nextOpen();     // generates random float in (0..1), 2^53 possible values
    int next(int bound);   // generates random integer in [0..bound), bound < 2^31
};

//---------------------------
//functions in mersenne.h & mersenne.cpp

/*
  Notes on seeding

  1. Seeding with an integer
     To avoid different seeds mapping to the same sequence, follow one of
     the following two conventions:
     a) Only use seeds in 0..2^31-1     (preferred)
     b) Only use seeds in -2^30..2^30-1 (2-complement machines only)

  2. Seeding with an array (die-hard seed method)
     The length of the array, len, can be arbitrarily high, but for lengths greater
     than N, collisions are common. If the seed is of high quality, using more than
     N values does not make sense.
*/
inline Mersenne::Mersenne()
{
    seed(0);
}
inline Mersenne::Mersenne(int s)
{
    seed(s);
}
inline Mersenne::Mersenne(unsigned int *init_key, int key_length)
{
    seed(init_key, key_length);
}
inline Mersenne::Mersenne(const Mersenne &copy)
{
    *this = copy;
}
inline Mersenne &Mersenne::operator=(const Mersenne &copy)
{
    for (int i = 0; i < N; i++)
        mt[i] = copy.mt[i];
    mti = copy.mti;
    return *this;
}
inline void Mersenne::seed(int se)
{
    unsigned int s = ((unsigned int)(se << 1)) + 1;
    // Seeds should not be zero. Other possible solutions (such as s |= 1)
    // lead to more confusion, because often-used low seeds like 2 and 3 would
    // be identical. This leads to collisions only for rarely used seeds (see
    // note in header file).
    mt[0] = s & 0xffffffffUL;
    for (mti = 1; mti < N; mti++) {
        mt[mti] = (1812433253UL * (mt[mti - 1] ^ (mt[mti - 1] >> 30)) + mti);
        mt[mti] &= 0xffffffffUL;
    }
}
inline void Mersenne::seed(unsigned int *init_key, int key_length)
{
    int i = 1, j = 0, k = (N > key_length ? N : key_length);
    seed(19650218UL);
    for (; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 30)) * 1664525UL)) + init_key[j] + j;
        mt[i] &= 0xffffffffUL;
        i++;
        j++;
        if (i >= N) {
            mt[0] = mt[N - 1];
            i = 1;
        }
        if (j >= key_length)
            j = 0;
    }
    for (k = N - 1; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 30)) * 1566083941UL)) - i;
        mt[i] &= 0xffffffffUL;
        i++;
        if (i >= N) {
            mt[0] = mt[N - 1];
            i = 1;
        }
    }
    mt[0] = 0x80000000UL;
}
inline unsigned int Mersenne::next32()
{
    unsigned int y;
    static unsigned int mag01[2] = {0x0UL, MATRIX_A};
    if (mti >= N) {
        int kk;
        for (kk = 0; kk < N - M; kk++) {
            y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
            mt[kk] = mt[kk + M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        for (; kk < N - 1; kk++) {
            y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
            mt[kk] = mt[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        y = (mt[N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
        mt[N - 1] = mt[M - 1] ^ (y >> 1) ^ mag01[y & 0x1UL];
        mti = 0;
    }
    y = mt[mti++];
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);
    return y;
}
inline int Mersenne::next31()
{
    return (int)(next32() >> 1);
}
inline double Mersenne::nextClosed()
{
    unsigned int a = next32() >> 5, b = next32() >> 6;
    return (a * 67108864.0 + b) * (1.0 / 9007199254740991.0);
}
inline double Mersenne::nextHalfOpen()
{
    unsigned int a = next32() >> 5, b = next32() >> 6;
    return (a * 67108864.0 + b) * (1.0 / 9007199254740992.0);
}
inline double Mersenne::nextOpen()
{
    unsigned int a = next32() >> 5, b = next32() >> 6;
    return (0.5 + a * 67108864.0 + b) * (1.0 / 9007199254740991.0);
}
inline int Mersenne::next(int bound)
{
    unsigned int value;
    do {
        value = next31();
    } while (value + (unsigned int)bound >= 0x80000000UL);
    // Just using modulo doesn't lead to uniform distribution. This does.
    return (int)(value % bound);
}

}

#endif
