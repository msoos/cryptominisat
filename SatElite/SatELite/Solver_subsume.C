#include "Solver.h"
#include "Main.h"
#include "Sort.h"
#include "BcnfWriter.iC"

#define Report(format, args...) ((verbosity >= 1) ? reportf(format , ## args) : 0)
//#define Report(format, args...)
#define Report2(format, args...) ((verbosity >= 2) ? reportf(format , ## args) : 0)


macro bool has(Clause c, Lit p) {
    for (int i = 0; i < c.size(); i++)
        if (c[i] == p) return true;
    return false; }


#if 1
// Assumes 'seen' is cleared (will leave it cleared)
static
bool subset(Clause A, Clause B, vec<char>& seen)
{
    for (int i = 0; i < B.size(); i++) seen[index(B[i])] = 1;
    for (int i = 0; i < A.size(); i++){
        if (!seen[index(A[i])]){
            for (int i = 0; i < B.size(); i++) seen[index(B[i])] = 0;
            return false;
        }
    }
    for (int i = 0; i < B.size(); i++) seen[index(B[i])] = 0;
    return true;
}
#else
static
bool subset(Clause A, Clause B, vec<char>& seen)
{
    for (int i = 0; i < A.size(); i++)
        if (!has(B, A[i]))
            return false;
    return true;
}
#endif


macro bool subset(uint64 A, uint64 B) { return (A & ~B) == 0; }


// Assumes 'seen' is cleared (will leave it cleared)
static
bool selfSubset(Clause A, Clause B, vec<char>& seen)
{
    for (int i = 0; i < B.size(); i++) seen[index(B[i])] = 1;

    bool    flip = false;
    for (int i = 0; i < A.size(); i++){
        if (!seen[index(A[i])]){
            if (flip == true || !seen[index(~A[i])]){
                for (int i = 0; i < B.size(); i++) seen[index(B[i])] = 0;
                return false;
            }
            flip = true;
        }
    }
    for (int i = 0; i < B.size(); i++) seen[index(B[i])] = 0;
    return flip;
}


macro bool selfSubset(uint64 A, uint64 B)
{
    uint64 B_tmp = B | ((B & 0xAAAAAAAAAAAAAAAALL) >> 1) | ((B & 0x5555555555555555LL) << 1);
    if ((A & ~B_tmp) == 0){
        uint64 C = A & ~B;
        return (C & (C-1)) == 0;
    }else
        return false;
}


void Solver::findSubsumed(Clause ps, vec<Clause>& out_subsumed)
{
    uint64  abst;
    if (ps.dynamic()){
        abst = 0;
        for (int i = 0; i < ps.size(); i++)
            abst |= abstLit(ps[i]);
    }else
        abst = ps.abst();

    int min_i = 0;
    for (int i = 1; i < ps.size(); i++){
        if (occur[index(ps[i])].size() < occur[index(ps[min_i])].size())
            min_i = i;
    }

    vec<Clause>& cs = occur[index(ps[min_i])];
    for (int i = 0; i < cs.size(); i++){
        if (cs[i] != ps && ps.size() <= cs[i].size() && subset(abst, cs[i].abst()) && subset(ps, cs[i], seen_tmp))
            out_subsumed.push(cs[i]);
    }
}


bool Solver::isSubsumed(Clause ps)
{
    uint64  abst;
    if (ps.dynamic()){
        abst = 0;
        for (int i = 0; i < ps.size(); i++)
            abst |= abstLit(ps[i]);
    }else
        abst = ps.abst();

    for (int j = 0; j < ps.size(); j++){
        vec<Clause>& cs = occur[index(ps[j])];

        for (int i = 0; i < cs.size(); i++){
            if (cs[i] != ps && cs[i].size() <= ps.size() && subset(cs[i].abst(), abst)){
                if (subset(cs[i], ps, seen_tmp))
                    return true;
            }
        }
    }
    return false;
}


// Will put NULL in 'cs' if clause removed.
void Solver::subsume0(Clause ps, int& counter)
{
    if (!opt_0sub) return;

    vec<Clause>    subs;
    findSubsumed(ps, subs);
    for (int i = 0; i < subs.size(); i++){
        if (&counter != NULL) counter++;
        removeClause(subs[i]);
    }
}


