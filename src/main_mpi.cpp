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
#include <unistd.h>
#include <mpi.h>
#include "datasyncserver.h"
#include "cryptominisat.h"
#include "solverconf.h"
#include "zlib.h"
#include "dimacsparser.h"


using std::cout;
using std::endl;


int num_threads = 2;

vector<lbool> solve(lbool& solution_val)
{
    int err, mpiRank, mpiSize;
    err = MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    assert(err == MPI_SUCCESS);
    err = MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    assert(err == MPI_SUCCESS);
    CMSat::SolverConf conf;
    conf.verbosity = 0; //(mpiRank == 1);
    conf.is_mpi = true;
    conf.do_bva = false;

    if (mpiSize > 1 && mpiRank > 1) {
        conf.origSeed = mpiRank*2000; //this will be added T that is the thread number within the MPI
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
    solver.set_num_threads(num_threads);

    //Receive the num variables, and all the claues
    Lit data[1024];
    vector<Lit> clause;
    uint32_t num_msgs = 0;
    uint32_t num_clauses = 0;
    bool done = false;


    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    cout << "c created solver " << mpiRank << " reading in file..." << endl;
    #endif
    while(!done) {
        MPI_Bcast(&data, 1024, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
        //cout << "c solver " << mpiRank << " got file msg " << num_msgs << endl;

        uint32_t i = 0;
        if (num_msgs == 0) {
            solver.new_vars(data[0].var());
            #ifdef VERBOSE_DEBUG_MPI_SENDRCV
            cout << "c Solver " << mpiRank
            << " was told by MPI there are " << solver.nVars() << " variables" << endl;
            #endif
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
    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    cout << "c Solver " << mpiRank
    << " finished getting all of the file."
    << " nvars: " << solver.nVars()
    << " num_clauses: " << num_clauses << endl;
    #endif

    solution_val = solver.solve();
    vector<lbool> model;
    if (solution_val == l_True) {
        model = solver.get_model();
    }
    return model;
}


int main(int argc, char** argv)
{
    int err;
    err = MPI_Init(&argc, &argv);
    assert(err == MPI_SUCCESS);

    int mpiRank, mpiSize;
    err = MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    assert(err == MPI_SUCCESS);

    err = MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    assert(err == MPI_SUCCESS);

    if (mpiSize <= 1) {
        cout << "ERROR: you must run on at least 2 MPI nodes" << endl;
        cout << "NOTE: If using mpirun, use: mpirun -c NUM_PROCESSES ./cryptominisat5_mpi FILENAME NUM_THREADS" << endl;
        exit(-1);
    }

    if (argc != 3) {
        cout << "ERROR: You MUST give 2 position parameters: FILENAME and NUM_THREADS" << endl;
        exit(-1);
    }

    assert(argc == 3);
    for(uint32_t i = 0; i < strlen(argv[2]); i++) {
        if (argv[2][i]-'0' < 0 || argv[2][i]-'0' > '9') {
            cout << "ERROR: You MUST give a thread number that's an integer!" << endl;
            exit(-1);
        }
    }
    num_threads = atoi(argv[2]);
    if (num_threads < 2) {
        cout << "ERROR: you must have at least 2 threads per MPI node" << endl;
        exit(-1);
    }


    if (mpiRank == 0) {
        std::string filename(argv[1]);
        cout << "c Filename is: " << filename << endl;
        cout << "c num threads used: " << num_threads << endl;

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

        lbool sol = server.actAsServer();
        if (sol == l_True) {
            cout << "s SATISFIABLE" << endl;
            server.print_solution();
        } else if (sol == l_False) {
            cout << "s UNSATISFIABLE" << endl;
        } else {
            assert(false);
        }
    } else {
        lbool solution_val;
        const vector<lbool> model = solve(solution_val);
        #ifdef VERBOSE_DEBUG_MPI_SENDRCV
        cout << "c --> MPI Slave Rank " << mpiRank
        << " Solved " <<  << " with value: " << solution_val << std::endl;
        #endif

        if (solution_val != l_Undef) {
            //Send tag 1 to 0 that indicates we solved
            MPI_Request req;
            vector<int> solution_dat;
            solution_dat.push_back(toInt(solution_val));
            if (solution_val == l_True) {
                solution_dat.push_back(model.size());
                for(uint32_t i = 0; i < model.size(); i++) {
                    solution_dat.push_back(toInt(model[i]));
                }
            }

            err = MPI_Isend(solution_dat.data(), solution_dat.size(), MPI_UNSIGNED, 0, 1, MPI_COMM_WORLD, &req);
            assert(err == MPI_SUCCESS);
            #ifdef VERBOSE_DEBUG_MPI_SENDRCV
            cout << "c --> MPI Slave Rank " << mpiRank
            << " sent tag 1 to master to indicate finished" << std::endl;
            #endif

            //Either we should we get an acknowledgement of receipt, or we get an interrupt
            int flag;
            MPI_Status status;
            while(true) {
                err = MPI_Iprobe(0, 1, MPI_COMM_WORLD, &flag, &status);
                assert(err == MPI_SUCCESS);

                //We received an interrupt, let's cancel the send and exit
                if (flag == true) {
                    unsigned buf;
                    err = MPI_Recv(&buf, 0, MPI_UNSIGNED, 0, 1, MPI_COMM_WORLD, &status);
                    assert(err == MPI_SUCCESS);
                    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
                    cout << "c --> MPI Slave Rank " << mpiRank
                    << " got tag 1 from master. Let's cancel our send & exit." << std::endl;
                    #endif

                    err = MPI_Cancel(&req);
                    assert(err == MPI_SUCCESS);
                    break;
                }

                int op_completed;
                err = MPI_Test(&req, &op_completed, &status);
                assert(err == MPI_SUCCESS);

                //OK, server got our message, we can exit
                if (op_completed) {
                    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
                    cout << "c --> MPI Slave Rank " << mpiRank
                    << " completed sending solution & tag 1 to master. Let's exit." << std::endl;
                    #endif
                    break;
                }

                usleep(50);
            }
        }
    }

    err = MPI_Finalize();
    assert(err == MPI_SUCCESS);
    return 0;
}
