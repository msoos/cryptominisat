#include <iostream>
#include <vector>
#include <map>

namespace nnfns {
  struct node {
    enum type { AND, OR, LEAF };
    type t;
    int var;
    std::vector<node*> children;

    node() {}
    node(type _t) : t(_t) {}
    node(type _t, int _var) : t(_t), var(_var) {}
  };
}

struct nnf_read_error {};
struct nnf_not_smooth {};

struct nnf {
  std::vector<nnfns::node*> nodes;
  nnfns::node *root;
  int nvars;
};

nnf readnnf(std::istream& is);

/* decomposes an nnf to cnf, optionally enforcing gac

   nnf: input

   cnf: vector of clauses. each clause is a vector<int> with x or -x

   varmap: a map from variable indexes in the nnf to variable indexes
           in the output cnf, i.e., if varmap[5] = 269, it means that
           when the nnf refers to variable 5, the output cnf will
           refer to variable 269. If empty/uninitialized, uses the
           identify map

   newindex: an unused variable index. assumes indices newindex+1,
             newindex+2, ... are also free

   gac: if the nnf is smooth, will output a cnf that enforces gac

   returns newindex+k where k is the number of new variables is has used
   to encode the nnf
*/
int dnnf2cnf(nnf& n, std::vector< std::vector<int> >& cnf,
             std::map<int, int>& varmap, int newindex,
             bool gac);
