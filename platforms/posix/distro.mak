ROOT = ../..
DISTRO = $(ROOT)/package
DISTRO_USR = www-data
DISTRO_GRP = www-data

DISTRO_FILES = \
	$(ROOT)/httpd/bin/dbtool \
	$(ROOT)/httpd/bin/reedr

DISTRO_FILES_ETC = \
	$(ROOT)/httpd/config/reedr.conf \
	$(ROOT)/httpd/config/connection.ini \
	$(ROOT)/httpd/config/smtp.ini \
	$(ROOT)/httpd/config/nginx.conf

COPY_CONTENT = \
	$(ROOT)/httpd/data/images \
	$(ROOT)/httpd/data/locales \
	$(ROOT)/httpd/www

DISTRO_DIRS = \
	/etc/reedr \
	/var/log/reedr \
	/usr/share/reedr

WILDCARDS = \
	* \
	*/* \
	*/*/* \
	*/*/*/* \
	*/*/*/*/* \
	*/*/*/*/*/* \
	*/*/*/*/*/*/* \
	*/*/*/*/*/*/*/*

COPIED_CONTENT = $(shell python $(CURDIR)/files_only.py $(sort $(filter-out %.xcf, $(foreach dir, $(COPY_CONTENT), $(foreach card, $(WILDCARDS), $(wildcard $(dir)$(card)))))))
DISTRO_FILES += $(COPIED_CONTENT)

EMPTY_DIRS = \
	$(DISTRO)/var/log/reedr

DIST_FILES = $(addprefix $(DISTRO)/usr/share/reedr/, $(patsubst $(ROOT)/httpd/%, %, $(DISTRO_FILES)))
DIST_FILES_ETC = $(addprefix $(DISTRO)/etc/reedr/, $(patsubst $(ROOT)/httpd/config/%, %, $(DISTRO_FILES_ETC)))

DIRS = $(addprefix $(DISTRO), $(DISTRO_DIRS))

install: preinstall $(DIST_FILES) $(DIST_FILES_ETC) $(EMPTY_DIRS)
	@python $(CURDIR)/copy_distro.py "$(DISTRO)"
	@chown -R $(DISTRO_USR):$(DISTRO_GRP) /var/log/reedr

clean_distro:
	@echo [ RM ] distro
	@rm -rf $(DIRS)
	@python $(CURDIR)/remove_empty.py $(DIRS)

$(EMPTY_DIRS):
	@echo [ MK ] $(patsubst $(DISTRO)%, %, $@)
	@mkdir -p $@

$(DISTRO)/usr/share/reedr/bin%:  $(ROOT)/httpd/bin%
	@echo [ CP ] $(patsubst $(DISTRO)%, %, $@)
	@mkdir -p $(dir $@)
	@chmod 0755 $(dir $@)
	@cp $< $@
	@chmod 0755 $@

$(DISTRO)/usr/share/reedr/data%:  $(ROOT)/httpd/data%
	@echo [ CP ] $(patsubst $(DISTRO)%, %, $@)
	@mkdir -p $(dir $@)
	@chmod 0755 $(dir $@)
	@cp $< $@
	@chmod 0644 $@

$(DISTRO)/usr/share/reedr/www%:  $(ROOT)/httpd/www%
	@echo [ CP ] $(patsubst $(DISTRO)%, %, $@)
	@mkdir -p $(dir $@)
	@chmod 0755 $(dir $@)
	@cp $< $@
	@chmod 0644 $@

$(DISTRO)/etc/reedr%:  $(ROOT)/httpd/config%
	@echo [ CP ] $(patsubst $(DISTRO)%, %, $@)
	@mkdir -p $(dir $@)
	@chmod 0755 $(dir $@)
	@cp $< $@
	@chmod 0644 $@
