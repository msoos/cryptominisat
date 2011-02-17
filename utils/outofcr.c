/*****************************************************************************
intocr -- Copyright (c) 2011 Martin Maurer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "assert.h"


long *variables;

int main(int argc, char** argv)
{
  unsigned long nr_of_fixed_variables;
  unsigned long nr_of_total_variables;
  unsigned long current_nr_of_variable;
  unsigned long i;
  char result_string[1024];
  FILE *infile;
  long literal;
  unsigned long offset;

  FILE* infile2;
  if (argc == 1) {
      infile2 = stdin;
      printf("c reading from stdin\n");
  } else {
      infile2 = fopen(argv[1], "r");
      assert(infile2 != NULL);
  }

  fgets(result_string, sizeof(result_string) - 1, infile2);

  if(strcmp(result_string, "s UNSATISFIABLE\n") == 0)
  {
    printf("%s", result_string);
    exit(1);
  }

  infile = fopen("outfile.tmp", "r" );

  if(fscanf(infile, "p tmp %lu %lu\n", &nr_of_fixed_variables, &nr_of_total_variables) != 2)
  {
    fprintf(stderr, "error: expected: p tmp <nr_of_fixed_variables> <nr_of_total_variables>\n");
    exit(1);
  }

  variables     = (long *)malloc((nr_of_total_variables + 1) * sizeof(long));
  /*if(variables == 0)
  {
    fprintf(stderr, "error allocating memory for variables\n");
    exit(1);
  }*/

  for(i = 1; i <= nr_of_total_variables; i++)
  {
    variables[i] = 0; // means undef
  }

  for(current_nr_of_variable = 1; current_nr_of_variable <= nr_of_fixed_variables; current_nr_of_variable++)
  {
    if(fscanf(infile, "%ld 0\n", &literal) != 1)
    {
      fprintf(stderr, "error format clause nr. %lu\n", current_nr_of_variable);
      exit(1);
    }

    variables[(literal > 0 ? literal : -literal)] = literal;
  }

  printf("%s", result_string);

  fscanf(infile2, "v");
  printf("v");

  offset = 1;

  for(current_nr_of_variable = 1; current_nr_of_variable <= nr_of_total_variables; current_nr_of_variable++)
  {
      //printf("var: %ld\n", current_nr_of_variable);
    if(variables[current_nr_of_variable] != 0)
    {
      printf(" %ld", variables[current_nr_of_variable]);
      offset++;
    }
    else
    {
        //printf("max var: %ld\n", nr_of_total_variables);
      if(fscanf(infile2, " %ld", &literal) != 1)
      {
        fprintf(stderr, "error format solution (1)\n");
        exit(1);
      }

      if(literal == 0)
      {
        fprintf(stderr, "error format solution (2)\n");
        exit(1);
      }

      printf(" %ld", (literal > 0 ? 1 : -1) * ((literal > 0 ? (literal+offset-1) : (-literal+offset-1))));
    }
  }

/*char tmp[1000];
fscanf(infile2, "%s", tmp);
printf("cicc:%s\n", tmp);
exit(-1);*/
  if(fscanf(infile2, " %ld", &literal) != 1)
  {
    fprintf(stderr, "error format solution (3)\n");
    exit(1);
  }

  if(literal != 0)
  {
    fprintf(stderr, "error format solution (4)\n");
    exit(1);
  }

  printf(" 0\n");
  return 0;
}
