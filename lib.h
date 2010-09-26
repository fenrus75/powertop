#ifndef INCLUDE_GUARD_LIB_H
#define INCLUDE_GUARD_LIB_H

#include <libintl.h>
#include <stdint.h>


#define _(STRING)    gettext(STRING)


extern int get_max_cpu(void);
extern void set_max_cpu(int cpu);

extern double percentage(double F);
extern char *hz_to_human(unsigned long hz, char *buffer, int digits = 2);


extern const char *kernel_function(uint64_t address);

class stringless  
{  
public:
	bool operator()(const char * const & lhs, const char * const & rhs) const ;
};  




#include <string>
using namespace std;

extern void write_sysfs(string filename, string value);
extern int read_sysfs(string filename);

extern void format_watts(double W, char *buffer, unsigned int len);


#endif