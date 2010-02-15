/*
Requires copy constructor and assignment operator to be defined for the key and
datum types, (plus default constructor unless null value is passed to constructor)
*/


#ifndef Map_h
#define Map_h

#include "VecAlloc.h"
#include "Hash_standard.h"


//=================================================================================================
// Map implementation:


template <class K, class D, class Par = Hash_params<K> >
class Map
{

    struct  Cell {
        K       key;
        D       datum;
        Cell*   next;
    };

    VecAlloc<Cell>  alloc;
    Cell**          table;
    int             capacity;
    int             nelems;
    const D         D_null;

    //---------------------------------------------------------------------------------------------

    int index(const K& key) const {
        return Par::hash(key) % capacity; }     // (this is done in 'rehash()' as well)

    int getCapacity(int min_capacity) {
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
                p->datum.~D();
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

    D& newEntry(int i, const K& key, const D& value) {
        if (nelems > capacity / 2){
            rehash(capacity * 2);
            i = index(key); }
        Cell* p = alloc.alloc();
        new (&p->key)   K(key);
        new (&p->datum) D(value);
        p->next  = table[i];
        table[i] = p;
        nelems++;
        return p->datum; }

    //---------------------------------------------------------------------------------------------

public:
    // Types:
    typedef K Key;
    typedef D Datum;

    // Constructors:
    Map(void)                        : D_null()     { init(1);        }
    Map(const D& null)               : D_null(null) { init(1);        }
    Map(const D& null, int capacity) : D_null(null) { init(capacity); }
   ~Map(void)                                       { dispose();      }

    // Size operations:
    int      size  (void) const { return nelems; }
    void     clear (void)       { dispose(); init(1); }

    // Don't allow copying:
    Map<K,D,Par>& operator = (Map<K,D,Par>& other) { TEMPLATE_FAIL; }
                  Map        (Map<K,D,Par>& other) { TEMPLATE_FAIL; }

    //---------------------------------------------------------------------------------------------
    // Export:


    void domain(vec<K>& result) const {
        for (int i = 0; i < capacity; i++)
            for (Cell* p = table[i]; p != NULL; p = p->next)
                result.push(p->key); }
    void range (vec<D>& result) const {
        for (int i = 0; i < capacity; i++)
            for (Cell* p = table[i]; p != NULL; p = p->next)
                result.push(p->datum); }
    void pairs (vec<Pair<K,D> >& result) const {
        for (int i = 0; i < capacity; i++)
            for (Cell* p = table[i]; p != NULL; p = p->next)
                result.push(Pair_new(p->key, p->datum)); }


    //---------------------------------------------------------------------------------------------
    // Main:


    // Searches for 'key' in the hash table. Returns the value of 'found' or 'notfound'.
    // 'found' may use 'Cell* p' refering to the cell that was found; 'notfound' may use
    // 'unsigned i', the index position in the table.
    #define SEARCH(found, notfound) \
        unsigned    i = index(key); \
        for (Cell* p = table[i]; p != NULL; p = p->next){ \
            if (Par::equal(p->key, key)) \
                return found; \
        } \
        return notfound;


    // Returns a reference to existing or newly created element.
    D& ref(const K& key) {
        SEARCH(  p->datum
              ,  newEntry(i, key, D_null)
              );}

    // Sets 'key' to 'value' and returns 'value'.
    const D& set(const K& key, const D& value) {
        SEARCH(  p->datum = value
              ,  newEntry(i, key, value)
              );}

    // Sets 'key' to 'value' IF not already set. Returns the value at 'key' after operation (new or old).
    const D& weakSet(const K& key, const D& value) {
        SEARCH(  p->datum
              ,  newEntry(i, key, value)
              );}

    // Same as 'set()' but 'key' must not be set already.
    const D& add(const K& key, const D& value) {
        SEARCH(  (PANIC("Tried to add an already existing element to the hashtable."), D_null)
              ,  newEntry(i, key, value)
              );}

    // Same as 'has' but returns through reference a pointer to the element (if it exists).
    bool peek(const K& key, D*& result) const {
        SEARCH(  (result = &p->datum, true)
              ,  false
              );}

    // Same as 'has' but returns through reference the element (if it exists).
    bool peek(const K& key, D& result) const {
        SEARCH(  (result = p->datum, true)
              ,  false
              );}

    // Returns datum at 'key' (or the null value if none)
    const D& at(const K& key) {
        SEARCH(  p->datum
              ,  D_null
              );}

    // Same as 'at' but 'key' must contain a value.
    const D& find(const K& key) {
        SEARCH(  p->datum
              ,  (PANIC("Tried to find non-existing element in hashtable."), D_null)
              );}

    // Have 'key' been set?
    bool has(const K& key) {
        SEARCH(  true
              ,  false
              );}

    // Get the current equal (but not same) key used for 'key'.
    const K& findKey(const K& key) {
        SEARCH(  p->key
              ,  (PANIC("Tried to find non-existing element in hashtable."), *(K*)NULL)
              );}

    // Exclude 'key' if exists.
    bool exclude(const K& key) {
        unsigned    i = index(key);
        for (Cell** pp = &table[i]; *pp != NULL; pp = &(*pp)->next){
            if (Par::equal((*pp)->key, key)){
                Cell*   p = *pp;
                *pp = (*pp)->next;
                nelems--;
                p->key  .~K();
                p->datum.~D();
                alloc.free(p);
                return true; }
        }
        return false;
    }

    // Same as 'exclude' but 'key' must exist.
    void remove(const K& key) {
        if (!exclude(key)) PANIC("Tried to remove non-existing element from hashtable."); }


    #undef SEARCH

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    String show(void) const {
        String header = showf("capacity = %d\n", capacity)
                      + showf("nelems   = %d\n", nelems)
                      + sref ("D_null   = ") + ::show(D_null) + sref("\n\n");
        String  body;
        int     width = strlen(sFree(strnum(capacity)));
        for (int i = 0; i < capacity; i++){
            body = body + showf("%*d:", width, i);
            for (Cell* p = table[i]; p != NULL; p = p->next)
                body = body + sref(" (") + ::show(p->key) + sref(", ") + ::show(p->datum) + sref(")");
            body = body + sref("\n");
        }
        return header + body;
    }

};


//=================================================================================================

#endif
