.DEFAULT_GOAL:=default

export BS:=$(dir $(lastword $(MAKEFILE_LIST)))

include $(BS)/export.mk

variable_exporter=T='$1' $(foreach v,$(export_$2),$1_$v='$($1_$v)')
makecommand=$(MAKE) -f $(BS)/second_$2.mk generated/$1 $(call variable_exporter,$1,$2)

define targetrule
generated/$1: FORCE
	@mkdir -p generated
	@echo "$(call makecommand,$1,$($1_rule))" > generated/$1.log
	@echo >> generated/$1.log
	+@$(call makecommand,$1,$($1_rule)) >> generated/$1.log

endef

$(eval $(foreach t,$(TARGETS), $(call targetrule,$t)))

clean: extraclean
	@rm -rf generated

extraclean:

FORCE::


