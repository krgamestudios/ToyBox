#compiler settings reference
#CC=gcc
#CFLAGS+=-std=c17 -g -Wall -Werror -Wextra -Wpedantic -Wformat=2 -Wno-newline-eof
#LIBS+=-lm
#LDFLAGS+=

#directories
export GAME_SOURCEDIR=source
export GAME_OUTDIR=out
export GAME_OBJDIR=obj

#Override Toy's build destination
export TOY_SOURCEDIR=Toy/source
export TOY_OUTDIR=../$(GAME_OUTDIR)
export TOY_OBJDIR=$(GAME_OBJDIR)

#targets
all: $(GAME_OUTDIR) Toy source

.PHONY: source
source:
	$(MAKE) -C $(GAME_SOURCEDIR)

.PHONY: Toy
Toy:
	$(MAKE) -C $(TOY_SOURCEDIR)

#util targets
$(GAME_OUTDIR):
	mkdir $(GAME_OUTDIR)

#util commands
.PHONY: clean
clean:
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

