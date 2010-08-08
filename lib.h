#ifndef INCLUDE_GUARD_LIB_H
#define INCLUDE_GUARD_LIB_H

#include <libintl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _(STRING)    gettext(STRING)


extern double percentage(double F);
extern char *hz_to_human(unsigned long hz, char *buffer);



#ifdef __cplusplus
}
#endif

#endif