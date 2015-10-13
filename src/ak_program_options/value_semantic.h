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

#ifndef VALUE_SEMANTIC_H_INCLUDED
#define VALUE_SEMANTIC_H_INCLUDED

#include <iostream>

namespace ak_program_options {

    class value_semantic {  
    public:
        virtual ~value_semantic() {};
        virtual void apply_implicit() {};
        virtual bool composing() const { return false; };
        virtual void notify() const {};
        virtual void set_value(const char *v) {(void)v;};
        virtual int *get_value() const { return nullptr; };
        virtual std::string to_string() const { return std::string("???"); };
        std::string name() const;

        /** If stored value if of type T, returns that value. Otherwise,
        throws exception. */
        template<typename T>
        const T as() const {
            return *((T *)get_value());
        }

        /// Returns true if a default value is defined
        virtual bool defaulted() const { return false; };

        /// Returns true if no value is stored.
        virtual bool empty() const { return true; };

        /// Returns true if implicit value is specified
        virtual bool implicited() const { return false; };

        /// Return true if a value argument is required
        virtual bool required() const { return false; };
        
        /// Return true iff it is a bool_switch
        virtual bool is_bool_switch() const { return false; };      
    };

#define NO_VALUE ((value_semantic *)0)

    template<class T> class Value : public value_semantic
    {
    public:
        Value();
        Value(T *v);
        ~Value() {};
                
        Value *default_value(const T &v) { 
            m_default = v; 
            m_defaulted = true;
            return this; 
        };
        Value *default_value(const T &v, const std::string &s) { 
            m_default = v; 
            m_defaulted = true;
            m_textual = s;
            return this; 
        };
        Value *implicit_value(const T &v) {
            m_implicit = v; 
            m_implicited = true;
            m_required = false;
            return this;
        };
        void apply_implicit() {
            m_value = m_implicit;
            m_empty = false;
        };
        void notify() const {
            if (m_destination != nullptr) {
                *m_destination = (empty() && defaulted()) ? m_default 
                                                          : m_value;
            }
        }
        void set_value(const char *v);
        int *get_value() const { return (int *)(empty() ? &m_default : &m_value); };
        bool composing() const { return m_composing; }
        bool defaulted() const { return m_defaulted; };
        bool implicited() const { return m_implicited; };
        bool empty() const { return m_empty; }
        bool required() const { return m_required; }
        bool is_bool_switch() const { return m_is_bool_switch; }
        void set_as_bool_switch() { m_is_bool_switch = true; }
        std::string to_string() const;
        const std::string &textual() const { return m_textual; };

    private:
        std::string int_to_string() const;
    
    private:
        T *m_destination = nullptr;
        T m_default;
        T m_implicit;
        T m_value;
        bool m_empty = true;
        bool m_composing = false;
        bool m_required = false;
        bool m_defaulted = false;
        bool m_implicited = false;
        bool m_is_bool_switch = false;
        std::string m_textual = "";
    };

    template<typename T>
    Value<T> *value() { return new Value<T>(); }

    Value<int> *value(int *v);
    Value<long> *value(long *v);
    Value<long long> *value(long long *v);
    Value<unsigned int> *value(unsigned int *v);
    Value<unsigned long> *value(unsigned long *v);
    Value<unsigned long long> *value(unsigned long long *v);
    Value<double> *value(double *v);
    Value<std::string> *value(std::string *v);
    Value<std::vector<std::string>> *value(std::vector<std::string> *v);

    Value<bool> *bool_switch(bool *v);
    
}  //  namespace ak_program_options

#endif
