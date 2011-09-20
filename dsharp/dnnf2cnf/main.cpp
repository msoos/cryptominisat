#include <iostream>
#include <fstream>
#include <string>
#include <boost/foreach.hpp>
#include "dnnf2cnf.hpp"

#define foreach BOOST_FOREACH
using namespace std;

void output_cnf(ostream& os, vector<vector<int> > const& cnf)
{
  int nv = 0;
  foreach( vector<int> const& cl, cnf) {
    foreach( int l, cl )
      nv = max( nv, abs(l) );
  }

  os << "p cnf " << nv << ' ' << cnf.size() << "\n";
  foreach( vector<int> const& cl, cnf) {
    foreach( int l, cl )
      os << l << ' ';
    os << "0\n";
  }
}

int main(int argc, char **argv)
{
  if( argc != 2 ) {
    cout << "provide nnf filename\n";
    return 1;
  }

  string nnfname = argv[1];
  string cnfname = nnfname;
  size_t extpos = cnfname.rfind(".nnf");
  cnfname.replace(extpos, 4, ".cnf");

  cout << cnfname << endl;

  ifstream ifs(nnfname.c_str());
  if( !ifs ) {
    cout << "could not open \"" << nnfname << "\"\n";
    return 1;
  }
  try {
    nnf n;
    n = readnnf(ifs);
    vector< vector<int> > cnf;
    map<int, int> vm;
    int k = dnnf2cnf(n, cnf, vm, n.nvars+1, false);
    cout << cnf.size() << " clauses in cnf encoding\n";
    cout << k-1 << " total vars\n";

    ofstream ofs(cnfname.c_str());
    if( !ofs ) {
      cout << "could not open \"" << cnfname << "\"\n";
      return 1;
    }
    output_cnf(ofs, cnf);
  } catch(nnf_read_error) {
    cout << "blblbl\n";
  } catch(nnf_not_smooth) {
    cout << "cannot enforce gac on non-smooth nnf\n";
  }

  return 0;
}
