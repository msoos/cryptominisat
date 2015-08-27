/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
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

#ifndef _FEATURES_TO_RECONF_H_
#define _FEATURES_TO_RECONF_H_

#include "features.h"
#include <iostream>
using std::cout;
using std::endl;

namespace CMSat {

int get_reconf_from_features(const Features& /*feat*/, const int /*verb*/)
{
    return 0;
}


} //end namespace

#endif //_FEATURES_TO_RECONF_H_


