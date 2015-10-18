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

#ifndef OPTION_DESCRIPTION_H_INCLUDED
#define OPTION_DESCRIPTION_H_INCLUDED

#include <assert.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "errors.h"
#include "scan_arguments.h"
#include "value_semantic.h"

namespace ak_program_options {

#define OPTION_DESCRIPTION_HEADER "OD"

    class option_description {
    private:
#if USE_OD_DEBUGGING
        //  for validity check
        char *hd = OPTION_DESCRIPTION_HEADER;
#endif
        option_description& set_name(const char* name);

        std::string m_short_name = "";
        std::string m_long_name = "";
        std::string m_description = "";
        //  id is either an ASCII char or a unique index derived from a pointer
        int m_id;

        std::shared_ptr<value_semantic> m_value_semantic;

    public:
        option_description();
                
        virtual ~option_description();

        int id() const { 
            valid();
            if (m_short_name.size() == 2) {
                return (int)m_short_name[1];
            }

            //  add 256 to avoid clashes with ASCII chars
            return m_id + 256; 
        };

        bool valid() const {
#if USE_OD_DEBUGGING
            assert(!strcmp(OPTION_DESCRIPTION_HEADER, hd));
#endif
            return true;
        }

        /*
        The 'name' parameter is interpreted by the following rules:
        - if there's no "," character in 'name', it specifies long name
        - otherwise, the part before "," specifies long name and the part
        after -- short name.
        */
        option_description(const char* name,
            value_semantic* s);

        /// Initializes the class with the passed data.
        option_description(const char* name,
            value_semantic* s,
            const char* description);

        // Explanation of this option
        const std::string& description() const;

        /// Semantic of option's value
        std::shared_ptr<value_semantic> semantic() const;

        /// Return the name
        std::string name() const;

        /// Returns the option name, formatted suitably for usage message. 
        std::string format_name() const;

        /// Returns the parameter name and properties, formatted suitably for
        /// usage message.
        std::string format_parameter() const;

        long_option_struct *long_option() const;
    };
}

#endif
