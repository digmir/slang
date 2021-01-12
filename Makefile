ifeq ($(TARGETOS),)
	export TARGETOS = linux64
endif
export ROOTDIR=$(CURDIR)
export BINDIR=$(ROOTDIR)/bin/$(TARGETOS)

include make.config

SRCDIR=src test

all:
	$(call show_begin)
	$(FOR) i in $(SRCDIR); do \
		$(call show_stage,$$i); \
		$(MAKE) -C $$i; \
	done
	$(call show_end)

.PHONY:clean
clean:
	$(FOR) i in $(SRCDIR); do \
		$(MAKE) -C $$i clean; \
	done

.PHONY:test
test:
	$(call show_begin)
	@ $(MAKE) -C test test
	$(call show_end)
