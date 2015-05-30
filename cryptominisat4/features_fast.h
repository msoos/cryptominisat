#ifndef __FEATURES_FAST_H__
#define __FEATURES_FAST_H__

#include <vector>
using std::vector;

namespace CMSat {

class Solver;

struct FeatureExtract {
public:
    FeatureExtract(const Solver* _solver) :
        solver(_solver) {
    }
    void fill_vars_cls();
    void extract();
    void print_stats() const;

private:
    const Solver* solver;
    template<class Function, class Function2>
    void for_one_clause(
        const Watched& cl
        , const Lit lit
        ,  Function func
        ,  Function2 func_each_lit
    ) const;
    template<class Function, class Function2>
    void for_all_clauses(Function func,  Function2 func_each_lit) const;
    struct VARIABLE {
        int numPos;
        int size;
        int horn;

        VARIABLE() : numPos(0), size(0), horn(0) {}
    };

    vector<VARIABLE> myVars;
    int numVars;
    int numClauses;
    int clause_id = 0;
    double unary = 0;
    double binary = 0;
    double trinary = 0;
    double horn = 0;

    double eps = 0.00001;
    double vcg_var_mean = 0;
    double vcg_var_std = 0;
    double vcg_var_min, vcg_var_max, vcg_var_spread;
    double vcg_cls_mean = 0;
    double vcg_cls_std = 0;
    double vcg_cls_min, vcg_cls_max, vcg_cls_spread;
    double pnr_var_mean = 0;
    double pnr_var_std = 0;
    double pnr_var_min, pnr_var_max, pnr_var_spread;
    double pnr_cls_mean = 0;
    double pnr_cls_std = 0;
    double pnr_cls_min, pnr_cls_max, pnr_cls_spread;
    double horn_mean = 0;
    double horn_std = 0;
    double horn_min, horn_max, horn_spread;
};

} //end namespace

#endif //__FEATURES_FAST_H__