#if 1
// With queue
void Solver::subsume1(Clause ps, int& counter)
{
    if (!opt_1sub) return;

    vec<Clause>    Q;
    vec<Clause>    subs;
    Clause_t       qs;
    int            q;

    registerIteration(Q);
    registerIteration(subs);

    Q.push(ps);
    q = 0;
    while (q < Q.size()){
        if (Q[q].null()) { q++; continue; }
        qs.clear();
        for (int i = 0; i < Q[q].size(); i++)
            qs.push(Q[q][i]);

        for (int i = 0; i < qs.size(); i++){
            qs[i] = ~qs[i];
            findSubsumed(qs, subs);
            for (int j = 0; j < subs.size(); j++){
                /*DEBUG*/
              #ifndef NDEBUG
                if (&counter != NULL && counter == -1){
                    dump(subs[j]);
                    qs[i] = ~qs[i];
                    dump(qs);
                    printf(L_LIT"\n", L_lit(qs[i]));
                    exit(0);
                }
              #endif
                /*END*/
                if (subs[j].null()) continue;
                if (&counter != NULL) counter++;
                strengthenClause(subs[j], qs[i]);
                Q.push(subs[j]);
            }

            qs[i] = ~qs[i];
            subs.clear();
        }
        q++;
    }

    unregisterIteration(Q);
    unregisterIteration(subs);
}
#else
// Without queue
void Solver::subsume1(Clause ps, int& counter)
{
    if (!opt_1sub) return;

    vec<Clause>    subs;
    Clause_t       qs;

    registerIteration(subs);

    assert(!ps.null());
    for (int i = 0; i < ps.size(); i++)
        qs.push(ps[i]);

    for (int i = 0; i < qs.size(); i++){
        qs[i] = ~qs[i];
        findSubsumed(qs, subs);
        for (int j = 0; j < subs.size(); j++){
            if (subs[j].null()) continue;
            if (&counter != NULL) counter++;
            strengthenClause(subs[j], qs[i]);
        }

        qs[i] = ~qs[i];
        subs.clear();
    }
    unregisterIteration(subs);
}
#endif


