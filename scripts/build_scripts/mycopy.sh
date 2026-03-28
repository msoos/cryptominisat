#!/usr/bin/bash

set -x
set -e
scp ../scripts/crystal/*.py  \
    nscc:/home/projects/11000744/matesoos/cryptominisat5-predict-main
scp ../scripts/output_parser/create_solvetimes.sh ../scripts/output_parser/concat_files.py ../scripts/output_parser/solved_with_options.sh ../scripts/output_parser/solvetimes_from_output_glucose.sh ../scripts/output_parser/solvetimes_from_output.sh \
    nscc://home/projects/11000744/matesoos/cryptominisat5-main/
