FILES = \
	../3rdparty.files \
	../dbtool.files \
	../libenv.files \
	../libremote.files \
	../server.files

all clean install: Makefile.gen
	@$(MAKE) -C . -f Makefile.gen $@

clean_gen:
	rm Makefile.gen

Makefile.gen: $(FILES) gen_makefiles.py Project.py
	@echo [ PY ] Makefile; python gen_makefiles.py 2>&1 >Makefile.gen
