include $(BS)/second_common.mk

ifeq ($($T_cpp),)
  CPP=$($T_cc)
else
  CPP=$($T_cpp)
endif

ALLCFILES:=$(subst .c,,$(foreach s,$($T_sourcesearchdirs),$(wildcard $s/*.c)))
ALLCFILES+=$(subst .cpp,,$(foreach s,$($T_sourcesearchdirs),$(wildcard $s/*.cpp)))

-include $(foreach n,$(notdir $(basename $(ALLCFILES))),generated/$n.$T.d)

vpath % $(foreach d,$($T_sourcesearchdirs),$d:)

DASHISEARCH:=$(foreach d,$($T_sourcesearchdirs),-I$d ) $(foreach d,$($T_headersearchdirs),-I$d )

linkdepsrec = $(if $(findstring $1 ,$(CURDEPS)),,$(eval CURDEPS+=$1 )$(foreach d,$(LINKDEPS_$1),$(call linkdepsrec,$d)))
linkdeps = $(eval CURDEPS:=)$(call linkdepsrec,$(notdir $1))$(foreach d,$(CURDEPS),generated/$d.$T.obj)

AUTODEPOBJECTS:=$(sort $(foreach f,$($T_autodepsources),$(call linkdeps,$(basename $f))))
OBJECTS:=$(foreach f,$(patsubst %.cpp,%.$T.obj,$($T_sources:.c=.$T.obj)),generated/$f)

generated/%.$T.d: %.c
	@$(shell mkdir -p generated)
	@$(shell $($T_cc) $(DASHISEARCH) -Igenerated/skippeddependencies/$T -M -MG $< | sed "s/.*: //" > $@.tmp)
	@echo $@ $(@:.d=.obj): $(shell cat $@.tmp) > $@
	@echo LINKDEPS_$(notdir $(basename $<)) = $(foreach d,$(filter-out $(notdir $(basename $@)),$(notdir $(basename $(filter %.h,$(shell cat $@.tmp))))), $(filter $d,$(notdir $(ALLCFILES)))) >> $@
	@rm -f $@.tmp

generated/%.$T.d: %.cpp
	@$(shell mkdir -p generated)
	@$(shell $(CPP) $(DASHISEARCH) -Igenerated/skippeddependencies/$T -M -MG $< | sed "s/.*: //" > $@.tmp)
	@echo $@ $(@:.d=.obj): $(shell cat $@.tmp) > $@
	@echo LINKDEPS_$(notdir $(basename $<)) = $(foreach d,$(filter-out $(notdir $(basename $@)),$(notdir $(basename $(filter %.h,$(shell cat $@.tmp))))), $(filter $d,$(notdir $(ALLCFILES)))) >> $@
	@rm -f $@.tmp

generated/%.$T.obj: %.c generated/%.$T.d
	mkdir -p generated
	$(call print,CC $<)
	$($T_cc) -g $(DASHISEARCH) $2 -o $@ -c $<

generated/%.$T.obj: %.cpp generated/%.$T.d
	mkdir -p generated
	$(call print,CPP $<)
	$(CPP) -g $(DASHISEARCH) $2 -o $@ -c $<

generated/$T: $(OBJECTS) $(AUTODEPOBJECTS) $($T_staticlibs) $($T_extradeps) $(foreach l,$($T_dynamiclibs),-l$l)
	@echo objects: $(OBJECTS)
	@echo auto dep objects: $(AUTODEPOBJECTS)
	$(call print,LD $@)
	$($T_ld) -o $@ $(OBJECTS) $(AUTODEPOBJECTS) $($T_staticlibs) $(foreach l,$($T_dynamiclibs),-l$l )

# Better error if a header file cannot be found/generated
%.h:
	mkdir -p generated/skippeddependencies/$T
	touch generated/skippeddependencies/$T/$(notdir $@)

