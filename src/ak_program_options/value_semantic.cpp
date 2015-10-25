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

#include <ctype.h>
#include <stdlib.h> 
#include <string.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "errors.h"
#include "value_semantic.h"

#define BASE10 10

namespace ak_program_options {
    using std::shared_ptr;
    using std::string;
    using std::vector;

    string arg("arg");
    
    string optional_arg("[arg]");

    string no_arg("");

    string value_semantic::name() const {
        return is_bool_switch() ? no_arg :
               implicited() ? optional_arg : 
               arg ;
    }

    ///  Constructors without argument
    
    ///  general case for integer types
    template<typename T>
    Value<T>::Value() {
        m_default = 0;
        m_required = true;
    }
    
    ///  specializations for non-integer types
        
    template<>
    Value<bool>::Value() {
        m_default = false;
        m_required = false;
        m_is_bool_switch = true;
    }

    template<>
    Value<double>::Value() {
        m_default = 0.0;
        m_required = true;
    }

    template<>
    Value<string>::Value() {
        m_default = "";
        m_required = true;
    }

    template<>
    Value<vector<string>>::Value() : Value(nullptr) {
        m_composing = true;
        m_required = true;
    }

    ///  constructors with value destination argument
    
    template <typename T>
    Value<T>::Value(T *v) {
        m_destination = v;
        /*  the following has unclear side-effects
            taken out for the time being
        if (v != nullptr) {
            //  use destination value as natural default
            m_defaulted = true;
            m_default = *v;
        }
        */
        m_composing = std::is_same<T, vector<string>>::value;
    }
        
    Value<bool> *bool_switch(bool *v) {
        Value<bool> *val = new Value<bool>(v);
        val->implicit_value(true);
        val->set_as_bool_switch();
        return val;
    }

    ///  only string vector Values return a string vector

    template<typename T>
    const vector<string> Value<T>::get_string_vector_value() const {
        vector<string> x;

        return x;
    }

    template<>
    const vector<string> Value<vector<string>>::get_string_vector_value() const {
        return get_value();
    }


    ///  general case for integer types

    template<typename T>
    string Value<T>::to_string() const {
        return int_to_string();
    }
            
    ///  specializations for non-integer types    
    
    template<>
    string Value<bool>::to_string() const {
        return string((empty() ? m_default : m_value) ? "true" : "false");
    }

    template<>
    string Value<double>::to_string() const {
        return std::to_string(empty() ? m_default : m_value);
    }

    template<>
    string Value<string>::to_string() const {
        return empty() ? m_default : m_value;
    }

    template<>
    string Value<vector<string>>::to_string() const {
        string s("[");
        vector<string> v(empty() ? m_default : m_value);        
        int i = 0;
        
        for (string si : v)
        {
            if (i++ > 0)
            {
                s += ",";
            }
            s += si;
        }
        s += "]";
        
        return s;
    }

    template<typename T>
    string Value<T>::int_to_string() const {
        return std::to_string(empty() ? m_default : m_value);
    }
    
    
    /// FIXME: the following value() functions should be possible as template
    /// attempts to use template resulted in linking errors
    
    Value<int> *value(int *v) {
        Value<int> *val = new Value<int>(v);
        return val;
    }

    Value<long> *value(long *v) {
        Value<long> *val = new Value<long>(v);
        return val;
    }

    Value<long long> *value(long long *v) {
        Value<long long> *val = new Value<long long>(v);
        return val;
    }

    Value<unsigned int> *value(unsigned int *v) {
        Value<unsigned int> *val = new Value<unsigned int>(v);
        return val;
    }

    Value<unsigned long> *value(unsigned long *v) {
        Value<unsigned long> *val = new Value<unsigned long>(v);
        return val;
    }

    Value<unsigned long long> *value(unsigned long long *v) {
        Value<unsigned long long> *val = new Value<unsigned long long>(v);
        return val;
    }

    Value<double> *value(double *v) {
        Value<double> *val = new Value<double>(v);
        return val;
    }

    Value<string> *value(string *v) {
        Value<string> *val = new Value<string>(v);
        return val;
    }

    Value<vector<string>> *value(vector<string> *v) {
        Value<vector<string>> *val = new Value<vector<string>>(v);
        return val;
    }

    ///  general case
    
    template<typename T>
    void Value<T>::set_value(const char *v, shared_ptr<const option_description> opt) {
        char* x;
        m_value = (T)strtol(v, &x, BASE10);
        if (x != v + strlen(v)) {
            throw_exception<invalid_option_value>(invalid_option_value(opt ? opt->name() : "", v));
        }
        m_empty = false;
    }
    
    ///  the integer specializations are covered by the general case and thus omitted
    
    template<>
    void Value<bool>::set_value(const char *v, shared_ptr<const option_description> opt) {
        //  avoid strcasestr()/stristr() as it is not available everywhere
        string pattern(v);
        for (char &c : pattern)
        {
            c = tolower(c);
        }
        bool f = !!strstr("0~false~no~off", pattern.c_str());
        bool t = !!strstr("1~true~yes~on", pattern.c_str());

        if (f == t)
        {
            throw_exception<invalid_option_value>(invalid_option_value(opt ? opt->name() : "", v));
        }

        m_value = !f;
        m_empty = false;
    }

    template<>
    void Value<double>::set_value(const char *v, shared_ptr<const option_description> opt) {
        char *x;
        m_value = strtod(v, &x);
        if (x != v + strlen(v)) {
            throw_exception<invalid_option_value>(invalid_option_value(opt ? opt->name() : "", v));
        }
        m_empty = false;
    }

    template<>
    void Value<string>::set_value(const char *v, shared_ptr<const option_description> opt) {
        m_value = string(v);
        if ((m_value.size() >= 2) && m_value.substr(0, 2) == "--") {
            throw_exception<invalid_option_value>(invalid_option_value(opt ? opt->name() : "", m_value));
        }
        m_empty = false;
    }

    template<>
    void Value<vector<string>>::set_value(const char *v, shared_ptr<const option_description> opt) {
        (void)opt;
        m_value.push_back(string(v));
        m_empty = false;
    }
}

