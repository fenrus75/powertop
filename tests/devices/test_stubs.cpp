/*
 * Stubs for symbols that device.cpp and alsa.cpp pull in transitively
 * but that are not needed (or too heavy to link) in unit tests.
 *
 * Now that device.cpp only contains the class implementation, these are
 * the three remaining symbols from measurement/ and devlist/ that tests
 * need to stub out.
 */
#include <string>
#include "devices/device.h"
#include "measurement/measurement.h"
#include "devlist.h"

/* From measurement/measurement.cpp */
double global_power() { return 0.0; }
void save_all_results([[maybe_unused]] const std::string &filename) {}

/* From devlist.cpp */
void register_devpower([[maybe_unused]] const std::string &devstring,
                       [[maybe_unused]] double power,
                       [[maybe_unused]] class device *dev) {}
