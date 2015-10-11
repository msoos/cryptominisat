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

#include <algorithm>
#include <cassert>

#include "errors.h"
#include "akpo_getopt.h"
#include "option_description.h"

namespace ak_program_options {

	option_description::
		option_description(const char *name,
			const value_semantic *s)
	{
		m_value_semantic = s;
		set_name(name);
	}

	option_description::
		option_description(const char* name,
			const value_semantic *s,
			const char* description)
	{
		m_description = description;
		m_value_semantic = s;
		set_name(name);
	}

	const std::string&
		option_description::description() const
	{
		return m_description;
	}

	std::string option_description::short_option() const {
		if (!m_short_name.empty()) {
		        // assert(m_short_name.size() == 2);
			// assert(m_short_name.substr(0, 1) == std::string("-"));
			if (m_value_semantic == NO_VALUE) {
				return m_short_name.substr(1);
			}
			else {
				return m_short_name.substr(1).append(":");
			}
		}
		else {
			return std::string("");
		}
	}

	option *option_description::long_option() const {
		option *opt;

		if (m_long_name.empty()) {
			opt = nullptr;
		}
		else {
			const value_semantic *sem = m_value_semantic;
			opt = new option;
			//  option 
			opt->has_arg = (sem == NO_VALUE) ? no_argument :
							sem->implicited() ? optional_argument :
							required_argument;
			opt->name = m_long_name.c_str();
			opt->flag = 0;
			//  val is either the short name char or a unique hash int beyond 256
			if (!m_short_name.empty()) {
				assert(m_short_name.size() == 2);
				assert(m_short_name[0] == '-');
			}
			opt->val = (m_short_name.empty() ? (int)(256 + m_id) : (int)m_short_name[1]);
		}

		return opt;
	}

	std::string
		option_description::format_name() const
	{
		if (!m_short_name.empty())
		{
			return m_long_name.empty()
				? m_short_name
				: std::string(m_short_name).append(" [ --").
				append(m_long_name).append(" ]");
		}
		return std::string("--").append(m_long_name);
	}

	std::string
		option_description::format_parameter() const
	{
		if (m_value_semantic != NO_VALUE)
			return m_value_semantic->name();
		else
			return "";
	}

	std::string option_description::name() const {
		if (m_long_name.empty()) {
			assert(m_short_name.size() == 2);
			assert(m_short_name[0] == '-');
			return m_short_name.substr(1);
		}
		else {
			return m_long_name;
		}
	}

	value_semantic *option_description::semantic() const {
		return (value_semantic *)m_value_semantic;
	}

	option_description&
		option_description::set_name(const char* _name)
	{
		std::string name(_name);
		std::string::size_type n = name.find(',');
		if (n != std::string::npos) {
			assert(n == name.size() - 2);
			m_long_name = std::string(name.substr(0, n));
			m_short_name = std::string("-").append(name.substr(n + 1, 1));
			m_id = (int)m_short_name[1];
		}
		else if (name.size() < 2) {
			//  no long name supplied, only a short name
			m_long_name = std::string("");
			m_short_name = std::string("-").append(name);
		}
		else {
			m_long_name = std::string(name);
			m_short_name = std::string("");
		}

		m_id = (int)((size_t)this);

		return *this;
	}
}
