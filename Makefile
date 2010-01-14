     CC := gcc
  STRIP := strip
     CP := cp
	 LD := ld

 SOURCE := libsopt.c 
   OBJS := libsopt.o
INSTDIR := /usr/local/lib

   CFLAGS := -Wtraditional -W -Wall -Wshadow -Wundef
   CFLAGS += -O9 -funroll-loops -fomit-frame-pointer -ffast-math -pipe
   CFLAGS += $(shell if $(CC) -S -march=`uname -m` -o /dev/null -xc /dev/null >/dev/null 2>&1; then echo "-march=`uname -m`" ; fi)
   CFLAGS += $(shell if $(CC) -S -fstrict-aliasing -o /dev/null -xc /dev/null >/dev/null 2>&1; then echo "-fstrict-aliasing" ; fi)
   CFLAGS += -fPIC -static -nostdlib
  LDFLAGS := -G /usr/lib/libdl.so
ifdef USE_BSD
   CFLAGS += -DUSE_BSD
  LDFLAGS :=
endif
  TARGETS := libsopt.so

DEPS := $(OBJS:%.o=.deps/%.d)
 FOO := $(shell mkdir .deps >/dev/null 2>&1 || :)

all:
	$(MAKE) all-targets

all-targets: $(TARGETS)


.SUFFIXES: .o .a
	$(CC) $(CFLAGS) -c $< -o $@ $(DEFINES)
	

$(TARGETS): $(OBJS)
	$(LD) -o $@ $(LDFLAGS) $<
	$(STRIP) --strip-all $@

clean:
	rm -rf $(TARGETS) *~ .deps *.o

install:
	@echo "export LD_PRELOAD=$(INSTDIR)/$(TARGETS) and run any program or"
	@echo "echo $(INSTDIR)/$(TARGETS) > /etc/ld.so.preload if you´re an "
	@echo "dangerous boy! ;) try the LD_PRELOAD first of all."
	@-$(CP) $(TARGETS) $(INSTDIR)
	

-include $(DEPS)

$(DEPS):;

# Enhanced compiling ruleset that also generate dependencies. It's quite
# ugly, like anything borrowed from automake/libtool :)

%.o: %.c
	@echo '$(CC) $(CFLAGS) -c $< -o $@'; \
	$(CC) $(CFLAGS) -Wp,-MD,.deps/$(*F).pp -c $< -o $@
	@-cp .deps/$(*F).pp .deps/$(*F).d; \
	tr ' ' '\012' <.deps/$(*F).pp \
	| sed -e 's/^\\$$//' -e '/^$$/ d' -e '/:$$/ d' -e 's/$$/ :/' \
	>> .deps/$(*F).d; \
	rm .deps/$(*F).pp

.PHONY: clean all all-targets
