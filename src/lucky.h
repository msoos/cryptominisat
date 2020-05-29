/******************************************
Copyright (c) 2020, Mate Soos
Originally from CaDiCaL's "lucky.cpp" by Armin Biere, 2019

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

#ifndef LUCKY_PHASES_H_
#define LUCKY_PHASES_H_

namespace CMSat {

class Solver;

class Lucky
{
public:
    Lucky(Solver* solver);
    bool doit();

//private:
    bool check_all(bool polar);
    bool search_fwd_sat(bool polar);
    bool search_backw_sat(bool polar);
    bool horn_sat(bool polar);

private:
    bool enqueue_and_prop_assumptions();
    void set_polarities_to_enq_val();
    Solver* solver;
};

}

#endif
