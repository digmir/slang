include ../../make.config

SRCDIR=.
OBJDIR=$(BINDIR)/obj/$(PROJNAME)

LDFLAGS += -L$(BINDIR)

OBJS=$(patsubst $(SRCDIR)/%.c,${OBJDIR}/%.o,$(wildcard $(SRCDIR)/*.c)) $(patsubst $(SRCDIR)/%.cpp,${OBJDIR}/%.o,$(wildcard $(SRCDIR)/*.cpp))

ifeq ($(TARGET_TYPE),LIB)
TARGET=$(BINDIR)/lib$(PROJNAME).a
all: $(TARGET)
$(TARGET): $(OBJS)
	$(call CHECKPATH,AR,$@,$?)
	$(LD) -r -o $@ $^
else
ifeq ($(TARGETOS),win32)
	TARGET=$(BINDIR)/$(PROJNAME).exe
else ifeq ($(TARGETOS),win64)
	TARGET=$(BINDIR)/$(PROJNAME).exe
else
	TARGET=$(BINDIR)/$(PROJNAME)
endif
all: $(TARGET)
$(TARGET): $(DEP_LIB) $(OBJS)
	$(call CHECKPATH,LD,$@,$?)
	$(CC) $(LDFLAGS) $^ $(SYS_LIB) -o $@
endif

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(call CHECKPATH,CC,$@,$?)
	$(CC) $(CFLAGS) $(INC_PATH) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(call CHECKPATH,CPP,$@,$?)
	$(CPP) $(CFLAGS) $(INC_PATH) -c $< -o $@

DEPS=$(patsubst %.o,%.dep,${OBJS})

-include $(DEPS)

.PHONY:depend
depend: $(DEPS)

$(OBJDIR)/%.dep: $(SRCDIR)/%.c
	$(call CHECKPATH,DEP,$@,$?)
	$(CC) $(CFLAGS) $(INC_PATH) -MM -MT $(patsubst %.dep,%.o,$@) $< -o $@

$(OBJDIR)/%.dep: $(SRCDIR)/%.cpp
	$(call CHECKPATH,DEP,$@,$?)
	$(CPP) $(CFLAGS) $(INC_PATH) -MM -MT $(patsubst %.dep,%.o,$@) $< -o $@

.PHONY:clean
clean:
	$(RM) $(OBJS) $(DEPS) $(TARGET)
