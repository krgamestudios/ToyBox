#compiler settings reference
#CC=gcc
#CFLAGS+=-std=c17 -g -Wall -Werror -Wextra -Wpedantic -Wformat=2 -Wno-newline-eof
#LIBS+=-lm
#LDFLAGS+=

#TODO: add debug & release build options, at least for Toy

#directories
export BOX_SOURCEDIR=source
export BOX_OUTDIR=out
export BOX_OBJDIR=obj

#Override Toy's build destination
export TOY_SOURCEDIR=Toy/source
export TOY_OUTDIR=../$(BOX_OUTDIR)
export TOY_OBJDIR=$(BOX_OBJDIR)

#targets
all: $(BOX_OUTDIR) Toy tools source tools-clean

.PHONY: source
source:
	$(MAKE) -C $(BOX_SOURCEDIR)

.PHONY: Toy
Toy:
	$(MAKE) -C $(TOY_SOURCEDIR) -k

#copy the various tools into the source repo
.PHONY: tools
tools:
	cp Toy/repl/*library* $(BOX_SOURCEDIR)
	cp Toy/repl/*inspector* $(BOX_SOURCEDIR)

.PHONY: tools-clean
tools-clean:
ifeq ($(shell uname),Linux)
	find . -type f -wholename "./$(BOX_SOURCEDIR)/*inspector*" -delete
	find . -type f -wholename "./$(BOX_SOURCEDIR)/*library*" -delete
else
	@echo "tools-clean failed, check the makefile and add this platform"
endif

#utils
$(BOX_OUTDIR):
	mkdir $(BOX_OUTDIR)

.PHONY: clean
clean: tools-clean
ifeq ($(shell uname),Linux)
	find . -type f -name '*.o' -delete
	find . -type f -name '*.a' -delete
	find . -type f -name '*.out' -delete
	find . -type f -name '*.exe' -delete
	find . -type f -name '*.dll' -delete
	find . -type f -name '*.lib' -delete
	find . -type f -name '*.so' -delete
	find . -type f -name '*.dylib' -delete
	find . -type d -name 'out' -delete
	find . -type d -name 'obj' -delete
else ifeq ($(shell uname),NetBSD)
	find . -type f -name '*.o' -delete
	find . -type f -name '*.a' -delete
	find . -type f -name '*.out' -delete
	find . -type f -name '*.exe' -delete
	find . -type f -name '*.dll' -delete
	find . -type f -name '*.lib' -delete
	find . -type f -name '*.so' -delete
	find . -type f -name '*.dylib' -delete
	find . -type d -name 'out' -delete
	find . -type d -name 'obj' -delete
else ifeq ($(OS),Windows_NT)
	$(RM) *.o *.a *.exe *.dll *.lib *.so *.dylib
	$(RM) out
	$(RM) obj
else ifeq ($(shell uname),Darwin)
	find . -type f -name '*.o' -delete
	find . -type f -name '*.a' -delete
	find . -type f -name '*.out' -delete
	find . -type f -name '*.exe' -delete
	find . -type f -name '*.dll' -delete
	find . -type f -name '*.lib' -delete
	find . -type f -name '*.so' -delete
	find . -type f -name '*.dylib' -delete
	find . -type d -name 'out' -delete
	find . -type d -name 'obj' -delete
else
	@echo "Deletion failed - what platform is this?"
endif

