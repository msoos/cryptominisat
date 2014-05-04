/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include "solvertypesmini.h"

#include <vector>
#include <mutex>
using std::vector;
using std::mutex;

namespace CMSat {

class SharedData
{
    public:
        SharedData(const uint32_t _num_threads) :
            num_threads(_num_threads)
        {}

        struct Spec {
            Spec() {
                data = new vector<Lit>;
            }
            ~Spec() {
                delete data;
            }
            vector<Lit>* data;

            void clear()
            {
                delete data;
                data = NULL;
            }
        };
        vector<lbool> value;
        vector<Spec> bins;
        mutex unit_mutex;
        mutex bin_mutex;

        uint32_t num_threads;

        size_t calc_memory_use_bins()
        {
            size_t mem = 0;
            mem += bins.capacity()*sizeof(Spec);
            for(size_t i = 0; i < bins.size(); i++) {
                if (bins[i].data) {
                    mem += bins[i].data->capacity()*sizeof(Lit);
                    mem += sizeof(vector<Lit>);
                }
            }
            return mem;
        }
};

}

#endif //SHARED_DATA_H
