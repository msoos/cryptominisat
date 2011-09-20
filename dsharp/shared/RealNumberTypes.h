#ifndef REALNUMBERTYPES_H
#define REALNUMBERTYPES_H



using namespace std;

#ifdef GMP_BIGNUM
#include <gmpxx.h>
typedef mpf_class CRealNum;
extern bool pow(mpf_class &res, const mpf_class &base, unsigned long int iExp);
extern bool pow2(mpf_class &res, unsigned long int iExp);
extern bool to_div_2exp(mpf_class &res, const mpf_class &op1, unsigned long int iExp);
extern double to_doubleT(const mpf_class &num);



#else
typedef long double CRealNum;

extern bool pow(CRealNum &res, CRealNum base, unsigned long int iExp);
extern bool pow2(CRealNum &res, unsigned long int iExp);
extern bool to_div_2exp(CRealNum &res, const CRealNum &op1, unsigned long int iExp);
extern long double to_doubleT(const CRealNum &num);
#endif

#endif
