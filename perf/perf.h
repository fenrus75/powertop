
using namespace std;

class  perf_event {
protected:
	int perf_fd;
	void * perf_mmap;
	void * data_mmap;
	struct perf_event_mmap_page *pc;



	int bufsize;
	char *name;
	unsigned int trace_type;
	void create_perf_event(char *eventname);

public:
	perf_event(const char *event_name, int buffer_size = 128);

	void start(void);
	void stop(void);
};
