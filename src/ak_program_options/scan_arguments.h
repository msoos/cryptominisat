#ifndef __AKPO_GETOPT_H_INCLUDED_
#define __AKPO_GETOPT_H_INCLUDED_
//
//  akpo_getopt.h  -  renamed to avoid clash with other getopt.h includes
//

namespace ak_program_options
{

struct option_parser_control
{
    int idx;      //  index into parent argv vector 
    int key;      //  character checked for validity
    char *arg;    //  argument associated with option */
};

typedef struct option_parser_control OptionParserControl;

extern OptionParserControl optionParserControl;

enum Has_Argument { No, Required, Optional };

struct _long_option_struct
{
    const char *name;
    Has_Argument has_arg;
    int val;            /*  either the short_option char or a unique hash to recognize the option */
};

typedef struct _long_option_struct long_option_struct;

#define no_argument       0
#define required_argument 1
#define optional_argument 2

class options_description;

int ak_getopt_long(int nargc, char **nargv, const options_description *optionsDescription);

}
#endif /* __AKPO_GETOPT_H_INCLUDED_ */
