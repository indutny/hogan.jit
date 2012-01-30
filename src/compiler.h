#ifndef _SRC_ASM_H_
#define _SRC_ASM_H_

#include "parser.h"

#include <stdint.h> // uint32_t
#include <sys/types.h> // size_t

namespace hogan {

class Code;
class Template;
class TemplateCode;
class ObjectTemplate;

typedef size_t (*TemplateFunction)(ObjectTemplate* obj,
                                   Queue<char*>* out);

class Compiler {
 public:
  static Template* Compile(AstNode* ast, const char* code);
};

class Code {
 public:
  Code(uint32_t size_);
  ~Code();

  uint32_t size;
  char* code;
  Queue<char*>* data;
};

class TemplateCode : public Code {
 public:
  TemplateCode() : Code(1024) {
  }

  ~TemplateCode() {
    delete source;
  }

  inline TemplateFunction AsFunction() {
    return reinterpret_cast<TemplateFunction>(this->code);
  }

  // Just for reference
  const char* source;
};


} // namespace hogan

#endif // _SRC_ASM_H_
