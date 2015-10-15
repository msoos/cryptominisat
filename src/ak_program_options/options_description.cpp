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
#include <iostream>
#include <iterator>
#include <sstream>

#include "errors.h"
#include "scan_arguments.h"
#include "options_description.h"

namespace ak_program_options {

namespace {

    /* Given a string 'par', that contains no newline characters
    outputs it to 'os' with wordwrapping, that is, as several
    line.

    Each output line starts with 'indent' space characters,
    following by characters from 'par'. The total length of
    line is no longer than 'line_length'.

    */
    void format_paragraph(std::ostream& os,
        std::string par,
        unsigned indent,
        unsigned line_length)
    {
        // Through reminder of this function, 'line_length' will
        // be the length available for characters, not including
        // indent.
        assert(indent < line_length);
        line_length -= indent;

        // index of tab (if present) is used as additional indent relative
        // to first_column_width if paragrapth is spanned over multiple
        // lines if tab is not on first line it is ignored
        std::string::size_type par_indent = par.find('\t');

        if (par_indent == std::string::npos)
        {
            par_indent = 0;
        }
        else
        {
            // only one tab per paragraph allowed
            if (count(par.begin(), par.end(), '\t') > 1)
            {
                throw_exception(ak_program_options::error(
                    "Only one tab per paragraph is allowed in the options description"));
            }

            // erase tab from string
            par.erase(par_indent, 1);

            // this assert may fail due to user error or 
            // environment conditions!
            assert(par_indent < line_length);

            // ignore tab if not on first line
            if (par_indent >= line_length)
            {
                par_indent = 0;
            }
        }

        if (par.size() < line_length)
        {
            os << par;
        }
        else
        {
            std::string::const_iterator       line_begin = par.begin();
            const std::string::const_iterator par_end = par.end();

            bool first_line = true; // of current paragraph!        

            while (line_begin < par_end)  // paragraph lines
            {
                if (!first_line)
                {
                    // If line starts with space, but second character
                    // is not space, remove the leading space.
                    // We don't remove double spaces because those
                    // might be intentianal.
                    if ((*line_begin == ' ') &&
                        ((line_begin + 1 < par_end) &&
                            (*(line_begin + 1) != ' ')))
                    {
                        line_begin += 1;  // line_begin != line_end
                    }
                }

                // Take care to never increment the iterator past
                // the end, since MSVC 8.0 (brokenly), assumes that
                // doing that, even if no access happens, is a bug.
                unsigned remaining = static_cast<unsigned>(std::distance(line_begin, par_end));
                std::string::const_iterator line_end = line_begin +
                    ((remaining < line_length) ? remaining : line_length);

                // prevent chopped words
                // Is line_end between two non-space characters?
                if ((*(line_end - 1) != ' ') &&
                    ((line_end < par_end) && (*line_end != ' ')))
                {
                    // find last ' ' in the second half of the current paragraph line
                    std::string::const_iterator last_space =
                        std::find(std::reverse_iterator<std::string::const_iterator>(line_end),
                            std::reverse_iterator<std::string::const_iterator>(line_begin),
                            ' ')
                        .base();

                    if (last_space != line_begin)
                    {
                        // is last_space within the second half ot the 
                        // current line
                        if (static_cast<unsigned>(std::distance(last_space, line_end)) <
                            (line_length / 2))
                        {
                            line_end = last_space;
                        }
                    }
                } // prevent chopped words

                  // write line to stream
                copy(line_begin, line_end, std::ostream_iterator<char>(os));

                if (first_line)
                {
                    indent += static_cast<unsigned>(par_indent);
                    line_length -= static_cast<unsigned>(par_indent); // there's less to work with now
                    first_line = false;
                }

                // more lines to follow?
                if (line_end != par_end)
                {
                    os << '\n';

                    for (unsigned pad = indent; pad > 0; --pad)
                    {
                        os.put(' ');
                    }
                }

                // next line starts after of this line
                line_begin = line_end;
            } // paragraph lines
        }
    }

    void format_description(std::ostream& os,
        const std::string& desc,
        unsigned first_column_width,
        unsigned line_length)
    {
        // we need to use one char less per line to work correctly if actual
        // console has longer lines
        assert(line_length > 1);
        if (line_length > 1)
        {
            --line_length;
        }

        // line_length must be larger than first_column_width
        // this assert may fail due to user error or environment conditions!
        assert(line_length > first_column_width);

        std::istringstream ss(desc);

        while (!ss.eof())  // paragraphs
        {
            std::string par;

            getline(ss, par, '\n');

            format_paragraph(os, par, first_column_width,
                line_length);

            // prepair next line if any
            if (!ss.eof())
            {
                os << '\n';

                for (unsigned pad = first_column_width; pad > 0; --pad)
                {
                    os.put(' ');
                }
            }
        }  // paragraphs
    }

