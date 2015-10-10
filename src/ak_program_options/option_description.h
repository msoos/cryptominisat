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

#include <string>
#include <vector>

#include "errors.h"
#include "akpo_getopt.h"
#include "value_semantic.h"

namespace ak_program_options {

	class option_description {
	private:
		option_description& set_name(const char* name);

		std::string m_short_name;
		std::string m_long_name;
		std::string m_description;
		int m_id;

		const value_semantic *m_value_semantic;

	public:
		option_description() {
			m_value_semantic = NO_VALUE;
			m_id = (int)((size_t)this);
		};

		int id() const { 
			if (m_short_name.size() == 2) {
				return (int)m_short_name[1];
			}

			return m_id + 256; 
		};

		/*
		The 'name' parameter is interpreted by the following rules:
		- if there's no "," character in 'name', it specifies long name
		- otherwise, the part before "," specifies long name and the part
		after -- short name.
		*/
		option_description(const char* name,
			const value_semantic* s);

		/** Initializes the class with the passed data.
		*/
		option_description(const char* name,
			const value_semantic* s,
			const char* description);

		// Explanation of this option
		const std::string& description() const;

		/// Semantic of option's value
		value_semantic *semantic() const;

		/// Return the name
		std::string name() const;

		/// Returns the option name, formatted suitably for usage message. 
		std::string format_name() const;

		/** Returns the parameter name and properties, formatted suitably for
		usage message. */
		std::string format_parameter() const;

		/** return short option flag for getopt() */
		std::string short_option() const;

		/** return long option struct for getopt_log() */
		option *long_option() const;

	};
}

#endif
