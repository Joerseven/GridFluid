# GNU Make workspace makefile autogenerated by Premake

ifndef config
  config=debug
endif

ifndef verbose
  SILENT = @
endif

ifeq ($(config),debug)
  Fluid_config = debug
endif
ifeq ($(config),release)
  Fluid_config = release
endif

PROJECTS := Fluid

.PHONY: all clean help $(PROJECTS) 

all: $(PROJECTS)

Fluid:
ifneq (,$(Fluid_config))
	@echo "==== Building Fluid ($(Fluid_config)) ===="
	@${MAKE} --no-print-directory -C . -f Fluid.make config=$(Fluid_config)
endif

clean:
	@${MAKE} --no-print-directory -C . -f Fluid.make clean

help:
	@echo "Usage: make [config=name] [target]"
	@echo ""
	@echo "CONFIGURATIONS:"
	@echo "  debug"
	@echo "  release"
	@echo ""
	@echo "TARGETS:"
	@echo "   all (default)"
	@echo "   clean"
	@echo "   Fluid"
	@echo ""
	@echo "For more information, see https://github.com/premake/premake-core/wiki"