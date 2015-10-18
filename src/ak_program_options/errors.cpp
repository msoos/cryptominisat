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

#include <exception>
#include <string>
#include <iostream>
#include <stdio.h>

#include "errors.h"

namespace ak_program_options {

#if USE_O_FOR_DEBUGGING
    void o(const char *s) {
        fprintf(stderr, "%s\n", s);
        fflush(stderr);
        //  std::cout << std::string(s) << std::endl;
    }
    
    void o() {
        fprintf(stderr, "\n");
        fflush(stderr);
//        std::cout << std::endl;
    }
    
    void o(std::string s) {
        fprintf(stderr, "%s\n", s.c_str());
        fflush(stderr);
        // std::cout << s << std::endl;
    }
#endif   //  USE_O_FOR_DEBUGGING

    const std::string error::what() const {
        return m_msg;
    }

    const std::string error_with_no_option_name::what() const {
        std::string patt("%canonical_option%");
        size_t f = m_msg.find(patt);

        if (f != std::string::npos) {
            std::string *s = new std::string(m_msg);

            s->replace(f, std::string(patt).length(), m_token);

            return *s;
        }

        return m_msg;
    }

    const std::string error_with_option_name::what() const {
        std::string patt("%canonical_option%");
        size_t f = m_msg.find(patt);

        if (f != std::string::npos) {
            std::string *s = new std::string(m_msg);
            s->replace(f, std::string(patt).length(), m_option_name);

            return *s;
        }

        return m_msg;
    }
}

