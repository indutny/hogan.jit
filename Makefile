CPPFLAGS += -Wall -Wextra -Wno-unused-parameter
CPPFLAGS += -fPIC -Iinclude

ifeq ($(MODE),release)
	CPPFLAGS += -O3
	CPPFLAGS += -DNDEBUG
else
	CPPFLAGS += -g
endif


all: hogan.a

OBJS += src/hogan.o
OBJS += src/parser.o
OBJS += src/x64/assembler-x64.o
OBJS += src/x64/codegen-x64.o
OBJS += src/compiler.o

hogan.a: $(OBJS)
	$(AR) rcs hogan.a $(OBJS)

src/%.o: src/%.cc
	$(CXX) $(CPPFLAGS) -Isrc -c $< -o $@

TESTS += test/test-api

test: $(TESTS)
	@test/test-api

test/%: test/%.cc hogan.a
	$(CXX) $(CPPFLAGS) hogan.a $< -o $@

.PHONY: all test
