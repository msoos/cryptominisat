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
 *
 * This routine was inspired by getopt_long.
 * It is gratefully acknowleged that this source includes software 
 * developed by the University of California, Berkeley and its contributors.
 *
 * ===============================================================
 *
 * Changes of the original: 
 * Arguments starting with '-' are treated as no-argument.
 * This is relevant for options with implicit values.
 * Compiled as c++ rather than c
 * Read option descriptions directly from the options_description object
 * error handling done by caller
 * status information is kept in one OptionParserControl struct
 * 
 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scan_arguments.h"
#include "options_description.h"

#define UNKNOWN_OPTION_KEY   (int)'?'
#define MISSING_ARGUMENT     (int)':'
#define END_OF_TOKEN    ""

#define FOUND_DOUBLE_DASH -2

namespace ak_program_options
{
    /*
     *  Parse argc/argv argument vector for short options with single dash.
     *  Return the key for the recognized option
     */
    static int scan_arguments_short(int argc, char **argv, const options_description *optionsDescription)
    {
        static const char *currChar = END_OF_TOKEN;       //  track current parsing location here
        std::shared_ptr<const option_description> opt_descr;
        OptionParserControl *opc = &optionParserControl;

        assert(argv != NULL);

        if (!*currChar) {      // update scanning pointer
            if (opc->idx >= argc || 
                (*(currChar = argv[opc->idx]) != '-')) {
                currChar = END_OF_TOKEN;
                return (-1);
            }
            if (currChar[1] && (*(++currChar) == '-')) {  // found "--" 
                currChar = END_OF_TOKEN;
                return FOUND_DOUBLE_DASH;
            }
        }                   // option letter okay? 
        if ((opc->key = (int)*(currChar++)) == (int)':' ||
            !(opt_descr = optionsDescription->findById(opc->key))) {
            /*
             * if the user didn't specify '-' as an option,
             * assume it means -1.
             */
            if (opc->key == (int)'-') {
                return (-1);
            }
            if (!*currChar) {
                opc->idx++;
            }
            return (UNKNOWN_OPTION_KEY);
        }
        long_option_struct *option = opt_descr->long_option();
        if (option->has_arg != Has_Argument::Required) { 
            opc->arg = NULL;
            if (!*currChar) {
                opc->idx++;
            }
        }
        else {                // argument is required 
            if (*currChar) {  //  not at token end yet
                opc->arg = (char *)currChar;
            }
            else if (argc <= ++(opc->idx)) {   // tokens exhausted
                currChar = END_OF_TOKEN;
                return MISSING_ARGUMENT;
            }
            else {             // gap
                opc->arg = argv[opc->idx];
            }
            currChar = END_OF_TOKEN;
            ++(opc->idx);
        }

        return opc->key;    //  return short option identifier
    }

    /*
     * scan_arguments -- Parse argc/argv commandline tokens
     *
     * Use list of options to match short and long options
     */
    int scan_arguments(int argc, char **argv, const options_description *optionsDescription)
    {
        OptionParserControl &opc = optionParserControl;
        assert(argv);

        int retval = scan_arguments_short(argc, argv, optionsDescription);

        if (retval == FOUND_DOUBLE_DASH) {
            char *current_argv = argv[opc.idx++] + 2;
            char *contains_eq;
            std::shared_ptr<const option_description> opt_descr;

            if (*current_argv == '\0') {
                return(-1);
            }
            if ((contains_eq = strchr(current_argv, '=')) != NULL) {
                std::string name = std::string(current_argv).substr(0, contains_eq - current_argv);
                opt_descr = optionsDescription->findByName(name);
                //  point to value begind '='
                contains_eq++;
            }
            else
            {
                opt_descr = optionsDescription->findByName(current_argv);
            }

            const long_option_struct *long_opt = nullptr;

            if (opt_descr) {
                long_opt = opt_descr->long_option();
                assert(long_opt);
                opc.arg = NULL;

                if (long_opt->has_arg != Has_Argument::No) {
                    opc.arg = contains_eq ? contains_eq : argv[opc.idx++];

                    //  quick fix to handle optional argument which is not present
                    if ((opc.arg) && (*(opc.arg) == '-')) {
                        if (long_opt->has_arg == Has_Argument::Optional) {
                            opc.idx--;
                            opc.arg = NULL;
                        }
                    }
                }
                else if (contains_eq) {
                    //  an argument was specified, although no arg is expected
                    opc.arg = contains_eq;
                }
                if ((long_opt->has_arg == Has_Argument::Required) && !opc.arg) {
                    delete long_opt;
                    return (MISSING_ARGUMENT);
                }
            }
            else { // No matching argument
                return (UNKNOWN_OPTION_KEY);
            }
            //  indicate which option was found
            retval = long_opt->val;
            delete long_opt;
        }

        return retval;
    }

}  //  end of namespace

