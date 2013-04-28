LANGS=pl-PL en
DEFINES_FILE=../libenv/includes/site_strings.h
CLIENT_FILES=../httpd/static/code/lang/
EXPORTS=client.strings
CPS_IN= \
	iso-8859-1 \
	iso-8859-2 \
	iso-8859-3 \
	iso-8859-4 \
	iso-8859-5 \
	iso-8859-6 \
	iso-8859-8 \
	iso-8859-9 \
	iso-8859-15 \
	koi8-r \
	koi8-u \
	windows-437 \
	windows-720 \
	windows-737 \
	windows-775 \
	windows-850 \
	windows-852 \
	windows-855 \
	windows-857 \
	windows-858 \
	windows-862 \
	windows-866 \
	windows-874 \
	windows-1250 \
	windows-1251 \
	windows-1252 \
	windows-1253 \
	windows-1254 \
	windows-1255 \
	windows-1256 \
	windows-1257 \
	windows-1258

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
EXPORT=python ./code/export.py
CPS_PY=python ./code/cp.py
CHARSET_MERGE=python ./code/charsets.py
MKDIR=mkdir -p
CP=cp -f
RM=rm -f

SAVE_LANGS = $(subst -,_,$(LANGS))
POS = $(addsuffix /site_strings.po,$(SAVE_LANGS))
MOS = $(addsuffix /site_strings.mo,$(SAVE_LANGS))
LNGS = $(addprefix ../httpd/locales/, $(addsuffix /site_strings.lng,$(SAVE_LANGS)))
EXPORTED = $(addprefix $(CLIENT_FILES), $(addsuffix .js, $(foreach file, $(patsubst %.strings,%-,$(EXPORTS)), $(foreach lang, $(SAVE_LANGS), $(file)$(lang)))))
SAFE_CPS = $(subst -,_,$(CPS_IN))
CPS = $(addprefix ../int/strings/charset/,$(addsuffix .dat,$(SAFE_CPS)))
CPS_TXT = $(addprefix ./charset/,$(addsuffix .txt,$(SAFE_CPS)))
CHARSET_DB = ../httpd/locales/charset.db

get-locale=$(subst _,-,$(word 1,$(subst /, ,$1)))
get-locale-2=$(subst _,-,$(word 4,$(subst /, ,$1)))

all: $(DEFINES_FILE) $(LNGS) $(EXPORTED) $(CHARSET_DB)

help:
	$(Q)$(ECHO) -e "Targets are:\n    all\n    clean\n    help\n    update - extract new strings and distribute them\n    msgs - compile .mo files"

update: site_strings.pot $(POS)

clean:
	$(Q)$(RM) $(MOS)
	$(Q)$(RM) $(LNGS)
	$(Q)$(RM) $(DEFINES_FILE)
	$(Q)$(RM) $(EXPORTED)
	$(Q)$(RM) $(CHARSET_DB) $(CPS)

msgs: $(MOS)

.PHONY: all clean update msgs

$(DEFINES_FILE): site_strings.txt
	$(Q)$(ECHO) "[ENUM]" $(notdir $@)
	$(Q)$(MKDIR) -p ../httpd/locales
	$(Q)$(DEFINE) site_strings.txt $(DEFINES_FILE)

site_strings.pot: site_strings.txt
	$(Q)$(ECHO) "[LANG]" $@
	$(Q)$(MKDIR) ../httpd/locales
	$(Q)$(EXTRACT) site_strings.txt site_strings.pot

%.mo: %.po
	$(Q)$(ECHO) "[ MO ]" $(call get-locale,$@)
	$(Q)$(MKDIR) $(dir $@)
	$(Q)$(MSGFMT) -c -o $@ $<

$(LNGS): $(MOS)
	$(Q)$(ECHO) "[LANG]" $(call get-locale-2,$@)
	$(Q)$(MKDIR) $(dir $@)
	$(Q)$(DEPLOY) site_strings.txt $(subst -,_,$(call get-locale-2,$@))/site_strings.mo $@ $(call get-locale-2,$@)

init-command=$(MSGINIT) $2 $(subst _,-,$(subst /site_strings.po,,$1)) > $1
merge-command=$(MSGMERGE) -o $1 $1 $2 2>/dev/null

$(EXPORTED): $(LNGS) $(EXPORTS)
	$(Q)$(MKDIR) $(dir $@)
	$(Q)$(EXPORT) site_strings.txt site_strings.mo $@

$(POS): site_strings.pot
	$(Q)$(ECHO) $(if $(wildcard $@),"[LANG]","[NEW ]") $(call get-locale,$@)
	$(Q)$(MKDIR) $(dir $@)
	$(Q)$(if $(wildcard $@),$(call merge-command,$@,$<),$(call init-command,$@,$<))

$(CHARSET_DB): $(CPS)
	@echo "[ DB ]" $@
	@mkdir -p $(dir $@)
	$(Q)$(CHARSET_MERGE) $@ $^
	
../int/strings/charset/%.dat: ./charset/%.txt
	@echo "[ CH ]" $(subst _,-,$(*F))
	@mkdir -p $(dir $@)
	@$(CPS_PY) $< $@ 

