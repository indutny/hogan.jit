#ifndef _SRC_ASM_H_
#define _SRC_ASM_H_

#include "hogan.h"
#include "parser.h"
#include "queue.h" // Queue
#include "output.h" // TemplateOutput

#include <stdint.h> // uint32_t
#include <sys/types.h> // size_t

namespace hogan {

class Code;
class Template;
class TemplateCode;

typedef size_t (*TemplateFunction)(void* obj,
                                   TemplateOutput* out);

class Compiler {
 public:
  static Template* Compile(AstNode* ast, Options* options, const char* source);
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
