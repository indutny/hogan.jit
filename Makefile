CPPFLAGS += -Wall -Wextra -Wno-unused-parameter
CPPFLAGS += -fPIC -Iinclude
CPPFLAGS += -g

ifeq ($(MODE),release)
	CPPFLAGS += -O3
	CPPFLAGS += -DNDEBUG
endif


all: hogan.a

OBJS += src/hogan.o
OBJS += src/parser.o
OBJS += src/compiler.o

ifeq ($(ARCH),i386)
	CPPFLAGS += -arch i386
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

test: $(TESTS)
	@test/test-api

test/%: test/%.cc hogan.a
	$(CXX) $(CPPFLAGS) $< -o $@ hogan.a

.PHONY: all test
