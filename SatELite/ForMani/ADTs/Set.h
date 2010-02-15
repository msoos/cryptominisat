/*
Requires copy constructor and assignment operator to be defined for the type,
(plus default constructor unless null value is passed to constructor)
*/


#ifndef Set_h
#define Set_h

#include "VecAlloc.h"
#include "Hash_standard.h"

//=================================================================================================
// Set implementation:


template <class K, class Par = Hash_params<K> > class SetIter;
template <class K, class Par = Hash_params<K> >
class Set
{
    struct  Cell {
        K       key;
        Cell*   next;
    };

    VecAlloc<Cell>  alloc;
    Cell**          table;
    int             capacity;
    int             nelems;

#ifdef SET_ITERATORS
    friend class SetIter<K, Par>;
    vec<SetIter<K, Par>*>   iters;
    inline void adjustIterators(Cell* p);
public:
    int nIterators(void) { return iters.size(); }
private:
#endif

    //---------------------------------------------------------------------------------------------

    int index(const K& key) const {
        return Par::hash(key) % capacity; }     // (this is done in 'rehash()' as well)

    int getCapacity(int min_capacity) {
      #ifdef SET_DEBUG
        return 2;
      #endif
        int     i;
        for (i = 0; prime_twins[i] < min_capacity; i++);
        return prime_twins[i]; }

    void init(int min_capacity) {
        capacity = getCapacity(min_capacity);
        nelems   = 0;
        table    = xmalloc<Cell*>(capacity);
        for (int i = 0; i < capacity; i++) table[i] = NULL; }

    void dispose(void) {
        for (int i = 0; i < capacity; i++){
            for (Cell* p = table[i]; p != NULL;){
                Cell*    next = p->next;
                p->key  .~K();
                alloc.free(p);
                p = next; } }
        xfree(table);
    }

    void rehash(int min_capacity) {
        int     new_capacity = getCapacity(min_capacity);
        Cell**  new_table    = xmalloc<Cell*>(new_capacity);
        for (int i = 0; i < new_capacity; i++) new_table[i] = NULL;

        for (int i = 0; i < capacity; i++){
            for (Cell* p = table[i]; p != NULL;){
                Cell*    next = p->next;
                unsigned j    = Par::hash(p->key) % new_capacity;
                p->next = new_table[j];
                new_table[j] = p;
                p = next;
            }
        }
        xfree(table);
        table    = new_table;
        capacity = new_capacity;
    }

    void newEntry(int i, const K& key) {
      #ifndef SET_DEBUG
        if (nelems > capacity / 2){
            rehash(capacity * 2);
            i = index(key); }
      #endif
        Cell* p = alloc.alloc();
        new (&p->key) K(key);
        p->next  = table[i];
        table[i] = p;
        nelems++; }

    //---------------------------------------------------------------------------------------------

public:
    // Types:
    typedef K Key;

    // Constructors:
    Set(void)         { init(1);        }
    Set(int capacity) { init(capacity); }
   ~Set(void)         { dispose();      }

    // Size operations:
    int      size  (void) const { return nelems; }
    void     clear (void)       { dispose(); init(1); }

    // Don't allow copying:
    Set<K,Par>& operator = (Set<K,Par>& other) { TEMPLATE_FAIL; }
                Set        (Set<K,Par>& other) { TEMPLATE_FAIL; }

    //---------------------------------------------------------------------------------------------
    // Export:

    // Appends all elements to 'result'.
    void toVec(vec<K>& result) const {
        for (int i = 0; i < capacity; i++)
            for (Cell* p = table[i]; p != NULL; p = p->next)
                result.push(p->key); }

    //---------------------------------------------------------------------------------------------
    // Main:


    // Returns TRUE if element already exists.
    bool add(const K& key) {
        unsigned    i = index(key);
        for (Cell* p = table[i]; p != NULL; p = p->next)
            if (Par::equal(p->key, key))
                return true;
        newEntry(i, key);
        return false; }


    // Returns TRUE if element exists.
    bool has(const K& key) {
        unsigned    i = index(key);
        for (Cell* p = table[i]; p != NULL; p = p->next)
            if (Par::equal(p->key, key))
                return true;
        return false; }


    // Returns TRUE if element existed and was excluded.
    bool exclude(const K& key) {
        unsigned    i = index(key);
        for (Cell** pp = &table[i]; *pp != NULL; pp = &(*pp)->next){
            if (Par::equal((*pp)->key, key)){
                Cell*   p = *pp;
              #ifdef SET_ITERATORS
                adjustIterators(p);
              #endif
                *pp = (*pp)->next;
                nelems--;
                p->key  .~K();
                alloc.free(p);
                return true; }
        }
        return false;
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    String show(void) const {
        String header = showf("capacity = %d\n"  , capacity)
                      + showf("nelems   = %d\n\n", nelems);
        String  body;
        int     width = strlen(sFree(strnum(capacity)));
        for (int i = 0; i < capacity; i++){
            body = body + showf("%*d:", width, i);
            for (Cell* p = table[i]; p != NULL; p = p->next)
                body = body + sref(" ") + ::show(p->key);
            body = body + sref("\n");
        }
        return header + body;
    }
};


#ifdef SET_ITERATORS
template <class Key, class Par>
class SetIter {
    friend class Set<Key,Par>;

    Set<Key,Par>&                S;
    int                          index;
    typename Set<Key,Par>::Cell* cell;
    bool                         invalid_;

    void advanceIndex(void) {
        for(;;){
            index++;
            if (index >= S.capacity){
                cell  = NULL;
                break;
            }else if (S.table[index] != NULL){
                cell = S.table[index];
                break; } } }

    void advance(void) {
        if (cell != NULL){
            cell = cell->next;
            if (cell == NULL)
                advanceIndex(); } }

public:
    SetIter(Set<Key,Par>& set) : S(set), index(-1), cell(NULL), invalid_(false) { S.iters.push(this); advanceIndex(); }
   ~SetIter(void) { for (int i = 0; i < S.iters.size(); i++) if (S.iters[i] == this){ S.iters[i] = S.iters.last(); S.iters.pop(); break; } }
    void operator ++ (void) { if (invalid_) invalid_ = false; else advance(); }
    void operator ++ (int)  { if (invalid_) invalid_ = false; else advance(); }
    Key& operator *  (void) { assert(!invalid_); return cell->key; }
    Key* operator -> (void) { assert(!invalid_); return &cell->key; }
    bool done        (void) { return cell == NULL; }
    bool invalid     (void) { return invalid_; }
};


template <class Key, class Par>
void Set<Key,Par>::adjustIterators(Cell* p) {
    for (int i = 0; i < iters.size(); i++)
        if (iters[i]->cell == p)
            iters[i]->invalid_ = true,
            iters[i]->advance(); }
#endif


//=================================================================================================

#endif
