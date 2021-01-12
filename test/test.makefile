include ../../make.config

SRCDIR=.
OBJDIR=$(BINDIR)/obj/test

OBJS=$(patsubst $(SRCDIR)/%.c,${OBJDIR}/%.o,$(wildcard $(SRCDIR)/*.c)) $(patsubst $(SRCDIR)/%.cpp,${OBJDIR}/%.o,$(wildcard $(SRCDIR)/*.cpp))
TARGET=$(patsubst $(OBJDIR)/%.o,${BINDIR}/%,${OBJS})
TEST=$(patsubst $(SRCDIR)/test%.c,${BINDIR}/test%,$(wildcard $(SRCDIR)/test*.c)) $(patsubst $(SRCDIR)/test%.cpp,${BINDIR}/test%,$(wildcard $(SRCDIR)/test*.cpp))

all: $(TARGET)

test: $(TEST)
	$(FOR) i in $^; do \
		echo $$i; \
		$$i; \
	done

${BINDIR}/%: $(DEP_LIB) $(OBJDIR)/%.o
	$(call CHECKPATH,LD,$@,$?)
	$(CC) $(LDFLAGS) $^ $(SYS_LIB) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(call CHECKPATH,CC,$@,$?)
	$(CC) $(CFLAGS) $(INC_PATH) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(call CHECKPATH,CC,$@,$?)
	$(CC) $(CFLAGS) $(INC_PATH) -c $< -o $@

DEPS=$(patsubst %.o,%.dep,${OBJS})

-include $(DEPS)

.PHONY:depend
depend: $(DEPS)

$(OBJDIR)/%.dep: $(SRCDIR)/%.c
	$(call CHECKPATH,DEP,$@,$?)
	$(CC) $(CFLAGS) $(INC_PATH) -MM -MT $(patsubst %.dep,%.o,$@) $< -o $@

$(OBJDIR)/%.dep: $(SRCDIR)/%.cpp
	$(call CHECKPATH,DEP,$@,$?)
	$(CC) $(CFLAGS) $(INC_PATH) -MM -MT $(patsubst %.dep,%.o,$@) $< -o $@

.PHONY:clean
clean:
	$(RM) $(OBJS) $(TARGET)
