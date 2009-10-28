#include "my_row.h"

std::ostream& operator << (std::ostream& os, const my_row& m)
{
    for(uint i = 0; i < m.size; i++) {
        if (m[i]) os << i+1 << " ";
    }
    return os;
}

bool my_row::operator ==(const my_row& b) const
{
    #ifdef DEBUG_ROW
    assert(size > 0);
    assert(b.size > 0);
    assert(size == b.size);
    #endif
    
    return (xor_clause_inverted == b.xor_clause_inverted && std::equal(b.mp, b.mp+size, mp));
}

bool my_row::operator !=(const my_row& b) const
{
    #ifdef DEBUG_ROW
    assert(size > 0);
    assert(b.size > 0);
    assert(size == b.size);
    #endif
    
    return (xor_clause_inverted != b.xor_clause_inverted || !std::equal(b.mp, b.mp+size, mp));
}

bool my_row::popcnt_is_one() const
{
    char popcount = 0;
    for (uint i = 0; i < size; i++) {
        uint64_t tmp = mp[i];
        for (uint i2 = 0; i2 < 64; i2++) {
            popcount += tmp & 1;
            if (popcount > 1) return false;
            tmp >>= 1;
        }
    }
    return popcount;
}

bool my_row::popcnt_is_one(uint from) const
{
    from++;
    for (uint i = from/64; i < size; i++) {
        uint64_t tmp = mp[i];
        uint i2;
        if (i == from/64) {
            i2 = from%64;
            tmp >>= i2;
        } else
            i2 = 0;
        for (; i2 < 64; i2++) {
            if (tmp & 1) return false;
            tmp >>= 1;
        }
    }
    return true;
}

my_row& my_row::operator=(const my_row& b)
{
    #ifdef DEBUG_ROW
    assert(size > 0);
    assert(b.size > 0);
    assert(size == b.size);
    #endif
    
    std::copy(b.mp, b.mp+size, mp);
    xor_clause_inverted = b.xor_clause_inverted;
    return *this;
}

my_row& my_row::operator^=(const my_row& b)
{
    #ifdef DEBUG_ROW
    assert(size > 0);
    assert(b.size > 0);
    assert(b.size == size);
    #endif
    
    for (uint i = 0; i < size; i++) {
        mp[i] ^= b.mp[i];
    }
    xor_clause_inverted ^= !b.xor_clause_inverted;
    return *this;
}
