/* Host-only observation. Include the real declarations before replacing calls. */
#include <stdatomic.h>
#include <stdint.h>
#include "dsp.h"
extern void nature_test_publication(unsigned after);
extern void nature_test_load(void);
extern void nature_test_coefficient(void);
static inline void nature_test_store_u32(_Atomic uint32_t *p,uint32_t v,memory_order o) {
    nature_test_publication(0); atomic_store_explicit(p,v,o); nature_test_publication(1);
}
static inline void nature_test_store_int(_Atomic int *p,int v,memory_order o) {
    nature_test_publication(0); atomic_store_explicit(p,v,o); nature_test_publication(1);
}
static inline uint32_t nature_test_load_u32(_Atomic uint32_t *p,memory_order o) {
    nature_test_load(); return atomic_load_explicit(p,o);
}
static inline int nature_test_load_int(_Atomic int *p,memory_order o) {
    nature_test_load(); return atomic_load_explicit(p,o);
}
static inline void nature_test_svf_set(dsp_svf_t *p,float hz,float q) {
    nature_test_coefficient(); dsp_svf_set(p,hz,q);
}
#undef atomic_store_explicit
#undef atomic_load_explicit
#define atomic_store_explicit(p,v,o) _Generic((p), \
    _Atomic uint32_t *:nature_test_store_u32,_Atomic int *:nature_test_store_int)(p,v,o)
#define atomic_load_explicit(p,o) _Generic((p), \
    _Atomic uint32_t *:nature_test_load_u32,_Atomic int *:nature_test_load_int)(p,o)
#define dsp_svf_set nature_test_svf_set
