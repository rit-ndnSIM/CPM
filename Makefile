CC=g++
CFLAGS=-Wall -Weffc++ -Wextra -Wconversion -Wsign-conversion -Wshadow -pedantic -lboost_graph -lboost_program_options -std=c++17
DFLAGS=-ggdb
RFLAGS=-O2 -DNDEBUG -Werror

ext=cpp
srcdir=.
objdir=obj
makedir=make
sources=$(wildcard $(srcdir)/*.$(ext))
objects=$(sources:$(srcdir)/%.$(ext)=$(objdir)/%.o)
makefiles=$(sources:$(srcdir)/%.$(ext)=$(makedir)/%.d)
binary=cpm

debug: CFLAGS:=$(CFLAGS) $(DFLAGS)
debug: $(binary)

release: CFLAGS:=$(CFLAGS) $(RFLAGS)
release: clean
release: $(binary)

$(binary): $(objects)
	$(CC) $^ -o $@ $(CFLAGS)

$(objdir)/%.o: $(srcdir)/%.$(ext) | $(objdir)
	$(CC) $< -o $@ $(CFLAGS) -c

include $(makefiles)

$(makedir)/%.d: $(srcdir)/%.$(ext) | $(makedir)
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(objdir)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(objdir):
	mkdir $(objdir)

$(makedir):
	mkdir $(makedir)

run: $(binary)
	@./$(binary)

clean:
	-rm -f $(objects) $(makefiles) $(binary)

.PHONY: Makefile run clean debug release