void Solver::simplifyBySubsumption(bool with_var_elim)
{
    propagateToplevel(); if (!ok){ Report("(contradiction during subsumption)\n"); return; }

    int     orig_n_clauses  = nClauses();
    int     orig_n_literals = nLiterals();
    do{
        // SUBSUMPTION:
        //
      #ifndef SAT_LIVE
        Report("  -- subsuming                       \r");
      #endif
        int     clauses_subsumed = 0, literals_removed = 0;
        if (!opt_0sub && !opt_1sub){
            cl_added  .clear();
            cl_touched.clear();
            goto NoSubsumption; }

        if (cl_added.size() > nClauses() / 2){
            // Optimized variant when virtually whole database is involved:
            cl_added  .clear();
            cl_touched.clear();

            for (int i = 0; i < constrs.size(); i++) if (!constrs[i].null()) subsume1(constrs[i], literals_removed);
            propagateToplevel(); if (!ok){ Report("(contradiction during subsumption)\n"); return; }

            CSet s1;
            registerIteration(s1);
            while (cl_touched.size() > 0){
                for (int i = 0; i < cl_touched.size(); i++)
                    if (!cl_touched[i].null())
                        s1.add(cl_touched[i]);
                cl_touched.clear();

                for (int i = 0; i < s1.size(); i++) if (!s1[i].null()) subsume1(s1[i], literals_removed);
                s1.clear();

                propagateToplevel();
                if (!ok){ Report("(contradiction during subsumption)\n"); unregisterIteration(s1); return; }
            }
            unregisterIteration(s1);

            for (int i = 0; i < constrs.size(); i++) if (!constrs[i].null()) subsume0(constrs[i], clauses_subsumed);

        }else{
            //  Set used in 1-subs:
            //      (1) clauses containing a negated literal of an added clause.
            //      (2) all added or strengthened ("touched") clauses.
            //
            //  Set used in 0-subs:
            //      (1) clauses containing a (non-negated) literal of an added clause, including the added clause itself.
            //      (2) all strenghtened clauses -- REMOVED!! We turned on eager backward subsumption which supersedes this.

//Report("  PREPARING\n");
            CSet        s0, s1;     // 's0' is used for 0-subsumption, 's1' for 1-subsumption
            vec<char>   ol_seen(nVars()*2, 0);
            for (int i = 0; i < cl_added.size(); i++){
                Clause  c = cl_added[i]; if (c.null()) continue;
                s1.add(c);
                for (int j = 0; j < c.size(); j++){
                    if (ol_seen[index(c[j])]) continue;
                    ol_seen[index(c[j])] = 1;

                    vec<Clause>& n_occs = occur[index(~c[j])];
                    for (int k = 0; k < n_occs.size(); k++)     // <<= Bättra på. Behöver bara kolla 'n_occs[k]' mot 'c'
                        if (n_occs[k] != c && n_occs[k].size() <= c.size() && selfSubset(n_occs[k].abst(), c.abst()) && selfSubset(n_occs[k], c, seen_tmp))
                            s1.add(n_occs[k]);

                    vec<Clause>& p_occs = occur[index(c[j])];
                    for (int k = 0; k < p_occs.size(); k++)     // <<= Bättra på. Behöver bara kolla 'p_occs[k]' mot 'c'
                        if (subset(p_occs[k].abst(), c.abst()))
                            s0.add(p_occs[k]);
                }
            }
            cl_added.clear();

            registerIteration(s0);
            registerIteration(s1);

//Report("  FIXED-POINT\n");
            // Fixed-point for 1-subsumption:
            while (s1.size() > 0 || cl_touched.size() > 0){
                for (int i = 0; i < cl_touched.size(); i++)
                    if (!cl_touched[i].null())
                        s1.add(cl_touched[i]),
                        s0.add(cl_touched[i]);

                cl_touched.clear();
                assert(propQ.size() == 0);
//Report("s1.size()=%d  cl_touched.size()=%d\n", s1.size(), cl_touched.size());
                for (int i = 0; i < s1.size(); i++) if (!s1[i].null()){ subsume1(s1[i], literals_removed); }
                s1.clear();

                propagateToplevel();
                if (!ok){
                    Report("(contradiction during subsumption)\n");
                    unregisterIteration(s0);
                    unregisterIteration(s1);
                    return; }
                assert(cl_added.size() == 0);
            }
            unregisterIteration(s1);

            // Iteration pass for 0-subsumption:
            for (int i = 0; i < s0.size(); i++) if (!s0[i].null()) subsume0(s0[i], clauses_subsumed);
            s0.clear();
            unregisterIteration(s0);
        }

        if (literals_removed > 0 || clauses_subsumed > 0)
            Report2("  #literals-removed: %d    #clauses-subsumed: %d\n", literals_removed, clauses_subsumed);


        // VARIABLE ELIMINATION:
        //
      NoSubsumption:
        if (!with_var_elim || !opt_var_elim) break;

//Report("VARIABLE ELIMINIATION\n");
        vec<Var>    init_order;
        orderVarsForElim(init_order);   // (will untouch all variables)

        for (bool first = true;; first = false){
            int vars_elimed = 0;
            int clauses_before = nClauses();
            vec<Var>    order;

            if (opt_pure_literal){
                for (int i = 0; i < touched_list.size(); i++){
                    Var x = touched_list[i];
                    if (n_occurs[index(Lit(x))] == 0 && value(x) == l_Undef && !frozen[x] && !var_elimed[x] && n_occurs[index(~Lit(x))] > 0){
                        enqueue(~Lit(x));
                    }else if (n_occurs[index(~Lit(x))] == 0 && value(x) == l_Undef && !frozen[x] && !var_elimed[x] && n_occurs[index(Lit(x))] > 0){
                        enqueue(Lit(x));
                    }
                }
                propagateToplevel(); assert(ok);
            }

            if (first)
                init_order.copyTo(order);
            else{
                for (int i = 0; i < touched_list.size(); i++)
                    if (!var_elimed[touched_list[i]])
                        order.push(touched_list[i]),
                    touched[touched_list[i]] = 0;
                touched_list.clear();
            }

            assert(propQ.size() == 0);
            for (int i = 0; i < order.size(); i++){
              #ifndef SAT_LIVE
                if (i % 1000 == 999 || i == order.size()-1) Report("  -- var.elim.:  %d/%d          \r", i+1, order.size());
              #endif
                if (maybeEliminate(order[i])){
                    vars_elimed++;
                    propagateToplevel(); if (!ok){ Report("(contradiction during subsumption)\n"); return; }
                }
            }
            assert(propQ.size() == 0);

            if (vars_elimed == 0)
                break;

            Report2("  #clauses-removed: %-8d #var-elim: %d\n", clauses_before - nClauses(), vars_elimed);
        }
        //assert(touched_list.size() == 0);     // <<= No longer true, due to asymmetric branching. Ignore it for the moment...
//  }while (cl_added.size() > 0);
    }while (cl_added.size() > 100);

    if (orig_n_clauses != nClauses() || orig_n_literals != nLiterals())
        //Report("#clauses: %d -> %d    #literals: %d -> %d    (#learnts: %d)\n", orig_n_clauses, nClauses(), orig_n_literals, nLiterals(), nLearnts());
        //Report("#clauses:%8d (%6d removed)    #literals:%8d (%6d removed)    (#learnts:%8d)\n", nClauses(), orig_n_clauses-nClauses(), nLiterals(), orig_n_literals-nLiterals(), nLearnts());
        Report("| %9d | %7d %8d | %7s %7d %8s %7s | %6s   | %d/%d\n", (int)stats.conflicts, nClauses(), nLiterals(), "--", nLearnts(), "--", "--", "--", nClauses() - orig_n_clauses, nLiterals() - orig_n_literals);


    if (opt_pre_sat){
        // Compact variables:
        vec<Var>    vmap(nVars(), -1);
        int         c = 0;
        for (int i = 0; i < nVars(); i++)
            if (!var_elimed[i] && (occur[index(Lit(i))].size() != 0 || occur[index(~Lit(i))].size() != 0))
                vmap[i] = c++;

        Report("==============================================================================\n");
        Report("Result  :   #vars: %d   #clauses: %d   #literals: %d\n", c, nClauses(), nLiterals());
        Report("CPU time:   %g s\n", cpuTime());
        Report("==============================================================================\n");

        // Write CNF or BCNF file:
        cchar*  filename = (output_file == NULL) ? "pre-satelited.cnf" : output_file;
        int     len = strlen(filename);
        if (len >= 5 && strcmp(filename+len-5, ".bcnf") == 0){
            BcnfWriter w(filename);
            vec<Lit>   lits;
            for (int i = 0; i < constrs.size(); i++){
                Clause c = constrs[i]; if (c.null()) continue;
                lits.clear(); lits.growTo(c.size());
                for (int j = 0; j < c.size(); j++)
                    lits[j] = Lit(vmap[var(c[j])], sign(c[j]));
                w.addClause(lits);
            }
        }else{
            FILE*   out = (strcmp(filename, "-") == 0) ? stdout : fopen(filename, "wb");
            if (out == NULL) fprintf(stderr, "ERROR! Could not open output file: %s\n", filename), exit(1);

            fprintf(out, "c   #vars: %d   #clauses: %d   #literals: %d\n"  , c, nClauses(), nLiterals());
            fprintf(out, "c \n");
            fprintf(out, "p cnf %d %d\n", c, nClauses());
            for (int i = 0; i < constrs.size(); i++){
                Clause c = constrs[i]; if (c.null()) continue;
                for (int j = 0; j < c.size(); j++)
                    assert(vmap[var(c[j])]+1 != -1),
                    fprintf(out, "%s%d ", sign(c[j])?"-":"", vmap[var(c[j])]+1);
                fprintf(out, "0\n");
            }
            fclose(out);
        }

        // Write varible map:
        if (varmap_file != NULL){
            FILE*   out = fopen(varmap_file, "wb");
            if (out == NULL) fprintf(stderr, "ERROR! Could not open output file: %s\n", varmap_file), exit(1);

            fprintf(out, "%d 0\n", nVars());

            c = 0;
            for (int i = 0; i < nVars(); i++){
                if (!var_elimed[i] && value(i) != l_Undef){
                    c++;
                    fprintf(out, "%s%s%d", (c > 1)?" ":"", (value(i)==l_True)?"":"-", i+1);
                }
            }
            fprintf(out, " 0\n");

            c = 0;
            for (int i = 0; i < nVars(); i++){
                if (vmap[i] != -1){
                    assert(vmap[i] == c);
                    c++;
                    fprintf(out, "%s%d", (c > 1)?" ":"", i+1);
                }
            }
            fprintf(out, " 0\n");
            fclose(out);
        }
        fflush(elim_out);
        exit(0);
    }

//Report("DONE!\n");
}





