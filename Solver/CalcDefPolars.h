#ifndef CALCDEFAULTPOLARITIES__H
#define CALCDEFAULTPOLARITIES__H

#include "Vec.h"
#include "Vec2.h"
#include "Clause.h"
#include "Solver.h"

class CalcDefPolars
{
    public:
        CalcDefPolars(Solver& solver);
        void calculate();

    private:
        void tallyVotes(const vec<Clause*>& cs, vec<double>& votes) const;
        void tallyVotes(const vec<XorClause*>& cs, vec<double>& votes) const;
        void tallyVotesBin(vec<double>& votes, const vec<vec2<Watched> >& watched) const;

        Solver& solver;
};

#endif //CALCDEFAULTPOLARITIES__H
