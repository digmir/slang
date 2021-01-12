include ../make.config

#SRCDIR=$(dir $(wildcard ./*/Makefile))
SRCDIR=util loader

all:
	$(FOR) i in $(SRCDIR); do \
		$(call show_stage,$$i); \
		$(MAKE) -C $$i depend; \
		$(MAKE) -C $$i; \
	done

.PHONY:clean
clean:
	$(FOR) i in $(SRCDIR); do \
		$(call show_stage,$$i); \
		$(MAKE) -C $$i clean; \
	done
