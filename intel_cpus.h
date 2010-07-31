class nhm_package: public cpu_package 
{
public:
	void 	measurement_start(void);
	void 	measurement_end(void);

	void 	consolidate_children(void);
};

class nhm_core: public cpu_core 
{
public:
	void 	measurement_start(void);
	void 	measurement_end(void);

	void 	consolidate_children(void);
};

class atom_package: public cpu_package 
{
public:
	void 	measurement_start(void);
	void 	measurement_end(void);

	void 	consolidate_children(void);
};

class atom_core: public cpu_core
{
public:
	void 	measurement_start(void);
	void 	measurement_end(void);

	void 	consolidate_children(void);
};

