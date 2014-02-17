ROOT = ../..
DISTRO = $(ROOT)/distro

DISTRO_FILES = \
	$(ROOT)/httpd/bin/dbtool \
	$(ROOT)/httpd/bin/server

DISTRO_FILES_ETC = \
	$(ROOT)/httpd/config/reedr.conf \
	$(ROOT)/httpd/config/connection.ini \
	$(ROOT)/httpd/config/smtp.ini \
	$(ROOT)/httpd/config/nginx.conf

COPY_CONTENT = \
	$(ROOT)/httpd/data/images \
	$(ROOT)/httpd/data/locales \
	$(ROOT)/httpd/www

WILDCARDS = \
	* \
	*/* \
	*/*/* \
	*/*/*/* \
	*/*/*/*/* \
	*/*/*/*/*/* \
	*/*/*/*/*/*/* \
	*/*/*/*/*/*/*/*

COPIED_CONTENT = $(shell ./files_only.py $(sort $(filter-out %.xcf, $(foreach dir, $(COPY_CONTENT), $(foreach card, $(WILDCARDS), $(wildcard $(dir)$(card)))))))
DISTRO_FILES += $(COPIED_CONTENT)

EMPTY_DIRS = \
	$(DISTRO)/var/log/reedr

DIST_FILES = $(addprefix $(DISTRO)/usr/share/reedr/, $(patsubst $(ROOT)/httpd/%, %, $(DISTRO_FILES)))
DIST_FILES_ETC = $(addprefix $(DISTRO)/etc/reedr/, $(patsubst $(ROOT)/httpd/config/%, %, $(DISTRO_FILES_ETC)))

distro: install $(DIST_FILES) $(DIST_FILES_ETC) $(EMPTY_DIRS)

clean_distro:
	@echo [ RM ] distro
	@rm -rf $(DISTRO)

$(EMPTY_DIRS):
	@echo [ MK ] $(patsubst $(DISTRO)%, %, $@)
	@mkdir -p $@

$(DISTRO)/usr/share/reedr/bin%:  $(ROOT)/httpd/bin%
	@echo [ CP ] $(patsubst $(DISTRO)%, %, $@)
	@mkdir -p $(dir $@)
	@cp $< $@

$(DISTRO)/usr/share/reedr/data%:  $(ROOT)/httpd/data%
	@echo [ CP ] $(patsubst $(DISTRO)%, %, $@)
	@mkdir -p $(dir $@)
	@cp $< $@

$(DISTRO)/usr/share/reedr/www%:  $(ROOT)/httpd/www%
	@echo [ CP ] $(patsubst $(DISTRO)%, %, $@)
	@mkdir -p $(dir $@)
	@cp $< $@

$(DISTRO)/etc/reedr%:  $(ROOT)/httpd/config%
	@echo [ CP ] $(patsubst $(DISTRO)%, %, $@)
	@mkdir -p $(dir $@)
	@cp $< $@
