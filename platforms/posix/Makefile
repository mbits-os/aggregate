FILES = \
	../3rdparty.files \
	../dbtool.files \
	../libenv.files \
	../server.files

all: Makefile.gen
	$(MAKE) -f Makefile.gen all

clean: Makefile.gen
	$(MAKE) -f Makefile.gen clean
	rm Makefile.gen

Makefile.gen: $(FILES) gen_makefiles.py Project.py
	python gen_makefiles.py 2>&1 >Makefile.gen