//=================================================================================================
// Eliminate variables:


// Side-effect: Will untouch all variables.
void Solver::orderVarsForElim(vec<Var>& order)
{
    order.clear();
    vec<Pair<int, Var> > cost_var;
    for (int i = 0; i < touched_list.size(); i++){
        Var x = touched_list[i];
        touched[x] = 0;
        cost_var.push(Pair_new( occur[index(Lit(x))].size() * occur[index(~Lit(x))].size() , x ));
    }
    touched_list.clear();
    sort(cost_var);

    for (int x = 0; x < cost_var.size(); x++)
        if (cost_var[x].fst != 0)
            order.push(cost_var[x].snd);
}


#if 0
// Returns FALSE if clause is always satisfied ('out_clause' should not be used).
static
bool merge(Clause ps, Clause qs, Lit without_p, Lit without_q, vec<Lit>& out_clause)
{
    int     i = 0, j = 0;
    while (i < ps.size() && j < qs.size()){
        if      (ps[i] == without_p) i++;
        else if (qs[j] == without_q) j++;
        else if (ps[i] == ~qs[j])    return false;
        else if (ps[i] < qs[j])      out_clause.push(ps[i++]);
        else if (ps[i] > qs[j])      out_clause.push(qs[j++]);
        else                         out_clause.push(ps[i++]), j++;
    }
    while (i < ps.size()){
        if (ps[i] == without_p) i++;
        else                    out_clause.push(ps[i++]); }
    while (j < qs.size()){
        if (qs[j] == without_q) j++;
        else                    out_clause.push(qs[j++]); }
    return true;
}
#endif

// Returns FALSE if clause is always satisfied ('out_clause' should not be used). 'seen' is assumed to be cleared.
static
bool merge(Clause ps, Clause qs, Lit without_p, Lit without_q, vec<char>& seen, vec<Lit>& out_clause)
{
    for (int i = 0; i < ps.size(); i++){
        if (ps[i] != without_p){
            seen[index(ps[i])] = 1;
            out_clause.push(ps[i]);
        }
    }

    for (int i = 0; i < qs.size(); i++){
        if (qs[i] != without_q){
            if (seen[index(~qs[i])]){
                for (int i = 0; i < ps.size(); i++) seen[index(ps[i])] = 0;
                return false; }
            if (!seen[index(qs[i])])
                out_clause.push(qs[i]);
        }
    }

    for (int i = 0; i < ps.size(); i++) seen[index(ps[i])] = 0;
    return true;
}


Lit Solver::findUnitDef(Var x, vec<Clause>& poss, vec<Clause>& negs)
{
    vec<Lit>    imps;   // All p:s s.t. "~x -> p"
    for (int i = 0; i < poss.size(); i++){
        if (poss[i].size() == 2){
            if (var(poss[i][0]) == x)
                imps.push(poss[i][1]);
            else
                assert(var(poss[i][1]) == x),
                imps.push(poss[i][0]);
        }
    }

    // Quadratic algorithm; maybe should improve?
    for (int i = 0; i < negs.size(); i++){
        if (negs[i].size() == 2){
            Lit p = (var(negs[i][0]) == x ) ? ~negs[i][1] : (assert(var(negs[i][1]) == x), ~negs[i][0]);
            for (int j = 0; j < imps.size(); j++)
                if (imps[j] == p)
                    return ~imps[j];
                //**/else if (imps[j] == ~p)
                //**/    enqueue(~p);
        }
    }

    return lit_Undef;
}


