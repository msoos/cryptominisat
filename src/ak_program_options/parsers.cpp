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

#include <stdlib.h>

#include "command_line_parser.h"
#include "parsers.h"

namespace ak_program_options {
    
basic_parsed_options *parse_command_line(unsigned argc, char *argv[], const options_description &desc) {
    basic_parsed_options *bpo = new basic_parsed_options(argc, argv, desc);

    return bpo;
}

// Returns the option_description which has either the flag with code id
// or the pointer id. nullptr iff not found 
const option_description *basic_parsed_options::findById(int id) const {
    return m_desc.findById(id);
}

// Returns the option_description which has the name.
// nullptr iff not found 
const option_description *basic_parsed_options::findByName(std::string name) const {
    return m_desc.findByName(name);
}

//  return string of short option commandline flags
const std::string basic_parsed_options::short_options() const {
    std::vector<option_description *> opts = m_desc.options();
    std::string s("");

    for (const option_description *opt : opts) {
        s.append(opt->short_option());
    }

    std::string ret = std::string(s);

    return ret;
}

//  return array of long option structs
option *basic_parsed_options::long_options() const {
    std::vector<option_description *> opts = m_desc.options();
    option *long_opts = new option[opts.size() + 1];
    option *long_opt;
    int pos = 0;

    //  collect all options which have a long name
    for (const option_description *opt : opts) {
        long_opt = opt->long_option();

        if (long_opt != nullptr) {
            *(long_opts + pos++) = *long_opt;
            delete long_opt;
        }
    }

    //  create final entry in table
    long_opt = (struct option *)malloc(sizeof(struct option));
    long_opt->name = 0;
    long_opt->has_arg = false;
    long_opt->flag = 0;
    long_opt->val = 0;
    
    *(long_opts + pos) = *long_opt;
    free(long_opt);

    return long_opts;
}

basic_command_line_parser command_line_parser(unsigned argc, char **argv) {
    basic_command_line_parser *clp = new basic_command_line_parser(argc, argv);

    return *clp;
}

}
