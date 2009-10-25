#include "my_row.h"

std::ostream& operator << (std::ostream& os, const my_row& m)
{
    for(uint i = 0; i < m.size; i++) {
        if (m.mp[i]) os << i+1 << " ";
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
    
    return (xor_clause_inverted == b.xor_clause_inverted && popcount == b.popcount && std::equal(b.mp, b.mp+size, mp));
}

bool my_row::operator !=(const my_row& b) const
{
    #ifdef DEBUG_ROW
    assert(size > 0);
    assert(b.size > 0);
    assert(size == b.size);
    #endif
    
    return (xor_clause_inverted != b.xor_clause_inverted || popcount != b.popcount || !std::equal(b.mp, b.mp+size, mp));
}

my_row& my_row::operator=(const my_row& b)
{
    #ifdef DEBUG_ROW
    assert(size > 0);
    assert(b.size > 0);
    assert(size == b.size);
    #endif
    
    const uint64_t*  mp2 = (const uint64_t*)b.mp;
    uint64_t*  mp3 = (uint64_t*)mp;
    
    for (uint i = 0; i < size/8; i++) {
        mp3[i] = mp2[i];
    }
    xor_clause_inverted = b.xor_clause_inverted;
    popcount = b.popcount;
    return *this;
}

my_row& my_row::operator^=(const my_row& b)
{
    #ifdef DEBUG_ROW
    assert(size > 0);
    assert(b.size > 0);
    assert(b.size == size);
    #endif
    
    popcount = 0;
    for(uint i = 0; i < size; i++) {
        mp[i] ^= b.mp[i];
        popcount += mp[i];
    }
    xor_clause_inverted ^= !b.xor_clause_inverted;
    return *this;
}