// If returns TRUE 'out_def' is the definition of 'x' (always a disjunction).
// An empty definition means '~x' is globally true.
//
bool Solver::findDef(Lit x, vec<Clause>& poss, vec<Clause>& negs, Clause out_def)
{
    assert(out_def.size() == 0);
    Clause_t    imps;       // (negated implied literals, actually)
    uint64      abst = 0;

    for (int i = 0; i < negs.size(); i++){
        if (negs[i].size() == 2){
            Lit imp;
            if (negs[i][0] == ~x)
                imp = ~negs[i][1];
            else
                assert(negs[i][1] == ~x),
                imp = ~negs[i][0];

            imps.push(imp);
            abst |= abstLit(imp);
        }
    }

    if (opt_hyper1_res && isSubsumed(imps))
        return true;
    if (!opt_def_elim)
        return false;

    imps.push(x);
    abst |= abstLit(x);

    for (int i = 0; i < poss.size(); i++){
        if (subset(poss[i].abst(), abst) && subset(poss[i], imps, seen_tmp)){
            Clause c = poss[i];

            if (out_def.size() > 0 && c.size()-1 < out_def.size())
                //**/printf("----------\nImps  : "), dump(imps),printf("Before: "), dump(out_def), printf("Now   : "), dump(c, false), printf("  -- minus "L_LIT"\n", L_lit(x));
                out_def.clear();    // (found a better definition)
            if (out_def.size() == 0){
                for (int j = 0; j < c.size(); j++)
                    if (c[j] != x)
                        out_def.push(~c[j]);
                assert(out_def.size() == c.size()-1);
            }
        }
    }
    return (out_def.size() > 0);
}

/*
 - 1 2 3                 -3 -4
  1 -2 3  resolved with  -3 -5
  4  5 3                 -3 6 7
                         -3 -6 -7

3 -> ~4       == { ~3, ~4 }   (ger ~4, ~5 + ev mer som superset; negera detta och lägg till 3:an)
3 -> ~5       == { ~3, ~5 }

~4 & ~5 -> 3  == { 4, 5, 3 }
*/

int Solver::substitute(Lit x, Clause def, vec<Clause>& poss, vec<Clause>& negs, vec<Clause>& new_clauses = *(vec<Clause>*)NULL)
{
    vec<Lit>    tmp;
    int         counter = 0;


    // Positives:
    //**/if (hej) printf("    --from POS--\n");
    for (int i = 0; i < def.size(); i++){
        for (int j = 0; j < poss.size(); j++){
            if (poss[j].null()) continue;

            Clause c = poss[j];
            for (int k = 0; k < c.size(); k++){
                if (c[k] == ~def[i])
                    goto Skip;
            }
            tmp.clear();
            for (int k = 0; k < c.size(); k++){
                if (c[k] == x)
                    tmp.push(def[i]);
                else
                    tmp.push(c[k]);
            }
            //**/if (hej) printf("    "), dump(*this, tmp);
            if (&new_clauses != NULL){
                Clause tmp_c;
                tmp_c = addClause(tmp);
                if (!tmp_c.null())
                    new_clauses.push(tmp_c);
            }else{
                sortUnique(tmp);
                for (int i = 0; i < tmp.size()-1; i++)
                    if (tmp[i] == ~tmp[i+1])
                        goto Skip;
                counter++;
            }
          Skip:;
        }
    }
    //**/if (hej) printf("    --from NEG--\n");

    // Negatives:
    for (int j = 0; j < negs.size(); j++){
        if (negs[j].null()) continue;

        Clause c = negs[j];
        // If any literal from 'def' occurs in 'negs[j]', it is satisfied.
        for (int i = 0; i < def.size(); i++){
            if (has(c, def[i]))
                goto Skip2;
        }

        tmp.clear();
        for (int i = 0; i < c.size(); i++){
            if (c[i] == ~x){
                for (int k = 0; k < def.size(); k++)
                    tmp.push(~def[k]);
            }else
                tmp.push(c[i]);
        }
        //**/if (hej) printf("    "), dump(*this, tmp);
        if (&new_clauses != NULL){
            Clause tmp_c;
            tmp_c = addClause(tmp);
            if (!tmp_c.null())
                new_clauses.push(tmp_c);
        }else{
            sortUnique(tmp);
            for (int i = 0; i < tmp.size()-1; i++)
                if (tmp[i] == ~tmp[i+1])
                    goto Skip2;
            counter++;
        }
      Skip2:;
    }

    //**/if (counter != 0 && def.size() >= 2) exit(0);
    return counter;
}


