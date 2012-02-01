#include "hogan.h"
#include "compiler.h"
#include "parser.h"

#include <stdlib.h> // NULL
#include <string.h> // strlen

namespace hogan {

Hogan::Hogan(Options* options) {
  options_ = new Options(*options);
}

Hogan::~Hogan() {
  delete options_;
}

Template* Hogan::Compile(const char* source) {
  uint32_t len = strlen(source);
  char* source_ = new char[len];
  memcpy(source_, source, len);

  Parser parser(source_, len);
  parser.Parse();

  return Compiler::Compile(parser.Result(), options_, source_);
}

} // namespace hogan
