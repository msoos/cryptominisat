/**************************************************************************************************

STRING -- Department of Computer Science, Chalmers University of Technology 2002.

File..: String.h
Author: Niklas Een, een@cs.chalmers.se
Descr.: Minimal ADT for strings.

**************************************************************************************************/

#ifndef String_h
#define String_h

//=================================================================================================


struct String_t;
extern String_t String_empty;
struct String {
    String_t*   ptr;
    String(String_t* p) { ptr = p; }
    String(void)        { ptr = &String_empty; }

    String_t* operator -> (void)        const { return ptr;          }
    String_t* operator =  (String_t* p)       { return ptr = p;      }
    bool      operator == (String s)    const { return ptr == s.ptr; }
    bool      operator != (String s)    const { return ptr != s.ptr; }
    bool      operator == (String_t* p) const { return ptr == p;     }
    bool      operator != (String_t* p) const { return ptr != p;     }
};



struct String_t {
    struct {
        unsigned leaf     : 1;
        unsigned disposed : 1;
    } type;
    union {
        struct {
            char*       text;
            bool        owns;
        } leaf;
        struct {
            String_t*   left;
            String_t*   right;
        } node;
    };
};


static
String String_leaf(char* text, bool owns) {
    String  tmp(new String_t);
    tmp->type.leaf     = true;
    tmp->type.disposed = false;
    tmp->leaf.text     = text;
    tmp->leaf.owns     = owns;
    return tmp; }

static
String String_node(String left, String right) {
    String  tmp(new String_t);
    tmp->type.leaf     = false;
    tmp->type.disposed = false;
    tmp->node.left     = left .ptr;
    tmp->node.right    = right.ptr;
    return tmp; }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


String scopy(const char* text);
String sref(const char* text);
String sown(char* text);
String scat(String s1, String s2);
String operator + (String s1, String s2);

#include "Global.h"

inline String scopy(const char* text) {
    return String_leaf(xstrdup(text), true); }

inline String sref(const char* text) {
    return String_leaf(const_cast<char*>(text), false); }

inline String sown(char* text) {
    return String_leaf(text, true); }

inline String scat(String s1, String s2) {
    return String_node(s1, s2); }

inline String operator + (String s1, String s2) {
    return scat(s1, s2); }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


static void String_delete(String s) ___unused;
static
void String_delete(String s) {
    if (s.ptr != &String_empty)
        delete s.ptr; }

int     slen (String s);
char*   stext(String s);
void    sfree(String s);

inline char* stextf(String s)
    { char* tmp = stext(s); sfree(s); return tmp; }

#define stemp(s)  ((char*)Free<char>(stext(s)))
#define stempf(s) ((char*)Free<char>(stextf(s)))


//=================================================================================================

#endif