void Solver::asymmetricBranching(Lit p)
{
    if (!opt_asym_branch) return;

    assert(decisionLevel() == 0);
    propagateToplevel(); if (!ok){ Report("(contradiction during asymmetric branching)\n"); return; }

    vec<Lit>    learnt;
    //vec<char>&  seen = seen_tmp;
    vec<Clause> cs;
    occur[index(p)].copyTo(cs);
    registerIteration(cs);

    for (int i = 0; i < cs.size(); i++){
        Clause  c = cs[i]; if (c.null()) continue;

#if 1
        Clause  confl;
        bool    analyze    = false;

        //**/printf("Inspecting: "); dump(*this, c);
        for (int j = 0; j < c.size(); j++){
            if (c[j] == p) continue;
            Lit q = ~c[j];
            //**/printf("Assuming: "L_LIT"\n", L_lit(q));
            if (!assume(q)){
                //**/printf("Assumption failed!\n");
                learnt.clear();
                if (level[var(q)] > 0)
                    learnt.push(~q);
                Clause rs = reason[var(q)];
                for (int k = 1; k < rs.size(); k++){
                    assert(value(rs[k]) == l_False);
                    seen_tmp[index(~rs[k])] = 1; }
                analyze = true;
                break;
            }

            confl = propagate();
            if (!confl.null()){
                //**/printf("Lead to conflict! "); dump(*this, confl);
                learnt.clear();
                for (int k = 0; k < confl.size(); k++){
                    assert(value(confl[k]) == l_False);
                    seen_tmp[index(~confl[k])] = 1; }
                analyze = true;
                break;
            }
        }

        if (analyze){
            // Analyze conflict:
            for (int j = trail.size()-1; j >= 0; j--){
                Lit q = trail[j];
                if (seen_tmp[index(q)]){
                    Clause rs = reason[var(q)];
                    if (rs.null()){
                        if (level[var(q)] > 0)
                            learnt.push(~q);
                    }else{
                        for (int k = 1; k < rs.size(); k++)
                            seen_tmp[index(~rs[k])] = 1;
                    }
                    seen_tmp[index(q)] = 0;
                }
            }
        }

        cancelUntil(0);

        /*Test 'seen[]'*/
        //for (int i = 0; i < seen.size(); i++) assert(seen[i] == 0);
        /*End*/

        if (analyze){
            // Update clause:
            //**/printf("%d ", c.size() - learnt.size()); fflush(stdout);
            //**/{static int counter = 0; static const char chr[] = { '|', '/', '-', '\\' }; printf("%c\r", chr[counter++ & 3]); fflush(stdout); }
            assert(propQ.size() == 0);
            /*Test Subset*/
          #ifndef NDEBUG
            for (int i = 0; i < learnt.size(); i++){
                for (int j = 0; j < c.size(); j++)
                    if (c[j] == learnt[i])
                        goto Found;
                printf("\n");
                printf("asymmetricBranching("L_LIT" @ %d)\n", L_lit(p), level[var(p)]);
                printf("learnt: "); dump(*this, learnt);
                printf("c     : "); dump(*this, c     );
                assert(false);  // no subset!
              Found:;
            }
          #endif
            /*End*/
            unlinkClause(c);    // (will touch all variables)
            addClause(learnt, false, c);
            propagateToplevel(); if (!ok){ Report("(contradiction during asymmetric branching after learning unit fact)\n"); return; }
        }

#else
        assert(propQ.size() == 0);
        bool conflict = false;
        for (int j = 0; j < c.size(); j++){
            if (c[j] == p) continue;
            if (!assume(~c[j]))      { conflict = true; break; }
            if (!propagate().null()) { conflict = true; break; }
        }
        assert(propQ.size() == 0);
        cancelUntil(0);

        if (conflict){
            /**/putchar('+'); fflush(stdout);
            //**/printf("----------------------------------------\n");
            //**/printf("asymmetricBranching("L_LIT" @ %d)\n", L_lit(p), level[var(p)]);
            //**/dump(*this, c);
            //**/for (int j = 0; j < c.size(); j++) if (c[j] == p) goto Found; assert(false); Found:;
            assert(!cs[i].null());
            strengthenClause(c, p);
            propagateToplevel(); if (!ok){ Report("(contradiction during asymmetric branching after learning unit fact)\n"); return; }
        }
#endif
    }

    unregisterIteration(cs);
    assert(propQ.size() == 0);
}


