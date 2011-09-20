/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2011, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
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

#ifndef __RESTART_PRINTER__H__
#define __RESTART_PRINTER__H__

#include "constants.h"
#include <limits>

class ThreadControl;

class RestartPrinter
{
    public:
        RestartPrinter(const ThreadControl* _control) :
            control(_control)
            , lastConflPrint(0)
            , space(10)
        {}

        void printStatHeader() const;
        void printRestartStat(const char* type = "N");
        void printEndSearchStat();

    private:
        const ThreadControl* control;
        uint64_t lastConflPrint;
        const unsigned int space;
};

#endif //__RESTART_PRINTER__H__