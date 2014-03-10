FILES = \
	../../3rdparty/3rdparty.files \
	../../dbtool/dbtool.files \
	../../libenv/libenv.files \
	../../libremote/libremote.files \
	../../server/server.files

all clean preinstall install clean_strings strings all_a3rdparty all_libenv all_libremote all_dbtool all_server clean_distro: Makefile.gen
	@$(MAKE) -C . -f Makefile.gen $@

clean_gen:
	rm Makefile.gen

Makefile.gen: $(FILES) gen_makefiles.py distro.mak Project.py
	@echo [ PY ] Makefile; python gen_makefiles.py 2>&1 >Makefile.gen
