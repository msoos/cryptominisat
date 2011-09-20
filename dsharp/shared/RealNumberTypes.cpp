#include "RealNumberTypes.h"

#include <math.h>

#ifdef GMP_BIGNUM

const mpf_class mpf_TWO = 2.0;

bool pow(mpf_class &res, const mpf_class &base, unsigned long int iExp)
{
    mpf_pow_ui(res.get_mpf_t(),base.get_mpf_t(), iExp);
    return true;
}

bool pow2(mpf_class &res, unsigned long int iExp)
{
    mpf_class x(2.0,res.get_prec());
    // x= 2.0;
    mpf_pow_ui(res.get_mpf_t(),x.get_mpf_t(), iExp);
    return true;
}

bool to_div_2exp(mpf_class &res, const mpf_class &op1, unsigned long int iExp)
{
    mpf_div_2exp(res.get_mpf_t(),op1.get_mpf_t(),iExp);
    return true;
}

double to_doubleT(const mpf_class &num)
{
    return mpf_get_d(num.get_mpf_t());
}

#else

bool pow(CRealNum &res, double &base, unsigned long int iExp)
{
    res = pow(base, iExp);
    return true;
}

bool pow2(CRealNum &res, unsigned long int iExp)
{
    res = pow(2.00, iExp);
    return true;
}


bool to_div_2exp(CRealNum &res, const CRealNum &op1, unsigned long int iExp)
{
    res  = op1 / pow(2.00,iExp);
    return true;
}


long double to_doubleT(const CRealNum &num)
{
    return (long double) num;
}
#endif
