//
// SharpSAT-TD -- Copyright (c) 2021 Tuukka Korhonen
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <cassert>
#include <limits>
#include <algorithm>
#include <chrono>
#include <random>

namespace sspp {
typedef int Lit;
typedef int Var;

using std::vector;
using std::string;

inline Lit Neg(Lit x) {
	return x^1;
}

inline Var VarOf(Lit x) {
	return x/2;
}

inline Lit PosLit(Var x) {
	return x*2;
}

inline Lit NegLit(Var x) {
	return x*2+1;
}

inline Lit MkLit(Var var, bool phase) {
	if (phase) {
		return PosLit(var);
	} else {
		return NegLit(var);
	}
}

inline bool IsPos(Lit x) {
	return !(x&1);
}

inline bool IsNeg(Lit x) {
	return x&1;
}

inline vector<Lit> Negate(vector<Lit> vec) {
	for (Lit& lit : vec) {
		lit = Neg(lit);
	}
	return vec;
}

template<typename T>
inline T RandInt(T a, T b, std::mt19937& gen) {
	return std::uniform_int_distribution<T>(a,b)(gen);
}

template<typename T>
void SwapDel(vector<T>& vec, size_t i) {
	assert(i < vec.size());
	std::swap(vec[i], vec.back());
	vec.pop_back();
}

} // namespace sspp
