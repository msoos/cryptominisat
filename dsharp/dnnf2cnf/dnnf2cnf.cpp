#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <set>
#include <boost/foreach.hpp>
#include "assert.h"

#include "dnnf2cnf.hpp"

#define foreach BOOST_FOREACH
using namespace std;
using namespace nnfns;

nnf readnnf(istream& is)
{

  nnf rv;
  string line;
  getline(is, line);

  string kw;
  int v, e, n;
  {
    istringstream iss(line);
    iss >> kw >> n >> e >> v;
    if( kw != "nnf" || !iss )
      throw nnf_read_error();
  }

  rv.nodes.resize(n);
  rv.nvars = v;

  for(int i = 0; i != n; ++i) {
    getline(is, line);
    if( !is ) return rv;

    istringstream iss(line);
    char t;
    iss >> t;
    switch(t) {
    case 'L': // leaf
      {
        int lit;
        iss >> lit;
        rv.nodes[i] = new node(node::LEAF, lit);
        break;
      }
    case 'A': // and
      {
        int nc;
        iss >> nc;
        rv.nodes[i] = new node(node::AND);
        node *cn = rv.nodes[i];
        cn->children.resize(nc);
        for(int j = 0; j != nc; ++j) {
          int c;
          iss >> c;
          if( c >= i ) throw nnf_read_error();
          cn->children[j] = rv.nodes[c];
        }
        break;
      }
    case 'O': // or
      {
        int nc, var;
        iss >> var >> nc;
        rv.nodes[i] = new node(node::OR, var);
        node *cn = rv.nodes[i];
        cn->children.resize(nc);
        for(int j = 0; j != nc; ++j) {
          int c;
          iss >> c;
          if( c >= i ) throw nnf_read_error();
          cn->children[j] = rv.nodes[c];
        }
        break;
      }
    default:
      throw nnf_read_error();
    }
  }

  rv.root = rv.nodes.back();

  return rv;
};

bool is_smooth2(node *n, set<int> &v)
{
  if( n->t == node::LEAF ) {
    v.insert(abs(n->var));
    return true;
  }

  if( n->t == node::AND ) {
    foreach( node *c, n->children ) {
      if( !is_smooth2(c, v) )
        return false;
    }
    return true;
  }

  if( n->t == node::OR ) {
    if( n->children.empty() ) return true;

    set<int> vc;
    if( !is_smooth2( n->children[0], vc ) )
      return false;
    v.insert(vc.begin(), vc.end());
    for(size_t i = 1; i != n->children.size(); ++i) {
      set<int> vc2;
      if( !is_smooth2( n->children[i], vc2 ) )
        return false;
      if( vc != vc2 )
        return false;
    }
    return true;
  }

  assert(false);
  return true;
}

bool is_smooth(nnf &f)
{
  set<int> v;
  return is_smooth2(f.root, v);
}

int dnnf2cnf(nnf& f, std::vector< std::vector<int> >& cnf,
             map<int, int>& varmap, int newindex,
             bool gac)
{
  cout << "nnf with " << f.nodes.size() << " nodes\n";

  if( gac && !is_smooth(f) )
    throw nnf_not_smooth();

  map<node*, int> nodevars;
  foreach(node* n, f.nodes) {
    if( n->t == node::LEAF ) {
      nodevars[n] = n->var;
      if( varmap.find(abs(n->var)) == varmap.end() ) {
        varmap[n->var] = n->var;
        varmap[-n->var] = -n->var;
      }
      if( n->var < 0 ) {
        varmap[n->var] = -varmap[-n->var];
      }
    } else {
      nodevars[n] = newindex;
      varmap[ newindex ] = newindex;
      ++newindex;
    }
  }

  foreach(node *n, f.nodes) {
    if( n->t == node::AND ) {
      // cl2: -n -> -c1 \/ ... \/ -ck
      vector<int> cl2;
      cl2.push_back( varmap[nodevars[n]] );
      foreach(node *c, n->children) {
        // cl: -ci -> -n for all i
        vector<int> cl;
        cl.push_back( varmap[nodevars[c]] );
        cl.push_back( -varmap[nodevars[n]] );
        cnf.push_back(cl);

        cl2.push_back( -varmap[nodevars[c]] );
      }
      cnf.push_back(cl2);
    } else if( n->t == node::OR ) {
      // cl: -c1 /\ ... /\ -ck -> -n
      vector<int> cl;
      foreach(node *c, n->children) {
        cl.push_back(varmap[nodevars[c]] );

        // cl2: -n -> -ci for all i
        vector<int> cl2;
        cl2.push_back( varmap[ nodevars[n]] );
        cl2.push_back( - varmap[ nodevars[c]] );
        cnf.push_back(cl2);
      }
      cl.push_back( -varmap[nodevars[n]] );
      cnf.push_back(cl);
    }
  }

  if(gac) {
    map<node *, vector<node*> > parents;
    foreach(node *n, f.nodes) {
      foreach(node *c, n->children)
        parents[c].push_back(n);
    }

    map<node *, int> pdvars;
    foreach(node *n, f.nodes ) {
      if( n->t == node::LEAF ) {
        pdvars[n] = nodevars[n];
      } else {
        pdvars[n] = newindex;
        varmap[ newindex ] = newindex;
        ++newindex;
      }
    }

    foreach( node *n, f.nodes ) {
      // cl: -p1 /\ ... /\ -pk -> -n
      vector<int> cl;
      if( parents[n].empty() ) continue;
      foreach( node *p, parents[n] ) {
        cl.push_back( varmap[pdvars[p]] );
      }
      cl.push_back( -varmap[pdvars[n]] );
      cnf.push_back(cl);

      if( n->t == node::AND ) {
        vector<int> cl2;
        cl2.push_back( varmap[nodevars[n]] );
        cl2.push_back( -varmap[pdvars[n]] );
        cnf.push_back(cl2);
      }
    }

    vector<int> pdroot;
    pdroot.push_back( varmap[pdvars[f.root]] );
    cnf.push_back(pdroot);
  }

  vector<int> trueroot;
  trueroot.push_back( varmap[nodevars[f.root]] );
  cnf.push_back(trueroot);

  return newindex;
}
