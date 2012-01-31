CPPFLAGS += -Wall -Wextra -Wno-unused-parameter
CPPFLAGS += -fPIC -Iinclude
CPPFLAGS += -fno-strict-aliasing
CPPFLAGS += -g

ifeq ($(MODE),release)
	CPPFLAGS += -O3
	CPPFLAGS += -DNDEBUG
endif

ifeq ($(ARCH),)
	ARCH = $(shell sh -c 'uname -m | sed -e "s/i.86/i386/;s/x86_64/x64/;s/amd64/x64/"')
endif

all: hogan.a

OBJS += src/hogan.o
OBJS += src/parser.o
OBJS += src/compiler.o

ifeq ($(ARCH),i386)
	ifeq ($(shell sh -c 'uname -s 2>/dev/null'),Darwin)
		CPPFLAGS += -arch i386
	else
		CPPFLAGS += -m32
	endif
	OBJS += src/ia32/assembler-ia32.o
	OBJS += src/ia32/codegen-ia32.o
else
	OBJS += src/x64/assembler-x64.o
	OBJS += src/x64/codegen-x64.o
endif

hogan.a: $(OBJS)
	$(AR) rcs hogan.a $(OBJS)

src/%.o: src/%.cc
	$(CXX) $(CPPFLAGS) -Isrc -c $< -o $@

TESTS += test/test-api
TESTS += test/bench-basic

test: $(TESTS)
	@test/test-api

test/%: test/%.cc hogan.a
	$(CXX) $(CPPFLAGS) $< -o $@ hogan.a

.PHONY: all test
