/*
 * ak_program_options
 *
 * Copyright (c) 2015, Axel Kemper. All rights reserved.
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
 *
 * Package ak_program_options was derived from boost::program_options
 * as a simplified and restricted package suitable for cryptominisat.
 * Original copyright:
 * Copyright Vladimir Prus 2002-2004.
 * Distributed under the Boost Software License, Version 1.0.
 * (See http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef VARIABLES_MAP_H_INCLUDED
#define VARIABLES_MAP_H_INCLUDED

#include <map>
#include "parsers.h"
#include "value_semantic.h"

namespace ak_program_options {

    class variables_map : public std::map<std::string, value_semantic *> {
    public:
        variables_map() {};
        ~variables_map() {};

        /** Obtains the value of variable 'name', from *this.

        - if there's no value in *this returns empty value.

        - if there's defaulted value
        - otherwise, return value from *this

        - if there's a non-defaulted value, returns it.
        */
        const value_semantic & operator[](const std::string& name) const;

        void clear();

        void notify();
           
        /**  display a list of options and their values  */     
        void show_options();

    private:
        /** Returns value of variable 'name' stored in *this, or
        empty value otherwise. */
        const value_semantic *get(const std::string& name) const;
        /** remember options for deconstruction */
        const basic_parsed_options *m_options;
    };

    /** Stores in 'vm' all options that are defined in 'options'.
    If 'vm' already has a non-defaulted value of an option, that value
    is not changed, even if 'options' specify some value.
    'options' are deconstructed during deconstruction of 'vm'
    */
    void store(const basic_parsed_options *options, variables_map &vm);

    /** Runs all 'notify' function for options in 'vm'. */
    void notify(variables_map &vm);
}

#endif
