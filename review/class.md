# PowerTOP class hierarchy

## class device

Base class for all hardware components that PowerTOP monitors for power consumption and utilization.

### Key public methods
* `start_measurement()`: Initializes measurement for the device.
* `end_measurement()`: Finalizes measurement for the device.
* `utilization()`: Returns the utilization percentage of the device.
* `device_name()`: Returns a string representing the device name.
* `device_name_s()`: Returns a std::string representing the device name.
* `human_name()`: Returns a std::string representing the human readable name.
* `power_usage(result_bundle, parameter_bundle)`: Calculates power usage based on measurement results and parameters.

### Key public variables
* `bool hide`: If true, the device is hidden from the UI.
* `std::string guilty`: Stores information about processes responsible for this device's activity.
* `std::string real_path`: The sysfs path to the device.

### Derived class cpudevice
Specialized for CPU devices; serves as a base for RAPL (Running Average Power Limit) devices.

### Derived class i915gpu
Specialized for Intel integrated graphics devices.

### Derived class usbdevice
Handles USB device power management and accounting.

### Derived class ahci, alsa, alsa_pcm, backlight, devfreq, network, rfkill, runtime_pmdevice, thinkpad_fan, thinkpad_light
These classes implement device-specific logic for storage, audio (alsa codecs, alsa_pcm streams listed below their codec), displays, network interfaces, and other hardware components.


## class power_consumer

Represents any entity (process, interrupt, etc.) that consumes power.

### Key public methods
* `Witts()`: Returns the power consumption in Watts.
* `usage()`: Returns a numerical value for usage (e.g., CPU time).
* `events()`: Returns the rate of events (wakeups, ops) per second.
* `description()`: Returns a std::string description of the consumer.

### Key public variables
* `uint64_t accumulated_runtime`: Total time the consumer was active.
* `double power_charge`: Power attributed to this consumer from devices it opened.
* `int wake_ups`: Number of CPU wakeups caused by this consumer.

### Derived class process
Represents a Linux process.

### Derived class interrupt
Represents a hardware or software interrupt.

### Derived class timer
Represents a kernel timer.

### Derived class work
Represents a kernel workqueue item.

### Derived class device_consumer
Connects power consumption to specific hardware devices.


## class abstract_cpu

Base class representing a CPU hierarchy level (Package, Core, or logical CPU).

### Key public methods
* `measurement_start()`: Starts CPU state (C-state/P-state) measurement.
* `measurement_end()`: Ends CPU state measurement.
* `finalize_cstate(...)`: Processes collected C-state data.
* `finalize_pstate(...)`: Processes collected P-state data.
* `insert_cstate(...)` / `update_cstate(...)`: Manages idle state data.

### Key public variables
* `int number`: The CPU number.
* `vector<abstract_cpu *> children`: Child levels in the CPU hierarchy.
* `const char* name`: Human-readable name/type of the CPU.

### Derived class cpu_linux
Implementation for individual logical CPUs as seen by the Linux kernel.

### Derived class cpu_core
Represents a physical CPU core.

### Derived class cpu_package
Represents a physical CPU package (socket).

### Derived class nhm_cpu, nhm_core, nhm_package, atom_core, etc.
Architecture-specific implementations (e.g., Nehalem, Atom) that handle specific MSRs and power features.


## class tunable

Base class for power-saving settings that can be "tuned" (e.g., SATA link power management).

### Key public methods
* `good_bad()`: Returns whether the current setting is optimal for power saving.
* `toggle()`: Switches between "good" and "bad" settings.
* `description()`: Returns a std::string description of the tunable setting.
* `result_string()`: Returns a std::string representation of the current state (Good/Bad/Unknown).
* `toggle_script()`: Returns a shell script command to toggle the setting.
* `toggle_script_s()`: Returns a std::string shell script command to toggle the setting.

### Key public variables
* `double score`: The estimated power saving impact.
* `std::string desc`: Description text.

### Derived class usb_tunable, ethernet_tunable, wifi_tunable, bt_tunable, etc.
Specific implementations for different subsystems that allow power-saving toggles.


## class wakeup

Base class for system entities that can wake the system from sleep or idle.

### Key public methods
* `wakeup_value()`: Returns the current enable/disable state.
* `wakeup_toggle()`: Toggles the wakeup capability.
* `description()`: Returns a std::string description of the wakeup source.

### Key public variables
* `std::string desc`: Description text.

### Derived class usb_wakeup, ethernet_wakeup
Implementations for specific hardware wakeup sources.


## class power_meter

Base class for different methods of measuring system-wide power consumption.

### Key public methods
* `power()`: Returns the current power consumption in Watts.
* `start_measurement()`: Starts a measurement interval.
* `end_measurement()`: Finalizes a measurement interval.
* `is_discharging()`: Returns true if the system is running on battery.

### Derived class acpi_power_meter
Uses ACPI battery information for measurement.

### Derived class extech_power_meter
Uses an external Extech power analyzer.

### Derived class sysfs_power_meter
Uses internal sysfs power sensors.


## class report_formatter

Interface for generating reports in different formats (HTML, CSV).

### Key public methods
* `add_title(...)`: Adds a section title to the report.
* `add_table(...)`: Adds a data table to the report.

### Derived class report_formatter_string_base
Base for formatters that output to strings/files.

### Derived class report_formatter_html, report_formatter_csv
Implement specific output formatting for HTML and CSV files.
