#!/usr/bin/python3

import sys
import glob
import re

output_path = sys.argv[1]

def write_predictors(predictors, out, name):
    nums = []
    for pred in predictors:
        num = re.findall(r"all_predictors_{name}_conf([0-9]+).h".format(
            name=name), pred)
        if len(num) > 0:
            nums.append(int(num[0]))

    if len(nums) == 0:
        print("ERROR: Cannot calculate number of predictors '{name}'"
              .format(name=name))
        print("ERROR: Maybe you didn't generate the predictors?")
        exit(-1)

    out.write("""    should_keep_{name}_funcs.resize({num});\n""".format(
        num =max(nums)+1, name=name))

    out.write("""    should_keep_{name}_funcs_exists.resize({num}, 0);\n""".format(
        num =max(nums)+1, name=name))

    # set the correct value
    for num in sorted(nums):
        out.write("""    should_keep_{name}_funcs[{num}] = should_keep_{name}_conf{num}_funcs;\n"""
                  .format(num=num, name=name))
        out.write("""    should_keep_{name}_funcs_exists[{num}] = 1;\n"""
                  .format(num=num, name=name))


with open(output_path, 'w') as out:
    out.write("""/******************************************
Copyright (c) 2018, Mate Soos

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

//automated
""")
    predictors = glob.glob("all_predictors_*.h")
    for x in predictors:
        out.write('#include "predict/%s"\n' % x)

    out.write("""
#include "predict_func_type.h"
#include <vector>
using std::vector;

namespace CMSat {

vector<keep_func_type> should_keep_short_funcs;
vector<keep_func_type> should_keep_long_funcs;
vector<int> should_keep_short_funcs_exists;
vector<int> should_keep_long_funcs_exists;

void fill_pred_funcs() {
    //automated\n""")
    write_predictors(predictors, out, "short")
    out.write("""\n""")
    write_predictors(predictors, out, "long")
    out.write("""
}

//////////
//Function exists checks
//////////

bool short_pred_func_exists(size_t conf) {
    return (should_keep_short_funcs_exists.size() > conf
        && should_keep_short_funcs_exists[conf] == 1);
}

bool long_pred_func_exists(size_t conf) {
    return (should_keep_long_funcs_exists.size() > conf
        && should_keep_long_funcs_exists[conf] == 1);
}

//////////
//Function returns
//////////

const keep_func_type& get_short_pred_keep_funcs(size_t conf) {
    assert(short_pred_func_exists(conf));
    return should_keep_short_funcs[conf];
}

const keep_func_type& get_long_pred_keep_funcs(size_t conf) {
    assert(long_pred_func_exists(conf));
    return should_keep_long_funcs[conf];
}

}
""")
