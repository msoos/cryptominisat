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

#ifndef PARSERS_H_INCLUDED
#define PARSERS_H_INCLUDED

#include <string>
#include "options_description.h"
#include "positional_options.h"
#include "akpo_getopt.h"

namespace ak_program_options {
	class basic_parsed_options {
	private:
		const options_description &m_desc;
		const positional_options_description *m_positional_desc;

	public:
		unsigned argc;
		char **argv;

		basic_parsed_options(unsigned _argc, char *_argv[], const options_description &_desc) :
			m_desc(_desc), m_positional_desc(0), argc(_argc), argv(_argv)
			{}

		void set_positional_description(const positional_options_description *desc) { m_positional_desc = desc; };
		const positional_options_description *get_positional_description() const { return m_positional_desc; };

		///  return string of short option commandline flags
		const char *short_options() const;

		///  return array of long option structs
		option *long_options() const;

		const option_description *findById(int id) const;
		const option_description *findByName(std::string name) const;

		///  return options
		const options_description &descriptions() const { return m_desc; }
		const positional_options_description *positional_descriptions() const { return m_positional_desc; }
	};

	basic_parsed_options *parse_command_line(unsigned argc, char *argv[], const options_description &desc);
}

#endif