    void format_one(std::ostream& os, const option_description& opt,
        unsigned first_column_width, unsigned line_length)
    {
        std::stringstream ss;
        ss << "  " << opt.format_name() << ' ' << opt.format_parameter();

        // Don't use ss.rdbuf() since g++ 2.96 is buggy on it.
        os << ss.str();

        if (!opt.description().empty())
        {
            if (ss.str().size() >= first_column_width)
            {
                os.put('\n'); // first column is too long, lets put description in new line
                for (unsigned pad = first_column_width; pad > 0; --pad)
                {
                    os.put(' ');
                }
            }
            else {
                for (unsigned pad = first_column_width - static_cast<unsigned>(ss.str().size()); pad > 0; --pad)
                {
                    os.put(' ');
                }
            }

            format_description(os, opt.description(),
                first_column_width, line_length);
        }
    }
}

options_description& options_description::add(const options_description& desc)
{
    options_description *d(new options_description(desc));

    m_groups.push_back(d);

    for (option_description *opt : desc.m_options) {
        add(opt);
        m_belong_to_group.back() = true;
    }

    return *this;
}

const option_description *options_description::findById(int id) const {
    for (const option_description *opt : m_options) {
        if (opt->id() == id) {
            return opt;
        }
    }

    for (const options_description *group : m_groups) {
        const option_description *opt = group->findById(id);

        if (opt != nullptr) {
            return opt;
        }
    }

    return nullptr;
}

const option_description *options_description::findByName(std::string name) const {
    for (const option_description *opt : m_options) {
        if (opt->name() == name) {
            return opt;
        }
    }

    for (const options_description *group : m_groups) {
        const option_description *opt = group->findByName(name);

        if (opt != nullptr) {
            return opt;
        }
    }

    return nullptr;
}

unsigned
options_description::get_option_column_width() const
{
    /* Find the maximum width of the option column */
    unsigned width(23);
    
    for (const option_description *opt : m_options)
    {
        std::stringstream ss;
        ss << "  " << opt->format_name() << ' ' << opt->format_parameter();
        width = std::max(width, static_cast<unsigned>(ss.str().size()));
    }

    /* Get width of groups as well*/
    for (const options_description *group : m_groups)
        width = std::max(width, group->get_option_column_width());

    /* this is the column were description should start, if first
    column is longer, we go to a new line */
    const unsigned start_of_description_column = m_line_length - m_min_description_length;

    width = std::min(width, start_of_description_column - 1);

    /* add an additional space to improve readability */
    ++width;

    return width;
}

void options_description::print(std::ostream& os, unsigned width) const {
    if (!m_caption.empty())
        os << m_caption << ":\n";

    if (!width)
        width = get_option_column_width();

    /* The options formatting style is stolen from Subversion. */
    int i = 0;
    for (const option_description *opt : m_options)
    {
        if (m_belong_to_group[i++])
            continue;

        format_one(os, *opt, width, m_line_length);

        os << "\n";
    }

    for (options_description *group : m_groups) {
        os << "\n";
        group->print(os, width);
    }
}

std::vector<option_description *> options_description::options() const {
    std::vector<option_description *> v;
    int i = 0;

    for (option_description *opt : m_options) {
        if (!m_belong_to_group[i++]) {
            v.push_back(opt);
        }
    }

    for (const options_description *group : m_groups) {
        std::vector<option_description *> vGroup = group->options();

        v.insert(v.end(), vGroup.begin(), vGroup.end());
    }

    return v;
}

options_description_easy_init
options_description::add_options()
{
    return options_description_easy_init(this);
}

options_description_easy_init&
options_description_easy_init::
operator()(const char* name,
    const char* description)
{
    // Create untypes semantic which accepts zero tokens: i.e. 
    // no value can be specified on command line.
    // FIXME: does not look exception-safe
    option_description *d = new option_description(name, NO_VALUE, description);

    m_owner->add(d);
    return *this;
}

options_description_easy_init&
options_description_easy_init::
operator()(const char* name,
    const value_semantic* s)
{
    option_description *d = new option_description(name, s);
    m_owner->add(d);
    return *this;
}

options_description_easy_init&
options_description_easy_init::
operator()(const char* name,
    const value_semantic* s,
    const char* description)
{
    option_description *d = new option_description(name, s, description);

    m_owner->add(d);
    return *this;
}

void options_description::add(option_description *desc) {
    m_options.push_back(desc);
    m_belong_to_group.push_back(false);
}

std::ostream& operator<<(std::ostream& os, const options_description& desc)
{
    desc.print(os);
    return os;
}
}
