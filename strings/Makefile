LANGS=pl-PL en
DEFINES_FILE=../libenv/includes/site_strings.h

Q=@

ECHO=echo
TOUCH=touch
XGETTEXT=xgettext
MSGFMT=msgfmt
MSGINIT=python ./code/msginit.py
MSGMERGE=msgmerge
DEPLOY=python ./code/extract.py
EXTRACT=python ./code/strings.py
DEFINE=python ./code/defines.py
MKDIR=mkdir -p
CP=cp -f
RM=rm -f

SAVE_LANGS = $(subst -,_,$(LANGS))
POS = $(addsuffix /site_strings.po,$(SAVE_LANGS))
MOS = $(addsuffix /site_strings.mo,$(SAVE_LANGS))
LNGS = $(addprefix ../apache/locales/, $(addsuffix /site_strings.lng,$(SAVE_LANGS)))

get-locale=$(subst _,-,$(word 1,$(subst /, ,$1)))
get-locale-2=$(subst _,-,$(word 4,$(subst /, ,$1)))

all: $(DEFINES_FILE) $(LNGS)

help:
	$(Q)$(ECHO) -e "Targets are:\n    all\n    clean\n    help\n    update - extract new strings and distribute them\n    msgs - compile .mo files"

update: site_strings.pot $(POS)

clean:
	$(Q)$(RM) $(MOS)
	$(Q)$(RM) $(LNGS)
	$(Q)$(RM) $(DEFINES_FILE) 

msgs: $(MOS)

.PHONY: all clean update msgs

$(DEFINES_FILE): site_strings.txt
	$(Q)$(ECHO) "Writing" $(notdir $@)
	$(Q)$(MKDIR) ../app/locales
	$(Q)$(DEFINE) site_strings.txt $(DEFINES_FILE)

site_strings.pot: site_strings.txt
	$(Q)$(ECHO) "Extracting" $@
	$(Q)$(MKDIR) ../app/locales
	$(Q)$(EXTRACT) site_strings.txt site_strings.pot

%.mo: %.po
	$(Q)$(ECHO) "Compiling" $(call get-locale,$@)
	$(Q)$(MKDIR) $(dir $@)
	$(Q)$(MSGFMT) -c -o $@ $<

$(LNGS): $(MOS)
	$(Q)$(ECHO) "Deploying" $(call get-locale-2,$@)
	$(Q)$(MKDIR) $(dir $@)
	$(Q)$(DEPLOY) site_strings.txt $(subst -,_,$(call get-locale-2,$@))/site_strings.mo $@ $(call get-locale-2,$@)

init-command=$(MSGINIT) $2 $(subst _,-,$(subst /site_strings.po,,$1)) > $1
merge-command=$(MSGMERGE) -o $1 $1 $2 2>/dev/null

$(POS): site_strings.pot
	$(Q)$(ECHO) $(if $(wildcard $@),"Updating","Creating") $(call get-locale,$@)
	$(Q)$(MKDIR) $(dir $@)
	$(Q)$(if $(wildcard $@),$(call merge-command,$@,$<),$(call init-command,$@,$<))
