#include "my_row.h"

std::ostream& operator << (std::ostream& os, const my_row& m)
{
    for(uint i = 0; i < m.mp.size(); i++) {
        if (m.mp[i]) os << i+1 << " ";
    }
    return os;
}
