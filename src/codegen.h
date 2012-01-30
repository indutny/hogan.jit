#ifndef _SRC_CODEGEN_H_
#define _SRC_CODEGEN_H_

#include "compiler.h"
#include "assembler.h"
#include "parser.h"

namespace hogan {

class Code;

class Codegen : public Assembler {
 public:
  Codegen(Code* code_) : Assembler(code_) {
    data = new Queue<char*>();
  }

  void GeneratePrologue();
  void GenerateEpilogue();

  void GenerateBlock(AstNode* node);
  void GenerateString(AstNode* node);
  void GenerateProp(AstNode* node);
  void GenerateIf(AstNode* node);

  Queue<char*>* data;
};

} // namespace hogan

#endif // _SRC_CODEGEN_H_
