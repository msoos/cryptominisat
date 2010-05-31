#include "Clause.h"

#ifdef USE_POOLS
#ifdef USE_4POOLS
boost::pool<> clausePoolQuad(sizeof(Clause) + 5*sizeof(Lit));
#endif //USE_4POOLS
boost::pool<> clausePoolTri(sizeof(Clause) + 3*sizeof(Lit));
boost::pool<> clausePoolBin(sizeof(Clause) + 2*sizeof(Lit));
#endif //USE_POOLS
