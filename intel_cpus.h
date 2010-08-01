#include <stdint.h>

#define MSR_TSC				0x10
#define MSR_NEHALEM_PLATFORM_INFO	0xCE
#define MSR_NEHALEM_TURBO_RATIO_LIMIT	0x1AD
#define MSR_APERF			0xE8
#define MSR_MPERF			0xE7
#define MSR_PKG_C3_RESIDENCY		0x3F8
#define MSR_PKG_C6_RESIDENCY		0x3F9
#define MSR_CORE_C3_RESIDENCY		0x3FC
#define MSR_CORE_C6_RESIDENCY		0x3FD


class nhm_package: public cpu_package 
{
public:
	virtual void 	measurement_start(void);
	virtual void 	measurement_end(void);

	void 	consolidate_children(void);
};

class nhm_core: public cpu_core 
{
private:
	uint64_t	aperf_before, mperf_before;
	uint64_t	aperf_after, mperf_after;
	uint64_t	c3_before, c3_after;
	uint64_t	c6_before, c6_after;
public:
	virtual void 	measurement_start(void);
	virtual void 	measurement_end(void);

	void 	consolidate_children(void);
};

class atom_package: public cpu_package 
{
public:
	virtual void 	measurement_start(void);
	virtual void 	measurement_end(void);

	void 	consolidate_children(void);
};

class atom_core: public cpu_core
{
public:
	virtual void 	measurement_start(void);
	virtual void 	measurement_end(void);

	void 	consolidate_children(void);
};