// Returns TRUE if variable was eliminated.
bool Solver::maybeEliminate(Var x)
{
    assert(propQ.size() == 0);
    assert(!var_elimed[x]);
    if (frozen[x]) return false;
    if (value(x) != l_Undef) return false;
    if (occur[index(Lit(x))].size() == 0 && occur[index(~Lit(x))].size() == 0) return false;

    vec<Clause>&   poss = occur[index( Lit(x))];
    vec<Clause>&   negs = occur[index(~Lit(x))];
    vec<Clause>    new_clauses;

    int before_clauses  = -1;
    int before_literals = -1;

    bool    elimed     = false;
    bool    tried_elim = false;

    #define MigrateToPsNs vec<Clause> ps; poss.moveTo(ps); vec<Clause> ns; negs.moveTo(ns); for (int i = 0; i < ps.size(); i++) unlinkClause(ps[i], x); for (int i = 0; i < ns.size(); i++) unlinkClause(ns[i], x);
    #define DeallocPsNs   for (int i = 0; i < ps.size(); i++) deallocClause(ps[i]); for (int i = 0; i < ns.size(); i++) deallocClause(ns[i]);

    // Find 'x <-> p':
    if (opt_unit_def){
        Lit p = findUnitDef(x, poss, negs);
        /**/if (p == lit_Undef) propagateToplevel(); if (!ok) return true;
        if (p != lit_Undef){
            //**/printf("DEF: x%d = "L_LIT"\n", x, L_lit(p));
            /*BEG
            printf("POS:\n"); for (int i = 0; i < poss.size(); i++) printf("  "), dump(poss[i]);
            printf("NEG:\n"); for (int i = 0; i < negs.size(); i++) printf("  "), dump(negs[i]);
            printf("DEF: x%d = "L_LIT"\n", x, L_lit(p));
            hej = true;
            END*/
            Clause_t    def; def.push(p);
            MigrateToPsNs
            substitute(Lit(x), def, ps, ns, new_clauses);
            /*BEG
            hej = false;
            printf("NEW:\n");
            for (int i = 0; i < new_clauses.size(); i++)
                printf("  "), dump(new_clauses[i]);
            END*/
            propagateToplevel(); if (!ok) return true;
            DeallocPsNs
            goto Eliminated;
        }
    }

    // ...
#if 0
    if (poss.size() < 10){ asymmetricBranching( Lit(x)); if (!ok) return true; }
    if (negs.size() < 10){ asymmetricBranching(~Lit(x)); if (!ok) return true; }
    if (value(x) != l_Undef) return false;
#endif

    // Heuristic:
    if (poss.size() >= 8 && negs.size() >= 8)      // <<== CUT OFF
//  if (poss.size() >= 7 && negs.size() >= 7)      // <<== CUT OFF
//  if (poss.size() >= 6 && negs.size() >= 6)      // <<== CUT OFF
        return false;

    // Count clauses/literals before elimination:
    before_clauses  = poss.size() + negs.size();
    before_literals = 0;
    for (int i = 0; i < poss.size(); i++) before_literals += poss[i].size();
    for (int i = 0; i < negs.size(); i++) before_literals += negs[i].size();

    if (poss.size() >= 3 && negs.size() >= 3 && before_literals > 300)  // <<== CUT OFF
        return false;

    // Check for definitions:
    if (opt_def_elim || opt_hyper1_res){
        //if (poss.size() > 1 || negs.size() > 1){
        if (poss.size() > 2 || negs.size() > 2){
            Clause_t def;
            int      result_size;
            if (findDef(Lit(x), poss, negs, def)){
                if (def.size() == 0){ enqueue(~Lit(x)); return true; }  // Hyper-1-resolution
                result_size = substitute(Lit(x), def, poss, negs);
                if (result_size <= poss.size() + negs.size()){  // <<= elimination threshold (maybe subst. should return literal count as well)
                    MigrateToPsNs
                    substitute(Lit(x), def, ps, ns, new_clauses);
                    propagateToplevel(); if (!ok) return true;
                    DeallocPsNs
                    goto Eliminated;
                }else
                    tried_elim = true,
                    def.clear();
            }
            if (!elimed && findDef(~Lit(x), negs, poss, def)){
                if (def.size() == 0){ enqueue(Lit(x)); return true; }  // Hyper-1-resolution
                result_size = substitute(~Lit(x), def, negs, poss);
                if (result_size <= poss.size() + negs.size()){  // <<= elimination threshold
                    MigrateToPsNs
                    substitute(~Lit(x), def, ns, ps, new_clauses);
                    propagateToplevel(); if (!ok) return true;
                    DeallocPsNs
                    goto Eliminated;
                }else
                    tried_elim = true;
            }
        }
    }

    if (!tried_elim){
        // Count clauses/literals after elimination:
        int after_clauses  = 0;
        int after_literals = 0;
        Clause_t  dummy;
        for (int i = 0; i < poss.size(); i++){
            for (int j = 0; j < negs.size(); j++){
                // Merge clauses. If 'y' and '~y' exist, clause will not be created.
                dummy.clear();
                bool ok = merge(poss[i], negs[j], Lit(x), ~Lit(x), seen_tmp, dummy.asVec());
                if (ok){
                    after_clauses++;
                    /**/if (after_clauses > before_clauses) goto Abort;
                    after_literals += dummy.size(); }
            }
        }
      Abort:;

        // Maybe eliminate:
        if ((!opt_niver && after_clauses  <= before_clauses)
        ||  ( opt_niver && after_literals <= before_literals)
        ){
            MigrateToPsNs
            for (int i = 0; i < ps.size(); i++){
                for (int j = 0; j < ns.size(); j++){
                    dummy.clear();
                    bool ok = merge(ps[i], ns[j], Lit(x), ~Lit(x), seen_tmp, dummy.asVec());
                    if (ok){
                        Clause c = addClause(dummy.asVec());
                        if (!c.null()){
                            new_clauses.push(c); }
                        propagateToplevel(); if (!ok) return true;
                    }
                }
            }
            DeallocPsNs
            goto Eliminated;
        }

        /*****TEST*****/
#if 1
        // Try to remove 'x' from clauses:
        bool    ran = false;
        if (poss.size() < 10){ ran = true; asymmetricBranching( Lit(x)); if (!ok) return true; }
        if (negs.size() < 10){ ran = true; asymmetricBranching(~Lit(x)); if (!ok) return true; }
        if (value(x) != l_Undef) return false;
        if (!ran) return false;

        {
            // Count clauses/literals after elimination:
            int after_clauses  = 0;
            int after_literals = 0;
            Clause_t  dummy;
            for (int i = 0; i < poss.size(); i++){
                for (int j = 0; j < negs.size(); j++){
                    // Merge clauses. If 'y' and '~y' exist, clause will not be created.
                    dummy.clear();
                    bool ok = merge(poss[i], negs[j], Lit(x), ~Lit(x), seen_tmp, dummy.asVec());
                    if (ok){
                        after_clauses++;
                        after_literals += dummy.size(); }
                }
            }

            // Maybe eliminate:
            if ((!opt_niver && after_clauses  <= before_clauses)
            ||  ( opt_niver && after_literals <= before_literals)
            ){
                MigrateToPsNs
                for (int i = 0; i < ps.size(); i++){
                    for (int j = 0; j < ns.size(); j++){
                        dummy.clear();
                        bool ok = merge(ps[i], ns[j], Lit(x), ~Lit(x), seen_tmp, dummy.asVec());
                        if (ok){
                            Clause c = addClause(dummy.asVec());
                            if (!c.null()){
                                new_clauses.push(c); }
                            propagateToplevel(); if (!ok) return true;
                        }
                    }
                }
                DeallocPsNs
                goto Eliminated;
            }
        }
#endif
        /*****END TEST*****/
    }

    return false;

  Eliminated:
    assert(occur[index(Lit(x))].size() + occur[index(~Lit(x))].size() == 0);
    var_elimed[x] = 1;
    assigns   [x] = toInt(l_Error);
    return true;
}


