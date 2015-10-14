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
#include <string.h>

#include <iostream>
#include <string>
#include <vector>

#include "errors.h"
#include "value_semantic.h"

#define BASE10 10

namespace ak_program_options {
    std::string arg("arg");
    
    std::string optional_arg("[arg]");

    std::string no_arg("");

    std::string value_semantic::name() const {
        return required() ? arg : 
               is_bool_switch() ? no_arg :
               optional_arg;
    }

    template<>
    Value<int>::Value() {
        m_default = 0;
        m_required = true;
    }

    template<>
    Value<long>::Value() {
        m_default = 0;
        m_required = true;
    }

    template<>
    Value<long long>::Value() {
        m_default = 0;
        m_required = true;
    }

    template<>
    Value<unsigned int>::Value() {
        m_default = 0;
        m_required = true;
    }

    template<>
    Value<unsigned long>::Value() {
        m_default = 0;
        m_required = true;
    }

    template<>
    Value<unsigned long long>::Value() {
        m_default = 0;
        m_required = true;
    }

    template<>
    Value<bool>::Value() {
        m_default = false;
        m_required = true;
        m_is_bool_switch = true;
    }

    template<>
    Value<double>::Value() {
        m_default = 0.0;
        m_required = true;
    }

    template<>
    Value<std::string>::Value() {
        m_default = "";
        m_required = true;
    }

    template<>
    Value<std::vector<std::string>>::Value() : Value(nullptr) {
        m_composing = true;
        m_required = true;
    }

    template <typename T>
    Value<T>::Value(T *v) {
        m_destination = v;
        m_composing = std::is_same<T, std::vector<std::string>>::value;
    }

    Value<bool> *bool_switch(bool *v) {
        Value<bool> *val = new Value<bool>(v);
        val->implicit_value(true);
        val->set_as_bool_switch();
        return val;
    }

    /// FIXME: there should be a more compact way for the following
        
    template<>
    std::string Value<int>::to_string() const {
        return int_to_string();
    }

    template<>
    std::string Value<long>::to_string() const {
        return int_to_string();
    }

    template<>
    std::string Value<long long>::to_string() const {
        return int_to_string();
    }

    template<>
    std::string Value<unsigned int>::to_string() const {
        return int_to_string();
    }

    template<>
    std::string Value<unsigned long>::to_string() const {
        return int_to_string();
    }

    template<>
    std::string Value<unsigned long long>::to_string() const {
        return int_to_string();
    }

    template<>
    std::string Value<bool>::to_string() const {
        return std::string((empty() ? m_default : m_value) ? "true" : "false");
    }

    template<>
    std::string Value<double>::to_string() const {
        return std::to_string(empty() ? m_default : m_value);
    }

    template<>
    std::string Value<std::string>::to_string() const {
        return empty() ? m_default : m_value;
    }

    template<>
    std::string Value<std::vector<std::string>>::to_string() const {
        std::string s("[");
        std::vector<std::string> v(empty() ? m_default : m_value);        
        int i = 0;
        
        for (std::string si : v)
        {
            if (i++ > 0)
            {
                s += ",";
            }
            s += si;
        }
        s += "]";
        
        return std::string(s);
    }

    template<typename T>
    std::string Value<T>::int_to_string() const {
        return std::to_string(empty() ? m_default : m_value);
    }
    
    
    /// FIXME: the following value() functions should be possible as template

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

    Value<std::string> *value(std::string *v) {
        Value<std::string> *val = new Value<std::string>(v);
        return val;
    }

    Value<std::vector<std::string>> *value(std::vector<std::string> *v) {
        Value<std::vector<std::string>> *val = new Value<std::vector<std::string>>(v);
        return val;
    }

    template<>
    void Value<int>::set_value(const char *v) {
        char* x;
        m_value = (int)strtol(v, &x, BASE10);
        if (x != v + strlen(v)) {
            throw_exception<invalid_option_value>(invalid_option_value(v, ""));
        }
        m_empty = false;
    }

    template<>
    void Value<long>::set_value(const char *v) {
        char *x;
        m_value = strtol(v, &x, BASE10);
        if (x != v + strlen(v)) {
            throw_exception<invalid_option_value>(invalid_option_value("", v));
        }
        m_empty = false;
    }

    template<>
    void Value<long long>::set_value(const char *v) {
        char *x;
        m_value = strtol(v, &x, BASE10);
        if (x != v + strlen(v)) {
            throw_exception<invalid_option_value>(invalid_option_value("", v));
        }
        m_empty = false;
    }

    template<>
    void Value<unsigned int>::set_value(const char *v) {
        char *x;
        long int orig = strtol(v, &x, BASE10);
        if ((x != v + strlen(v))
            || (orig < 0)
        ) {
            throw_exception<invalid_option_value>(invalid_option_value("", v));
        }
        m_value = (unsigned int)orig;
        m_empty = false;
    }

    template<>
    void Value<unsigned long>::set_value(const char *v) {
        char *x;
        long int orig = strtol(v, &x, 10);
        if ((x != v + strlen(v))
            || (orig < 0)
        ) {
            throw_exception<invalid_option_value>(invalid_option_value("", v));
        }
        m_value = (unsigned long)orig;
        m_empty = false;
    }

    template<>
    void Value<unsigned long long>::set_value(const char *v) {
        char *x;
        long int orig = strtol(v, &x, 10);
        if ((x != v + strlen(v))
            || (orig < 0)
        ) {
            throw_exception<invalid_option_value>(invalid_option_value("", v));
        }
        m_value = (unsigned long long)orig;
        m_empty = false;
    }

    template<>
    void Value<bool>::set_value(const char *v) {
        m_value = (strstr("0~false~FALSE~no~NO", v) != NULL);
        m_empty = false;
    }

    template<>
    void Value<double>::set_value(const char *v) {
        char *x;
        m_value = strtod(v, &x);
        if (x != v + strlen(v)) {
            throw_exception<invalid_option_value>(invalid_option_value("", v));
        }
        m_empty = false;
    }

    template<>
    void Value<std::string>::set_value(const char *v) {
        m_value = std::string(v);
        if ((m_value.size() >= 2) && m_value.substr(0, 2) == "--") {
            throw_exception<invalid_option_value>(invalid_option_value("", m_value));
        }
        m_empty = false;
    }

    template<>
    void Value<std::vector<std::string>>::set_value(const char *v) {
        m_value.push_back(std::string(v));
        m_empty = false;
    }
}

