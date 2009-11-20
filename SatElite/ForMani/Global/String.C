/**************************************************************************************************

STRING -- Department of Computer Science, Chalmers University of Technology 2002.

File..: String.C
Author: Niklas Een, een@cs.chalmers.se
Descr.: Minimal ADT for strings.

***************************************************************************************************


General:
========================================

Minimal string ADT that supports concatenation in constant time (for functional programing style).
NOTE! A 'String' is a DAG where the leaves are pieces of text (char*). Freeing the top-node will
dispose all strings marked with as "owned". Therefore you cannot share substrings between
different 'String's (if you plan to dispose them).

Constructors:

  Constructors:
    String scopy(char* text)            -- Create a copy of 'text'.
    String sref (char* text)            -- Reference 'text' (must not be freed during liveness of the string).
    String sown (char* text)            -- Take ownership of 'text'. Will be freed when string is deleted.
    String scat (String s1, String s2)  -- Concatenate to strings.
    String operator + (...)             -- Syntactic sugar for 'scat'.

  Destructor:
    void    sfree  (String s);          -- Frees all internal 'char*' that are owned + the string ADT itself.

  Methods:
    int     slen   (String s)           -- Returns length of string.
    char*   stext  (String s)           -- Returns a newly allocated 'char*' with string.
    char*   stextf (String s)           -- 'stext' + 'sfree'
    char*   stemp  (String s)           -- Creates self-freeing 'char*' (when goes out of scope) with string.
    char*   stempf (String s)           -- 'stemp' + 'sfree'

History:
========================================

2001-06-20: Created.


=================================================================================================*/

#include "String.h"

//=================================================================================================


// (initialize the empty string)
String_t String_empty;
static struct Initializer { Initializer(void) {
    String_empty.type.leaf     = true;
    String_empty.type.disposed = false;
    String_empty.leaf.text     = "";
    String_empty.leaf.owns     = false;
} } String_init;


int slen(String s) {
    return (s->type.leaf) ? strlen(s->leaf.text)
                          : slen(s->node.left) + slen(s->node.right); }


static
void stext_(String s, char** result)
{
    if (s->type.leaf){
        char*   p = s->leaf.text;
        while (*p != '\0')
            *(*result)++ = *p++;
    }else{
        stext_(s->node.left , result);
        stext_(s->node.right, result);
    }
}

char* stext(String s)
{
    char*   result = xmalloc<char>(slen(s) + 1);
    char*   tmp = result;
    stext_(s, &tmp);
    *tmp = '\0';
    return result;
}


static
void sfree1(String s)
{
    if (s->type.leaf){
        if (s->leaf.owns)
            xfree(s->leaf.text);
    }else{
        if (s->node.left ->type.disposed) s->node.left  = NULL;
        else                              sfree1(s->node.left);

        if (s->node.right->type.disposed) s->node.right = NULL;
        else                              sfree1(s->node.right);
    }
    s->type.disposed = true;
}

static
void sfree2(String s)
{
    if (!s->type.leaf){
        if (s->node.left  != (String_t*)NULL) sfree2(s->node.left );
        if (s->node.right != (String_t*)NULL) sfree2(s->node.right);
    }
    String_delete(s);
}

void sfree(String s)
{
    sfree1(s);
    sfree2(s);
}
