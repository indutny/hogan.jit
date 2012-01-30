#include "hogan.h"
#include "compiler.h"
#include "parser.h"

#include <stdlib.h> // NULL
#include <string.h> // strlen

namespace hogan {

Hogan::Hogan() {
}

Hogan::~Hogan() {
}

Template* Hogan::Compile(const char* source_) {
  uint32_t len = strlen(source_);
  char* source = new char[len];
  memcpy(source, source_, len);

  Parser parser(source, len);
  parser.Parse();

  return Compiler::Compile(parser.Result(), source);
}

} // namespace hogan
