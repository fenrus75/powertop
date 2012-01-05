#ifndef __INCLUDE_GUARD_DEVLIST_H__
#define __INCLUDE_GUARD_DEVLIST_H__

struct devuser {
	unsigned int pid;
	char comm[32];
	char device[252];
};

class device;

struct devpower {
	char device[252];
	double power;
	class device *dev;
};

extern void collect_open_devices(void);

extern void clear_devpower(void);
extern void register_devpower(const char *devstring, double power, class device *dev);
extern void run_devpower_list(void);

extern void report_show_open_devices(void);

#endif