// Inefficient test implementation! (pre-condition: satisfied clauses have been removed)
//
void Solver::clauseReduction(void)
{
    /**/reportf("clauseReduction() -- BEGIN\n");
    assert(decisionLevel() == 0);
    propagateToplevel(); if (!ok){ Report("(contradiction during clause reduction)\n"); return; }

    vec<Lit>    learnt;
    vec<char>&  seen = seen_tmp;

    for (int i = 0; i < constrs.size(); i++){
        Clause  c = constrs[i]; if (c.null()) continue;
        Clause  confl;
        bool    analyze = false;

        //**/dump(*this, c);
        unwatch(c, ~c[0]);
        unwatch(c, ~c[1]);
        for (int j = 0; j < c.size(); j++){
            //**/printf(L_LIT" = %c\n", L_lit(c[j]), name(value(c[j])));

            if (!assume(~c[j])){
                learnt.clear();
                if (level[var(c[j])] > 0)
                    learnt.push(c[j]);
                Clause rs = reason[var(c[j])];
                for (int k = 1; k < rs.size(); k++){
                    assert(value(rs[k]) == l_False);
                    seen[index(~rs[k])] = 1; }
                analyze = true;
                break;
            }

            confl = propagate();
            if (!confl.null()){
                learnt.clear();
                for (int k = 0; k < confl.size(); k++){
                    assert(value(confl[k]) == l_False);
                    seen[index(~confl[k])] = 1; }
                analyze = true;
                break;
            }
        }

        if (analyze){
            // Analyze conflict:
            for (int j = trail.size()-1; j >= 0; j--){
                Lit p = trail[j];
                if (seen[index(p)]){
                    Clause rs = reason[var(p)];
                    if (rs.null()){
                        if (level[var(p)] > 0)
                            learnt.push(~p);
                    }else{
                        for (int k = 1; k < rs.size(); k++)
                            seen[index(~rs[k])] = 1;
                    }
                    seen[index(p)] = 0;
                }
            }

            // (kolla att seen[] är nollad korrekt här)

            /**/if (learnt.size() < c.size()){
                putchar('*'); fflush(stdout);
                //**/printf("Original: "); dump(c);
                //**/printf("Conflict:");
                //**/for (int j = 0; j < learnt.size(); j++) printf(" "L_LIT, L_lit(learnt[j]));
                //**/printf("\n");
                //**/if (learnt.size() == 0) exit(0);
            /**/}
        }

        cancelUntil(0);
        watch(c, ~c[0]);
        watch(c, ~c[1]);

        if (analyze && learnt.size() < c.size()){
            // Update clause:
            //**/printf("{"); for (int i = 0; i < c.size(); i++) printf(" %d", occur[index(c[i])].size()); printf(" }\n");

            /**/assert(propQ.size() == 0);
            /*Test Subset*/
            for (int i = 0; i < learnt.size(); i++){
                for (int j = 0; j < c.size(); j++)
                    if (c[j] == learnt[i])
                        goto Found;
                assert(false);  // no subset!
              Found:;
            }
            /*End*/
            unlinkClause(c);    // (will touch all variables)
            addClause(learnt, false, c);
            propagateToplevel(); if (!ok){ Report("(contradiction during clause reduction after learning unit fact)\n"); return; }
            /**/assert(propQ.size() == 0);
        }
        //**/else{ printf("                    {"); for (int i = 0; i < c.size(); i++) printf(" %d", occur[index(c[i])].size()); printf(" }\n"); }
    }
    /**/reportf("clauseReduction() -- END\n");
}
