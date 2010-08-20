#ifndef INCLUDE_GUARD_LIB_H
#define INCLUDE_GUARD_LIB_H

#include <libintl.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _(STRING)    gettext(STRING)

#ifdef __cplusplus
}
#endif

extern int get_max_cpu(void);
extern void set_max_cpu(int cpu);

extern double percentage(double F);
extern char *hz_to_human(unsigned long hz, char *buffer, int digits = 2);


extern const char *kernel_function(uint64_t address);



#endif