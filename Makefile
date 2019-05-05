CC      	= clang
CFLAGS		= -Wall
LDFLAGS 	=
BUILDDIR 	= build
SOURCEDIR 	= src
OBJECTDIR 	= obj
OUTPUT 		= headstripper
PREFIX 		= /

SRCS = $(wildcard $(SOURCEDIR)/*.c)
OBJS = $(SRCS:.c=.o)
OBJ  = $(OBJS:$(SOURCEDIR)/%=$(OBJECTDIR)/%)

build: dir $(OBJ)
	@echo [LD] $(OBJ)
	@$(CC) $(CFLAGS) -o $(BUILDDIR)/$(OUTPUT) $(OBJ) $(LDFLAGS)

debug: CFLAGS += -g -D _DEBUG
debug: build;

#CLEAN BEFORE WINDOWS BUILD!!!
windows: CC = x86_64-w64-mingw32-gcc
windows: OUTPUT =hs.exe
windows: build;

windows_debug: CC = x86_64-w64-mingw32-gcc
windows_debug: OUTPUT =hs.exe
windows_debug: CFLAGS += -g -D _DEBUG
windows_debug: build;


dir:
	@mkdir -p $(OBJECTDIR)
	@mkdir -p $(BUILDDIR)

$(OBJECTDIR)/%.o: $(SOURCEDIR)/%.c
	@echo [CC] $<
	@$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	@echo [RM] $(OBJ)
	@echo [RM] $(BUILDDIR)/$(OUTPUT)
	@rm -df  $(OBJ)
	@rm -Rdf $(BUILDDIR) $(OBJECTDIR)

install: build
	@cp $(BUILDDIR)/$(OUTPUT) $(PREFIX)/bin/$(OUTPUT)
