/*****************************************************************************
cnf fuzzer -- Copyright (c) 2011 Vegard Nossum

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "MersenneTwister.h"

#include <vector>

class clause {
public:
    bool is_xor;
    std::vector<int> literals;

    clause()
    {
    }
};

static MTRand rnd;

static unsigned int randNorm(double mean, double variance)
{
    double r = rnd.randNorm(mean, variance);
    if (r < 0)
        return 0;
    return r;
}

static std::vector<clause> clauses;
bool need_regular_clauses = true;

int main(int, char **)
{
    unsigned int nr_variables = randNorm(120, 50);

    unsigned int nr_constraint_types = 2 + rnd.randInt(15);
    for (unsigned int i = 0; i < nr_constraint_types; ++i) {
        bool is_xor = (rnd.randInt(15) == 1);
        unsigned nr_literals = 3 + ((rnd.randInt(4) == 1) ? rnd.randInt(40) : rnd.randInt(3));
        //printf("c literal num: %u\n", nr_literals);
        unsigned int offsets[nr_literals];
        bool polarities[nr_literals];

        //Set offsets and polarities
        for (unsigned int j = 0; j < nr_literals; ++j) {
            offsets[j] = rnd.randInt();
            polarities[j] = rnd.randInt(2);
        }
        unsigned int stride = 1 + rnd.randInt(3);

        for (unsigned int j = 0; j < nr_variables; j += stride) {
            clause c;
            c.is_xor = is_xor;

            //Vary the size of the clause
            if (j % 3 == 1) {
                nr_literals -= (int)rnd.randInt(2) - 1;
                if (nr_literals < 1) nr_literals = 3;
            }
            if (c.is_xor) nr_literals = std::min<unsigned>(nr_literals, 6);

            for (unsigned int k = 0; k < nr_literals; ++k) {
                int lit = (polarities[k] ? -1 : 1) * (1 + ((j + offsets[k]) % nr_variables));
                c.literals.push_back(lit);
                //printf("literal at this point: %d\n", lit);
            }
            //printf("clause end\n");

            if (is_xor && need_regular_clauses) {
                for (unsigned int k = 0; k < (1 << nr_literals); ++k) {
                    if (__builtin_parity(k))
                        continue;

                    clause nc;
                    nc.is_xor = false;

                    for (unsigned int l = 0; l < c.literals.size(); ++l)
                        nc.literals.push_back(((k & (1 << l)) ? -1 : 1) * c.literals[l]);

                    clauses.push_back(nc);
                }
            } else {
                clauses.push_back(c);
            }
        }
    }

    printf("p cnf %u %lu\n", nr_variables, clauses.size());
    for (unsigned int i = 0; i < clauses.size(); ++i) {
        clause &c = clauses[i];

        if (c.is_xor)
            printf("x ");

        for (unsigned int k = 0; k < c.literals.size(); ++k)
            printf("%d ", c.literals[k]);

        printf("0\n");
    }

    return 0;
}
