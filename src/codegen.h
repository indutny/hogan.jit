#ifndef _SRC_CODEGEN_H_
#define _SRC_CODEGEN_H_

#include "hogan.h"
#include "compiler.h"
#include "assembler.h"
#include "parser.h"

#include <string.h> // memcpy

namespace hogan {

class Code;

class Codegen : public Assembler {
 public:
  Codegen(Code* code_, Options* options_) : Assembler(code_),
                                            options(options_) {
    data = new Queue<char*>();
  }

  void GeneratePrologue();
  void GenerateEpilogue();

  void GenerateBlock(AstNode* node);
  void GenerateString(AstNode* node);
  void GenerateProp(AstNode* node);
  void GenerateIf(AstNode* node);
  void GeneratePartial(AstNode* node);

  typedef void (*InvokePartialType)(Template*, void*, TemplateOutput*);
  static void InvokePartial(Template* t, void* obj, TemplateOutput* out) {
    t->code->AsFunction()(obj, out);
  }

  inline char* ToData(AstNode* node) {
    char* value = new char[node->length + 1];
    memcpy(value, node->value, node->length);
    value[node->length] = 0;
    data->Push(value);

    return value;
  }

  Queue<char*>* data;
  Options* options;
};

} // namespace hogan

#endif // _SRC_CODEGEN_H_
