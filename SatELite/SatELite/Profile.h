#ifndef Profile_h
#define Profile_h

//=================================================================================================


extern __inline__ uint64 rdtsc(void) {
    uint64 x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x; }

extern uint64  t_total__;
extern uint64  t_mark__;
extern int     t_reclev__;

macro void   tset(void) { if (t_reclev__ == 0) t_mark__ = rdtsc(); t_reclev__++; }
macro void   tclr(void) { t_reclev__--; if (t_reclev__ == 0) t_total__ += rdtsc() - t_mark__; }
macro double tsum(void) { return (double)t_total__ / 2050000000.0; }       // (approximation of the CPU frequency goes here...)

struct TimeIt_t {
    TimeIt_t(void) { tset(); }
   ~TimeIt_t(void) { tclr(); }
};

#define TimeIt TimeIt_t TimeIt_dummy;


//=================================================================================================
#endif
