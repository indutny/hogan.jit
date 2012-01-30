CPPFLAGS += -Wall -Wextra -Wno-unused-parameter
CPPFLAGS += -Iinclude
CPPFLAGS += -g

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

test: hogan.a $(TESTS)
	@test/test-api

test/%: test/%.cc
	$(CXX) $(CPPFLAGS) hogan.a $< -o $@
