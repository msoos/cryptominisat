/******************************************
Copyright (c) 2019, Alex Ozdemir

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

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cryptominisat5/cryptominisat.h>

using namespace std;

int main(int argc, char **argv) {
  std::vector<std::string> args(argv, argv + argc);
  assert(args.size() == 3);
  const auto &dimacs = args[1];
  const auto &drat = args[2];

  ifstream dimacs_stream{dimacs};

  std::ofstream *proof_stream = new std::ofstream;
  proof_stream->open(drat.c_str(), std::ofstream::out | std::ofstream::binary);

  CMSat::SATSolver solver{};

  std::string p, cnf;
  uint64_t n_vars, n_clauses;
  dimacs_stream >> p >> cnf >> n_vars >> n_clauses;
  assert(p == "p");
  assert(cnf == "cnf");
  solver.set_drat(proof_stream, false);
  solver.new_vars(n_vars);
  solver.set_verbosity(1);

  for (size_t c_i = 0; c_i < n_clauses; c_i++) {
    std::vector<CMSat::Lit> clause;
    int64_t i;
    while (true) {
      dimacs_stream >> i;
      assert(dimacs_stream.good());
      if (i == 0) {
        break;
      } else {
        uint32_t var = static_cast<uint32_t>(abs(i));
        assert(var <= n_vars);
        CMSat::Lit l{var - 1, i < 0};
        clause.push_back(l);
      }
    }
    if (not solver.add_clause(clause)) {
      std::cout << "UNSAT" << std::endl;
      exit(-1);
    }
  }
  std::cerr << "Clauses added" << std::endl;

  CMSat::lbool res = solver.solve();

  assert(proof_stream->good());
  *proof_stream << std::flush;
  delete proof_stream;

  solver.print_stats();
  std::cerr << "Res: " << res << std::endl;
  return 0;
}
