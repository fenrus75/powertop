class nhm_package: public abstract_cpu 
{
private:
	int package;
public:
	nhm_package(int package);
	void 	measurement_start(void);
	void 	measurement_end(void);

	void 	consolidate_children(void);
};

class nhm_core: public abstract_cpu 
{
public:
	void 	measurement_start(void);
	void 	measurement_end(void);

	void 	consolidate_children(void);
};

class atom_package: public abstract_cpu 
{
public:
	void 	measurement_start(void);
	void 	measurement_end(void);

	void 	consolidate_children(void);
};

class atom_core: public abstract_cpu 
{
public:
	void 	measurement_start(void);
	void 	measurement_end(void);

	void 	consolidate_children(void);
};

