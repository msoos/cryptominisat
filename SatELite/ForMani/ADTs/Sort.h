/**************************************************************************************************

Sort.h -- (C) Niklas Een, 2003

Template based sorting routines: sort, sortUnique (remove duplicates). Can be applied either on
'vec's or on standard C arrays (pointers).  

**************************************************************************************************/


#ifndef Sort_h
#define Sort_h

//#include <cstdlib>


//=================================================================================================


template<class T>
struct LessThan_default {
    bool operator () (T x, T y) { return x < y; }
};


//=================================================================================================


template <class T, class LessThan>
void selectionSort(T* array, int size, LessThan lt)
{
    int     i, j, best_i;
    T       tmp;

    for (i = 0; i < size-1; i++){
        best_i = i;
        for (j = i+1; j < size; j++){
            if (lt(array[j], array[best_i]))
                best_i = j;
        }
        tmp = array[i]; array[i] = array[best_i]; array[best_i] = tmp;
    }
}
template <class T> static inline void selectionSort(T* array, int size) {
    selectionSort(array, size, LessThan_default<T>()); }


template <class T, class LessThan>
void sort(T* array, int size, LessThan lt, double& seed)
{
    if (size <= 15)
        selectionSort(array, size, lt);

    else{
        T           pivot = array[irand(seed, size)];
        T           tmp;
        int         i = -1;
        int         j = size;

        for(;;){
            do i++; while(lt(array[i], pivot));
            do j--; while(lt(pivot, array[j]));

            if (i >= j) break;

            tmp = array[i]; array[i] = array[j]; array[j] = tmp;
        }

        sort(array    , i     , lt, seed);
        sort(&array[i], size-i, lt, seed);
    }
}
template <class T, class LessThan> static inline void sort(T* array, int size, LessThan lt) {
    double  seed = 91648253; sort(array, size, lt, seed); }
template <class T> static inline void sort(T* array, int size) {
    sort(array, size, LessThan_default<T>()); }


template <class T, class LessThan>
void sortUnique(T* array, int& size, LessThan lt)
{
    int         i, j;
    T           last;

    if (size == 0) return;

    sort(array, size, lt);

    i    = 1;
    last = array[0];
    for (j = 1; j < size; j++){
        if (lt(last, array[j])){
            last = array[i] = array[j];
            i++; }
    }

    size = i;
}
template <class T> static inline void sortUnique(T* array, int& size) {
    sortUnique(array, size, LessThan_default<T>()); }


//=================================================================================================
// For 'vec's:


template <class T, class LessThan> void sort(vec<T>& v, LessThan lt) {
    sort((T*)v, v.size(), lt); }
template <class T> void sort(vec<T>& v) {
    sort(v, LessThan_default<T>()); }


template <class T, class LessThan> void sortUnique(vec<T>& v, LessThan lt) {
    int     size = v.size();
    T*      data = v.release();
    sortUnique(data, size, lt);
    v.~vec<T>();
    new (&v) vec<T>(data, size); }
template <class T> void sortUnique(vec<T>& v) {
    sortUnique(v, LessThan_default<T>()); }


//=================================================================================================
#endif
