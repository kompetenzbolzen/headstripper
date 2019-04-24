CC      	= clang
CFLAGS		= -Wall
LDFLAGS 	=
BUILDDIR 	= build
SOURCEDIR 	= src
OBJECTDIR 	= obj
OUTPUT 		= headstripper

SRCS = $(wildcard $(SOURCEDIR)/*.c)
OBJS = $(SRCS:.c=.o)
OBJ  = $(OBJS:$(SOURCEDIR)/%=$(OBJECTDIR)/%)

build: dir $(OBJ)
	@echo [LD] $(OBJ)
	@$(CC) $(CFLAGS) -o $(BUILDDIR)/$(OUTPUT) $(OBJ) $(LDFLAGS)

debug: CFLAGS += -g -D _DEBUG
debug: build;

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
