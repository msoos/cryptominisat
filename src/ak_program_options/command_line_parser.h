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

#ifndef CL_PARSERS_H_INCLUDED
#define CL_PARSERS_H_INCLUDED

#include <string>

#include "options_description.h"
#include "parsers.h"
#include "positional_options.h"

namespace ak_program_options {

	class basic_command_line_parser {
	private:
		unsigned m_argc;
		char **m_argv;
		const options_description *m_desc = nullptr;
		const positional_options_description *m_positional_desc = nullptr;

	public:
		basic_command_line_parser(unsigned argc, char **argv) : m_argc(argc), m_argv(argv) {};

		basic_command_line_parser &options(const options_description& desc)
		{
			m_desc = &desc;
			return *this;
		};

		basic_command_line_parser &positional(
				const positional_options_description& desc)	{
			m_positional_desc = &desc;
			return *this;
		};

		const basic_parsed_options *run() {
			basic_parsed_options *bpo = new basic_parsed_options(m_argc, m_argv, *m_desc);
			bpo->set_positional_description(m_positional_desc);

			return bpo;
		}
	};

	basic_command_line_parser command_line_parser(unsigned argc, char **argv);
}

#endif
