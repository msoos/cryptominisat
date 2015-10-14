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
#ifndef ERRORS_H_INCLUDED
#define ERRORS_H_INCLUDED

#include <string>
#include <vector>

namespace ak_program_options {

    namespace exception_detail {
        template <class T>
        class clone_impl : public T
        {
            struct clone_tag { };
            clone_impl(clone_impl const & x, clone_tag) : T(x) {};
        };

        template <class T>
        struct error_info_injector : public T
        {
            explicit error_info_injector(T const & x) : T(x) {}
        };
    }  //  namespace exception_detail

    class error {
    protected:
        const std::string m_msg;
    public:
        error(const std::string &s) : m_msg(s) {};
        virtual const std::string what() const;
    };

    template <typename T>
    void throw_exception(T const &e) { 
        using namespace exception_detail;

        error_info_injector<T> eii(e);
        clone_impl< error_info_injector<T> > ci((clone_impl< error_info_injector<T> > const &)eii);

        throw ci;
    }

    class error_with_no_option_name : public error {
    protected:
        const std::string m_token;
    public:
        error_with_no_option_name(const std::string &s, const std::string &token = "") : error(s), m_token(token) {};
        virtual const std::string what() const;
    };

    class error_with_option_name : public error_with_no_option_name {
    private:
        const std::string m_option_name;
    public:
        error_with_option_name(const std::string &s, const std::string &opt = "", const std::string &token = "")
            : error_with_no_option_name(s, token), m_option_name(opt) {};
        const std::string get_option_name() const { return m_option_name; };
        virtual const std::string what() const;
    };

    /**  The exception thrown in the event of a failed any_cast of an any value. */
    class bad_any_cast : public error {

    };

    /** Class thrown when a required/mandatory option is missing */
    class required_option : public error_with_option_name {
    public:
        // option name is constructed by the option_descriptor and never on the fly
        required_option(const std::string& option_name)
            : error_with_option_name("option '%canonical_option%' is required but missing", "", option_name) {}
    };

    /** Class thrown when there are several occurrences of an
    option, but user called a method which cannot return
    them all. */
    class multiple_occurrences : public error_with_option_name {
    public:
        multiple_occurrences(const std::string &option_name)
            : error_with_option_name("option '%canonical_option%' cannot be specified more than once", option_name) {}
    };

    /** Class thrown when option name is not recognized. */
    class unknown_option : public error_with_no_option_name {
    public:
        unknown_option(const std::string& original_token = "")
            : error_with_no_option_name("unrecognised option '%canonical_option%'", original_token) {}
    };

    /** Class thrown when there's ambiguity amoung several possible options. */
    class ambiguous_option : public error_with_option_name {
    public:
        ambiguous_option(const std::vector<std::string>& xalternatives)
            : error_with_option_name("option '%canonical_option%' is ambiguous"),
            m_alternatives(xalternatives){}

        const std::vector<std::string>& alternatives() const throw() { return m_alternatives; }

    private:
        std::vector<std::string> m_alternatives;
    };

    /** Class thrown if there is an invalid option value given */
    class invalid_option_value
        : public error_with_option_name
    {
    public:
        invalid_option_value(const std::string& opt, const std::string& value) :
            error_with_option_name(value, opt) {};
    };

    /** Class thrown when there are syntax errors in given command line */
    class invalid_command_line_syntax : public error_with_option_name {
    public:
        invalid_command_line_syntax(std::string s,
            const std::string& option_name = "",
            const std::string& original_token = "") :
            error_with_option_name(s, option_name, original_token) {}
    };


    /** Class thrown when there are too many positional options.
    This is a programming error.
    */
    class too_many_positional_options_error : public error {
    public:
        too_many_positional_options_error()
            : error("too many positional options have been specified on the command line"){}
    };

}

#endif
