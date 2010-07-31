#ifndef INCLUDE_GUARD_LIB_H
#define INCLUDE_GUARD_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

#define _(STRING)    gettext(STRING)


extern void read_cstate_data(int cpu, uint64_t * usage, uint64_t * duration, char **cnames);


#ifdef __cplusplus
}
#endif

#endif