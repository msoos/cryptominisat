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
long *new_variables;
unsigned long *clause_length;
long **clauses;
long *one_clause;

int main(int argc, char** argv)
{
  unsigned long nr_of_variables, nr_of_clauses;
  unsigned long nr_of_new_variables, nr_of_fixed_variable, nr_of_new_clauses;
  unsigned long current_nr_of_clause;
  unsigned long current_nr_of_literal;
  unsigned long preallocated_space_for_literals;
  unsigned long i, j;

  FILE * outfile;
  FILE * infile;

  long literal;
  long factor;
  char needOutFile = 1;


  nr_of_new_clauses = 0;

  if (argc == 1) {
      infile = stdin;
      //infile = fdopen(0, "r");
      printf("c reading from stdin\n");
  } else {
      if (argv[1][0] == 'x') {
          needOutFile = 0;
          infile = stdin;
      } else {
        infile = fopen(argv[1], "r");
        assert(infile != NULL);
      }
  }

  if(fscanf(infile, "p cnf %lu %lu\n", &nr_of_variables, &nr_of_clauses) != 2)
  {
    fprintf(stderr, "error: expected: p cnf <variables> <clauses>\n");
    exit(1);
  }
  printf("c num vars: %ld\n", nr_of_variables);
  printf("c num clauses: %ld\n", nr_of_clauses);

  variables     = (long *)malloc((nr_of_variables + 1) * sizeof(long));
  /*if(variables == 0)
  {
    fprintf(stderr, "error allocating memory for variables\n");
    exit(1);
  }*/

  new_variables     = (long *)malloc((nr_of_variables + 1) * sizeof(long));
  /*if(new_variables == 0)
  {
    fprintf(stderr, "error allocating memory for new_variables\n");
    exit(1);
  }*/

  for(i = 1; i <= nr_of_variables; i++)
  {
    variables[i] = new_variables[i] = 0; // means undef
  }

  clause_length = (unsigned long *)malloc(nr_of_clauses * sizeof(unsigned long));
  /*if(clause_length == 0)
  {
    fprintf(stderr, "error allocating memory for clause_length\n");
    exit(1);
  }*/

  clauses = (long **)malloc(nr_of_clauses * sizeof(unsigned long *));
  /*if(clauses == 0)
  {
    fprintf(stderr, "error allocating memory for clauses\n");
    exit(1);
  }*/

  preallocated_space_for_literals = 128;
  one_clause = (long *)malloc(preallocated_space_for_literals * sizeof(long));

  for(current_nr_of_clause = 0; current_nr_of_clause < nr_of_clauses; current_nr_of_clause++)
  {
    current_nr_of_literal = 0;

    while(1)
    {
      if(fscanf(infile, "%ld", &literal) != 1)
      {
        fprintf(stderr, "error reading file (clause %lu)\n", current_nr_of_clause + 1);
        exit(1);
      }

      if(literal == 0) break;

      one_clause[current_nr_of_literal++] = literal;

      if(current_nr_of_literal == preallocated_space_for_literals)
      {
        preallocated_space_for_literals *= 2;
        one_clause = (long *)realloc(one_clause, preallocated_space_for_literals * sizeof(long));
      }
    }

    fscanf(infile, "\n");

    if(current_nr_of_literal == 1)
    {
      variables[abs(one_clause[0])] = one_clause[0];
    }
    else
    {
      clause_length[nr_of_new_clauses] = current_nr_of_literal;
      clauses[nr_of_new_clauses] = (long *)malloc(current_nr_of_literal * sizeof(long));
      memcpy(clauses[nr_of_new_clauses], one_clause, current_nr_of_literal * sizeof(long));
      nr_of_new_clauses++;
    }
  }

  printf("c finished reading CNF\n");

  // Re-Numbering
  for(i = 1,nr_of_new_variables = 0,nr_of_fixed_variable=0; i <= nr_of_variables; i++)
  {
    if(variables[i] == 0) // means undef
    {
      nr_of_new_variables++;
      new_variables[i] = nr_of_new_variables;
    }
    else
    {
      nr_of_fixed_variable++;
    }
  }

  if (needOutFile) {
    outfile = fopen("outfile.tmp", "w" );
    fprintf(outfile, "p tmp %lu %lu\n", nr_of_fixed_variable, nr_of_variables);

    for(i = 1; i <= nr_of_variables; i++)
    {
        if(variables[i] != 0) // means undef
        {
        fprintf(outfile, "%ld 0\n", variables[i]);
        }
    }

    fclose(outfile);
  }

  printf("p cnf %lu %lu\n", nr_of_new_variables, nr_of_new_clauses);

  long lit;
  for(current_nr_of_clause = 0; current_nr_of_clause < nr_of_new_clauses; current_nr_of_clause++)
  {
    for(current_nr_of_literal = 0; current_nr_of_literal < clause_length[current_nr_of_clause]; current_nr_of_literal++)
    {
      lit = clauses[current_nr_of_clause][current_nr_of_literal];
      factor  = 1;
      if (lit < 0) {
          factor = -1;
      }
      if (variables[abs(lit)] != 0) {
          printf("clause: ");
          for (j = 0; i < clause_length[current_nr_of_clause]; i++) {
              printf("%ld ", clauses[current_nr_of_clause][i]);
          }
          printf("0\n");
        assert(variables[abs(lit)] == 0);
      }
      printf("%ld ", ((long int)new_variables[abs(lit)])*factor);
    }
    printf("0\n");
    free(clauses[current_nr_of_clause]);
  }

  free(variables);
  free(new_variables);
  free(clause_length);
  free(clauses);
  free(one_clause);

  return 0;
}
