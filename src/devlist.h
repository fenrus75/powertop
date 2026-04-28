#pragma once

#include <string>

struct devuser {
	unsigned int pid = 0;
	std::string comm;
	std::string device;
};

class device;

struct devpower {
	std::string device;
	double power = 0.0;
	class device *dev = nullptr;
};

extern void clean_open_devices();
extern void collect_open_devices(void);

extern void clear_devpower(void);
extern void register_devpower(const std::string &devstring, double power, class device *dev);
extern void run_devpower_list(void);

extern void report_show_open_devices(void);

