/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

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
#include <unistd.h>

#include "datasyncserver.h"
#include "solvertypes.h"
using std::vector;

#define VERBOSE_DEBUG_MPI_SENDRCV

using namespace CMSat;

DataSyncServer::DataSyncServer()
{
    int err;
    err = MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    assert(err == MPI_SUCCESS);

    sendRequests.resize(mpiSize);
    sendRequestsFinished.resize(mpiSize, true);
    interruptRequests.resize(mpiSize);

    int mpiRank;
    err = MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    assert(err == MPI_SUCCESS);
    assert(mpiRank == 0);

    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "c -->> MPI Server"
    << " says -- mpiSize:" << mpiSize << std::endl;
    #endif
    assert(sizeof(unsigned) == sizeof(uint32_t));
}

DataSyncServer::~DataSyncServer()
{
    delete[] sendData;
}

//Tag of messsage "0"
void DataSyncServer::mpi_recv_from_others()
{
    int err;
    MPI_Status status;
    int flag;
    int count;
    uint32_t thisRecvBinData = 0;

    //Check for message
    err = MPI_Iprobe(
        MPI_ANY_SOURCE, //from anyone (i.e. clients)
        0, //tag 0, i.e. unit/bin data
        MPI_COMM_WORLD, &flag, &status);
    assert(err == MPI_SUCCESS);
    if (flag == false) {
        return;
    }
    int source = status.MPI_SOURCE;

    //Get message size
    err = MPI_Get_count(&status, MPI_UNSIGNED, &count);
    assert(err == MPI_SUCCESS);
    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "c -->> MPI Server [from " << source << "]"
    << " Counted " << count << " uint32_t-s" << std::endl;
    #endif

    //Get message, BLOCKING
    assert(sizeof(unsigned int) == 4);
    uint32_t* buf = new uint32_t[count];
    err = MPI_Recv((unsigned*)buf, count, MPI_UNSIGNED,
                   source,
                   0, //tag "0", i.e. unit/bin data
                   MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    assert(err == MPI_SUCCESS);

    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "c -->> MPI Server [from " << source << "]"
    << " Received data " << std::endl;
    #endif

    int at = 0;
    assert(num_vars == buf[at]); //the first uint32_t is the number of bytes, always

    //Sync all units, starts with num_vars
    assert(num_vars == buf[at]);
    at++;
    for (uint32_t var = 0; var < num_vars; var++, at++) {
        lbool val = toLbool(buf[at]);
        if (value[var] == l_Undef) {
            if (val != l_Undef){
                value[var] = val;
            }
        } else if (val != l_Undef && value[var] != val) {
            //This will cause UNSAT real fast
            value[var] = val;
        }
    }

    //Sync all binaries, starts with num_vars*2
    //for each Lit, there is SIZE of elements that follow, then the elements
    assert(buf[at] == num_vars*2);
    at++;
    for (uint32_t wsLit = 0; wsLit < num_vars*2; wsLit++) {
        Lit lit = ~Lit::toLit(wsLit);
        uint32_t num = buf[at];
        at++;
        for (uint32_t i = 0; i < num; i++, at++) {
            Lit otherLit = Lit::toLit(buf[at]);
            get_bin(lit, otherLit);
            thisRecvBinData++;
        }
    }
    recvBinData += thisRecvBinData;

    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "c -->> MPI Server [from " << source << "]"
    << " Obtained " << thisRecvBinData << " bins" << std::endl;
    #endif

    delete[] buf;
    numGotPacket++;
}

void DataSyncServer::get_bin(const Lit lit1, const Lit lit2)
{
    assert(lit1 < lit2);

    //check if binary is already known
    vector<Lit>& thisBins = bins[(~lit1).toInt()];
    for (vector<Lit>::const_iterator it = thisBins.begin(), end = thisBins.end(); it != end; ++it) {
        if (*it == lit2) {
            return;
        }
    }

    //binary not already known
    thisBins.push_back(lit2);
}

void DataSyncServer::finish_data_send()
{
    if (sendData == NULL) {
        assert(send_requests_finished == true);
        return;
    }

    int err;
    int numFinished = 0;
    for (int i = 1; i < mpiSize; i++) {
        if (sendRequestsFinished[i]) {
            numFinished++;
            continue;
        }
//      #ifdef VERBOSE_DEBUG_MPI_SENDRCV
//      std::cout << "c -->> MPI Server"
//      << " Checking if sending finished to " << i << std::endl;
//      #endif

        MPI_Status status;
        int op_completed;
        err = MPI_Test(&(sendRequests[i]), &op_completed, &status);
        assert(err == MPI_SUCCESS);
        if (op_completed) {
            //NOTE: no need to free, MPI_Test also frees it
            sendRequestsFinished[i] = true;
            numFinished++;
            #ifdef VERBOSE_DEBUG_MPI_SENDRCV
            std::cout << "c -->> MPI Server [to:" << i << "]"
            << " Sending finished" << std::endl;
            #endif
        } else if (interrupt_sent) {
            //If we have finished then we must cancel this otherwise we may hang
            //   waiting for a receive of a thread that itself has already found SAT/UNSAT on its own

            //NOTE: "It is still necessary to complete a communication that has been marked for cancellation, using a call to MPI_REQUEST_FREE, MPI_WAIT or MPI_TEST (or any of the derived operations)."
            //      --> So we cannot set sendRequesFinished -- we must still test!

            err = MPI_Cancel(&(sendRequests[i]));
            assert(err == MPI_SUCCESS);
            sendRequestsFinished[i] = true;
            numFinished++;
            #ifdef VERBOSE_DEBUG_MPI_SENDRCV
            std::cout << "c -->> MPI Server [to:" << i << "]"
            << " cancelling due to interrupt" << std::endl;
            #endif
        }
    }
    if (numFinished != mpiSize-1) {
//      #ifdef VERBOSE_DEBUG_MPI_SENDRCV
//      std::cout << "c -->> MPI Server"
//      << " sending not all finished, exiting sendDataToAll" << std::endl;
//      #endif
        return;
    }
    send_requests_finished = true;
    delete[] sendData;
    sendData = NULL;
}

void DataSyncServer::sendDataToAll()
{
    int err;
    //If interrupt already sent, we just waited for things to be received
    if (interrupt_sent) {
        return;
    }

    //Set up units. First, the num_vars, then the values
    uint32_t thisSentBinData = 0;
    vector<uint32_t> data;
    data.push_back((uint32_t)num_vars);
    for (uint32_t var = 0; var < num_vars; var++) {
        data.push_back(toInt(value[var]));
    }

    //Binaries. First, the 2*num_vars, then the list sizes, then the data.
    data.push_back((uint32_t)num_vars*2);
    uint32_t at = 0;
    for(vector<vector<Lit> >::const_iterator it = bins.begin(), end = bins.end(); it != end; ++it, at++) {
        const vector<Lit>& binSet = *it;
        assert(binSet.size() >= syncMPIFinish[at]);
        uint32_t sizeToSend = binSet.size() - syncMPIFinish[at];
        data.push_back(sizeToSend);
        for (uint32_t i = syncMPIFinish[at]; i < binSet.size(); i++) {
            data.push_back(binSet[i].toInt());
            thisSentBinData++;
        }
        syncMPIFinish[at] = binSet.size();
    }
    sentBinData += thisSentBinData;

    //Send data as tag 0
    sendData = new uint32_t[data.size()];
    std::copy(data.begin(), data.end(), sendData);
    for (int i = 1; i < mpiSize; i++) {
        err = MPI_Isend(sendData, data.size(), MPI_UNSIGNED, i, 0, MPI_COMM_WORLD, &(sendRequests[i]));
        assert(err == MPI_SUCCESS);
        #ifdef VERBOSE_DEBUG_MPI_SENDRCV
        std::cout << "c -->> MPI Server [to " << i << "]"
        << " Sent " << data.size() << " uint32_t -s" << std::endl;
        std::cout << "c -->> MPI Server [to " << i << "]"
        << " Sent " << thisSentBinData << " bins " << std::endl;
        #endif
        sendRequestsFinished[i] = false;
    }
    lastSendNumGotPacket = numGotPacket;
    send_requests_finished = false;
}


//Tag of message "1"
bool DataSyncServer::check_interrupt_and_forward_to_all()
{
    int err;
    MPI_Status status;
    int flag;

    //Check for interrupt data
    err = MPI_Iprobe(
        MPI_ANY_SOURCE,
        1, //check tag "1", i.e. interrupt tag data
        MPI_COMM_WORLD, &flag, &status);
    int source = status.MPI_SOURCE;
    assert(err == MPI_SUCCESS);
    if (flag == false) {
        return false;
    }

    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "c -->> MPI Server"
    << " Got interrupt from " << source << std::endl;
    #endif

    int count;
    err = MPI_Get_count(&status, MPI_UNSIGNED, &count);
    assert(err == MPI_SUCCESS);

    #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "c -->> MPI Server"
    << " Interrupt from " << source << " has size " << count << std::endl;
    #endif

    //Get the interrupt signal, tagged 1, with the solution. BLOCKING.
    uint32_t* buf = new uint32_t[count];
    err = MPI_Recv(buf, count, MPI_UNSIGNED,
                   source, //coming from source
                   1, //tag "1", i.e. interrupt
                   MPI_COMM_WORLD, &status);
    assert(err == MPI_SUCCESS);

    int at = 0;
    solution_val = toLbool(buf[at++]);
    if (solution_val == l_True) {
        model.resize(buf[at++]);
        assert((int)model.size() == count-2);
        for(uint32_t i = 0; i < model.size(); i++) {
            int val = buf[at++];
            model[i] = toLbool(val);
        }
    }
    delete[] buf;

//     #ifdef VERBOSE_DEBUG_MPI_SENDRCV
    std::cout << "c -->> MPI Server"
    << " got solution from " << source << std::endl;
//     #endif

    //Send to all except: the one who sent it (source) and ourselves (0)
    for (int i = 1; i < mpiSize; i++) {
        if (i == source) {
            continue;
        }
//         #ifdef VERBOSE_DEBUG_MPI_SENDRCV
        std::cout << "c -->> MPI Server"
        << " sending interrupt to " << i << std::endl;
//         #endif

        err = MPI_Isend(
            NULL, // buf is actually empty that we send
            0, // sending 0 bytes
            MPI_UNSIGNED,
            i, // send to "i" target
            1, // tag "1" i.e. interrupt
            MPI_COMM_WORLD, &(interruptRequests[i]));
        assert(err == MPI_SUCCESS);
    }

    return true;
}

void CMSat::DataSyncServer::print_solution()
{
    cout << "v ";
    for(uint32_t i = 0; i < model.size(); i++) {
        if (model[i] == l_True) {
            cout << i+1 << " ";
        } else if (model[i] == l_False) {
            cout << "-" << i+1 << " ";
        } else {
            assert(false && "We should always have full model");
        }
    }
    cout << endl;
}

void CMSat::DataSyncServer::send_cnf_to_solvers()
{
    std::cout << "c -->> MPI Server"
    << "sending file to all solvers..." << endl;

    value.resize(num_vars, l_Undef);
    bins.resize(num_vars*2);
    syncMPIFinish.resize(num_vars*2, 0);

    int err;
    bool finished = false;
    Lit buf[1024];
    uint32_t at = 0;
    uint32_t i = 0;

    //first byte we send is the number of variables
    buf[i] = Lit(num_vars, false);
    i++;

    while(!finished) {
        for(;i < 1024; i++) {
            if (clauses_array.size() <= at) {
                buf[i] = lit_Error;
                finished = true;
                continue;
            }

            buf[i] = clauses_array[at];
            at++;
        }
        err = MPI_Bcast(buf, 1024, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
        assert(err == MPI_SUCCESS);
        i = 0;
    }
    assert(clauses_array.size() == at);

    std::cout << "c -->> MPI Server"
    << " sent file to all solvers" << endl;
}

void CMSat::DataSyncServer::new_var()
{
    num_vars++;
}

void CMSat::DataSyncServer::add_xor_clause(const vector<uint32_t>& vars, bool& rhs)
{
    assert(false);
}


void CMSat::DataSyncServer::new_vars(uint32_t i)
{
    num_vars+=i;
}

void CMSat::DataSyncServer::add_clause(const vector<CMSat::Lit>& lits)
{
    for(const auto& lit: lits) {
        clauses_array.push_back(lit);
    }
    clauses_array.push_back(lit_Undef);
}

lbool DataSyncServer::actAsServer()
{
    //both interrupt_sent AND send_requests_finished must be OK to exit
    while(!(interrupt_sent && send_requests_finished)) {
        mpi_recv_from_others();
        finish_data_send();

        if (lastSendNumGotPacket+(mpiSize/2)+1 < numGotPacket &&
            send_requests_finished &&
            !interrupt_sent)
        {
            sendDataToAll();
        }

        if (!interrupt_sent &&
            check_interrupt_and_forward_to_all())
        {
            interrupt_sent = true;

            //HACK below, we don't cleanly exit
            if (solution_val == l_True) {
                cout << "s SATISFIABLE" << endl;
            } else if (solution_val == l_False) {
                cout << "s UNSATISFIABLE" << endl;
            } else {
                assert(false);
            }
            MPI_Abort(MPI_COMM_WORLD, 0);
            exit(0);
        }
        usleep(1);
    }

    return solution_val;
}

