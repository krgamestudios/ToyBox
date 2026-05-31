#compiler settings
CC=gcc
CFLAGS+=-std=c17 -g -Wall -Werror -Wextra -Wpedantic -Wformat=2 -Wno-newline-eof
LIBS+=-lToy -lraylib -lX11 -lm
LDFLAGS+=-Wl,-rpath,'$$ORIGIN'

ifeq ($(shell uname),Darwin) #make sure there's enough space for the dylib fix
	LDFLAGS+=-Wl,-headerpad_max_install_names
endif

#directories
GAME_ROOTDIR=..
GAME_SOURCEDIR=.
GAME_TOYDIR=$(GAME_ROOTDIR)/$(TOY_SOURCEDIR)

GAME_OUTDIR=$(GAME_ROOTDIR)/$(BOX_OUTDIR)
GAME_OBJDIR=$(BOX_OBJDIR)

#file names
GAME_SOURCEFILES=$(wildcard $(GAME_SOURCEDIR)/*.c)
GAME_OBJFILES=$(addprefix $(GAME_OBJDIR)/,$(notdir $(GAME_SOURCEFILES:.c=.o)))
GAME_TARGETNAME=BOXires

#file extensions
ifeq ($(OS),Windows_NT)
	GAME_TARGETEXT=.exe
else
	GAME_TARGETEXT=.out
endif

#linker fix
LDFLAGS+=-L$(realpath $(GAME_OUTDIR))

#build the object files, compile the test cases, and run
all: build link

#targets for each step
.PHONY: build
build: $(GAME_OBJDIR) $(GAME_OBJFILES)

.PHONY: link
link: $(GAME_OUTDIR) $(GAME_OUTDIR)/$(GAME_TARGETNAME)$(GAME_TARGETEXT)

#util targets
$(GAME_OUTDIR):
	mkdir $(GAME_OUTDIR)

$(GAME_OBJDIR):
	mkdir $(GAME_OBJDIR)

#compilation steps
$(GAME_OBJDIR)/%.o: $(GAME_SOURCEDIR)/%.c
	$(CC) -c -o $@ $< $(addprefix -I,$(GAME_SOURCEDIR)) $(addprefix -I,$(GAME_TOYDIR)) $(CFLAGS)

$(GAME_OUTDIR)/$(GAME_TARGETNAME)$(GAME_TARGETEXT): $(GAME_OBJFILES)
	$(CC) -DTOY_IMPORT $(CFLAGS) -o $@ $(GAME_OBJFILES) $(LDFLAGS) $(LIBS)
ifeq ($(shell uname),Darwin) #dylib fix
	otool -L $@
	install_name_tool -add_rpath @executable_path/. $@
	install_name_tool -change $(GAME_OUTDIR)/libToy.dylib @executable_path/libToy.dylib $@
	otool -L $@
endif
