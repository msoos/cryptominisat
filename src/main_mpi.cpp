/******************************************
Copyright (C) 2020 Authors of CryptoMiniSat, see AUTHORS file

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

#include <iostream>
#include "mpi.h"
#include "datasyncserver.h"
#include "cryptominisat5/cryptominisat.h"
#include "solverconf.h"
#include "zlib.h"
#include "dimacsparser.h"


using std::cout;
using std::endl;

int solve()
{
    int err, mpiRank, mpiSize;
    err = MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    assert(err == MPI_SUCCESS);
    err = MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    assert(err == MPI_SUCCESS);
    CMSat::SolverConf conf;
    conf.verbosity = 1;
    conf.is_mpi = true;
    conf.do_bva = false;

    if (mpiSize > 1 && mpiRank > 1) {
        conf.origSeed = mpiRank;
        if (mpiRank % 6 == 3) {
            conf.polarity_mode = CMSat::PolarityMode::polarmode_pos;
            conf.restartType = CMSat::Restart::geom;
        }
        if (mpiRank % 6 == 4) {
            conf.polarity_mode = CMSat::PolarityMode::polarmode_neg;
            conf.restartType = CMSat::Restart::glue;
        }
    }

    CMSat::SATSolver solver(&conf);
    solver.set_num_threads(2);

    //Receive the num variables, and all the claues
    Lit data[1024];
    vector<Lit> clause;
    uint32_t num_msgs = 0;
    uint32_t num_clauses = 0;
    bool done = false;

    cout << "c created solver " << mpiRank << " reading in file..." << endl;
    while(!done) {
        MPI_Bcast(&data, 1024, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
        cout << "c solver " << mpiRank << " got file msg " << num_msgs << endl;

        uint32_t i = 0;
        if (num_msgs == 0) {
            solver.new_vars(data[0].var());
            cout << "We were told there are " << solver.nVars() << " variables" << endl;
            i++;
        }
        num_msgs++;

        for(; i < 1024; i ++) {
            if (data[i] == lit_Error) {
                done = true;
                break;
            } else if (data[i] == lit_Undef) {
                solver.add_clause(clause);
                num_clauses++;
                clause.clear();
            } else {
                assert(data[i].var() < solver.nVars());
                clause.push_back(data[i]);
            }
        }
    }
    cout << "c Solver " << mpiRank << " finished getting all of the file."
    << " nvars: " << solver.nVars()
    << " num_clauses: " << num_clauses << endl;

    lbool ret = solver.solve();
    return 0;
}


int main(int argc, char** argv)
{
    int ret;
    int err;
    err = MPI_Init(&argc, &argv);
    assert(err == MPI_SUCCESS);

    int mpiRank, mpiSize;
    err = MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    assert(err == MPI_SUCCESS);

    err = MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    assert(err == MPI_SUCCESS);

    if (mpiSize <= 1) {
        cout << "ERROR, you must run on at least 2 MPI nodes" << endl;
        exit(-1);
    }

    assert(argc == 2);


    if (mpiRank == 0) {
        std::string filename(argv[1]);
        cout << "c Filename is: " << filename << endl;

        CMSat::DataSyncServer server;
        gzFile in = gzopen(filename.c_str(), "rb");
        DimacsParser<StreamBuffer<gzFile, GZ>, CMSat::DataSyncServer> parser(&server, NULL, 0);
        if (in == NULL) {
            std::cerr
            << "ERROR! Could not open file '"
            << filename
            << "' for reading: " << strerror(errno) << endl;

            std::exit(1);
        }

        bool strict_header = false;
        if (!parser.parse_DIMACS(in, strict_header)) {
            exit(-1);
        }
        gzclose(in);

        cout << "c read in file" << endl;;

        server.send_cnf_to_solvers();

        ret = server.actAsServer();
        if (ret == 0) {
            cout << "s UNSATISFIABLE" << endl;
        } else {
            cout << "s SATISFIABLE" << endl;
            server.print_solution();
        }
    } else {
        solve();
    }

    err = MPI_Finalize();
    assert(err == MPI_SUCCESS);
}
