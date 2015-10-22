#include "cryptominisat4/solvertypesmini.h"
#include <vector>
#include <ostream>
#include <iostream>
#include <sstream>

using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::istringstream;
using namespace CMSat;

// trim from start
static inline std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
}

vector<Lit> str_to_cl(const string& data)
{
    istringstream iss(data);
    vector<string> tokens;
    std::copy(std::istream_iterator<string>(iss),
        std::istream_iterator<string>(),
        back_inserter(tokens));

    vector<Lit> ret;
    for(string& token: tokens) {
        int i = std::stoi(trim(token));
        Lit lit(abs(i)-1, i < 0);
        ret.push_back(lit);
    }
    return ret;
}

// string print(const vector<Lit>& dat) {
//     std::stringstream m;
//     for(size_t i = 0; i < dat.size();) {
//         m << dat[i];
//         i++;
//         if (i < dat.size()) {
//             m << ", ";
//         }
//     }
//     return m.str();
// }
