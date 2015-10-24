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

#include <assert.h>
#include <cstring>
#include <map>
#include <iostream>
#include <string>

#include "command_line_parser.h"
#include "scan_arguments.h"
#include "positional_options.h"
#include "variables_map.h"

namespace ak_program_options {

OptionParserControl optionParserControl;

/** Stores in 'vm' all options that are defined in 'options'.
If 'vm' already has a non-defaulted value of an option, that value
is not changed, even if 'options' specify some value.
*/
void store(const basic_parsed_options *options, variables_map &vm)
{
    o("store started");
    const options_description *optionsDescriptions = options->descriptions();
    const positional_options_description *positional_options = options->get_positional_description();
    unsigned pos_option_index = 0;
    OptionParserControl *opc = &optionParserControl;
    std::shared_ptr<value_semantic> sem;

    //  start with argv[1] as first parameter token
    opc->idx = 1;

    while (true)
    {
        int opt = scan_arguments(options->argc, options->argv, optionsDescriptions);
        if ((char)opt == ':') {
            throw_exception<invalid_option_value>(invalid_option_value(options->argv[opc->idx - 2], ""));
        }

        if (-1 == opt) {
            //  no named parameter found
            //  might be a positional parameter
            if (opc->idx >= (int)options->argc) {
                //  end of command line reached
                break;
            }
            if (pos_option_index < positional_options->max_total_count()) {
                //  create an entry for positional parameter in variables map
                const std::string name = positional_options->name_for_position(pos_option_index++);
                std::shared_ptr<const option_description> desc = options->findByName(name);
                //  use defined value iff avalaible or create a new string value if not
                
                if (desc == nullptr)
                {
                    sem = std::shared_ptr<value_semantic>(value<std::string>());
                    sem->set_value(options->argv[opc->idx++]);
                }
                else
                {
                    value_semantic *v = (value_semantic *)(desc->semantic().get());
                    v->set_value(options->argv[opc->idx++]);
                    sem = std::shared_ptr<value_semantic>(v);
                }

                //  entry must not be present yet
                assert(vm.find(name) == vm.end());

                vm.insert(std::pair<std::string, std::shared_ptr<value_semantic>>(name, sem));

                continue;
            }
            else {
                too_many_positional_options_error e;

                throw_exception<too_many_positional_options_error>(e);
            }

            break;
        }

        std::shared_ptr<const option_description> desc = options->findById(opt);

        if (desc == nullptr) {
            opc->idx--;
            std::string name(options->argv[opc->idx]);
            unknown_option e(name);

            throw_exception<unknown_option>(e);
        }

        std::string name(desc->name());
        value_semantic *vs = (value_semantic *)(desc->semantic().get());
        bool bInsertNeeded = (vm.find(name) == vm.end());

        if (!(bInsertNeeded || vs->composing())) {
            std::string name(desc->format_name());
            multiple_occurrences e(name);

            throw_exception<multiple_occurrences>(e);
        }
        else {
            if (vs != NO_VALUE) {
                if (opc->arg) {
                    vs->set_value(opc->arg);
                }
                else if (vs->implicited()) {
                    vs->apply_implicit();
                }
            }
            if (bInsertNeeded) {
                sem = desc->semantic();
                vm.insert(std::pair<std::string, std::shared_ptr<value_semantic>>(name, sem));
            }
        }        
    }

    //  add options which have defaults and are not contained yet
    std::vector<std::shared_ptr<option_description>> opts = optionsDescriptions->options();
    for (std::shared_ptr<option_description> opt : opts) {
        std::string name(opt->name());
        sem = opt->semantic();

        if (sem != nullptr) {
            bool bInsertNeeded = (vm.find(name) == vm.end()) && sem->defaulted();

            if (bInsertNeeded) {
                vm.insert(std::pair<std::string, std::shared_ptr<value_semantic>>(name, sem));
            }
        }
    }   
    
    //  remember options for deconstrcution
    //  vm.register_options(options);
    delete options;
    
    o("store completed");
    
}

void notify(variables_map& vm)
{
    vm.notify();
}

const value_semantic
&variables_map::operator[](const std::string& name) const
{
    std::shared_ptr<const value_semantic> v = get(name);

    return *(v.get());
}

void variables_map::clear()
{
    std::map<std::string, std::shared_ptr<value_semantic>>::clear();
}

std::shared_ptr<const value_semantic>
variables_map::get(const std::string& name) const
{
    static std::shared_ptr<const value_semantic> empty;    
    const_iterator i = find(name);
    
    return (i == end()) ? empty 
                        : i->second;
}

void variables_map::notify()
{
    // Not implemented: checks if all required options occur

    // Lastly, run notify actions.
    for (auto& kv : *this) {
        if (kv.second != nullptr) {
            kv.second->notify();
        }
    }
}

void variables_map::show_options()
{
    std::cout << "Entries in variables_map: " << this->size() << std::endl;
    
    for (auto& kv : *this) {
        std::cout << kv.first << ": ";
                
        if (kv.second != nullptr) {
            std::cout << kv.second->to_string();
        }
        else {
            std::cout << " <null>";
        }
        std::cout << std::endl;
    }
    
    std::cout << std::endl;
}

}   //  end of namespace
