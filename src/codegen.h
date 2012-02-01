#ifndef _SRC_CODEGEN_H_
#define _SRC_CODEGEN_H_

#include "hogan.h"
#include "compiler.h"
#include "assembler.h"
#include "parser.h"

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

  Queue<char*>* data;
  Options* options;
};

} // namespace hogan

#endif // _SRC_CODEGEN_H_
