#include "codegen.h"
#include "assembler-x64.h"
#include "assembler.h"
#include "parser.h" // AstNode
#include "output.h" // TemplateOutput
#include "hogan.h" // Options

#include <assert.h> // assert
#include <stdlib.h> // NULL

namespace hogan {

void Codegen::GeneratePrologue() {
  Push(rbp);
  Mov(rbp, rsp);

  // Reserve space for 4 pointers
  // and align stack
  SubImm(rsp, 32);

  MovToContext(-32, rcx); // store `template`
  MovToContext(-24, rsi); // store `out`
  MovToContext(-16, rdi); // store `obj`
  Xor(rax, rax); // nullify return value
  MovToContext(-8, rax);
}


void Codegen::GenerateEpilogue() {
  MovFromContext(rax, -8);
  Mov(rsp, rbp);
  Pop(rbp);
  Return(0);
}


void Codegen::GenerateBlock(AstNode* node) {
  AstNode* descendant;

  while ((descendant = node->Shift()) != NULL) {
    switch (descendant->type) {
     case AstNode::kString:
      GenerateString(descendant);
      break;
     case AstNode::kProp:
      GenerateProp(descendant, true);
      break;
     case AstNode::kRawProp:
      GenerateProp(descendant, false);
      break;
     case AstNode::kIf:
      GenerateIf(descendant);
      break;
     case AstNode::kPartial:
      GeneratePartial(descendant);
      break;
     default:
      assert(false && "Unexpected");
    }

    delete descendant;
  }

  delete node;
}


void Codegen::GenerateString(AstNode* node) {
  PushCallback method = &TemplateOutput::Push;

  char* value = ToData(node);

  int delta = PreCall(0, 4);

  MovFromContext(rdi, -24); // out
  MovImm(rsi, reinterpret_cast<const uint64_t>(value)); // value
  MovImm(rdx, node->length); // length
  MovImm(rcx, TemplateOutput::kNone); // flags
  Call(*reinterpret_cast<void**>(&method));

  AddImm(rsp, delta);
}


void Codegen::GenerateProp(AstNode* node, bool escape) {
  {
    PropertyCallback method = options->getString;

    char* value = ToData(node);

    int delta = PreCall(0, 2);

    MovFromContext(rdi, -16); // obj
    MovImm(rsi, reinterpret_cast<const uint64_t>(value)); // get prop value
    Call(*reinterpret_cast<void**>(&method));

    AddImm(rsp, delta);
  }

  Label skipPush;
  CmpImm(rax, 0);
  Je(&skipPush);

  {
    PushCallback method = &TemplateOutput::Push;

    int delta = PreCall(8, 4);

    MovFromContext(rdi, -24); // out
    Mov(rsi, rax); // result of get prop
    MovImm(rdx, 0); // let output stream determine size
    MovImm(rcx, (escape ? TemplateOutput::kEscape : TemplateOutput::kNone) |
                TemplateOutput::kAllocated);
    Call(*reinterpret_cast<void**>(&method)); // push

    AddImm(rsp, delta);
  }

  Bind(&skipPush);
}


void Codegen::GenerateIf(AstNode* node) {
  AstNode* main_block = node->Shift();
  AstNode* else_block = node->Shift();

  MovFromContext(rdi, -16); // save obj
  Push(rdi);

  {
    PropertyCallback method = options->getObject;

    char* value = ToData(node);

    int delta = PreCall(8, 2);

    MovImm(rsi, reinterpret_cast<const uint64_t>(value)); // get prop value
    Call(*reinterpret_cast<void**>(&method));

    AddImm(rsp, delta);

    MovToContext(-16, rax); // Replace context var
  }

  Label Start, Else, EndIf;

  // Check if object has that prop
  CmpImm(rax, 0);
  Je(&Else);

  // Push property (needed to restore after iteration loop)
  Push(rax);

  // Check if we need to iterate props
  {
    IsArrayCallback method = options->isArray;

    int delta = PreCall(16, 1);

    Mov(rdi, rax);
    Call(*reinterpret_cast<void**>(&method));

    AddImm(rsp, delta);
  }

  // Restore obj pointer
  MovFromStack(rdi, 0);

  // Index & Length (0 at start)
  PushImm(0);
  PushImm(0);

  // If not array - skip to if's body
  CmpImm(rax, 0);
  Je(&Start);

  // Get true array length
  {
    IsArrayCallback method = options->arrayLength;

    int delta = PreCall(16, 1);
    // rdi is already a pointer to array
    Call(*reinterpret_cast<void**>(&method));
    AddImm(rsp, delta);
  }

  // Replace the one on stack
  MovToStack(0, rax);

  // Start of loop
  Label Iterate, EndIterate;
  Bind(&Iterate);

  // Get item at index
  {
    NumericPropertyCallback method = options->at;

    MovFromStack(rsi, 8);
    MovFromStack(rdi, 16);
    IncStack(8);

    int delta = PreCall(24, 2);

    Call(*reinterpret_cast<void**>(&method));

    AddImm(rsp, delta);

    // Replace context var
    MovToContext(-16, rax);
  }

  Bind(&Start);

  int delta = PreCall(24, 0);

  GenerateBlock(main_block);

  AddImm(rsp, delta);

  MovFromStack(rcx, 0);
  MovFromStack(rax, 8);

  Cmp(rax, rcx);
  Je(&EndIterate);

  // And continue iterating
  Jmp(&Iterate);

  Bind(&EndIterate);

  // Cleanup loop data
  AddImm(rsp, 24);

  Jmp(&EndIf);

  Bind(&Else);

  if (else_block != NULL) GenerateBlock(else_block);
  Bind(&EndIf);

  Pop(rdi);
  MovToContext(-16, rdi); // restore obj
}


void Codegen::GeneratePartial(AstNode* node) {
  // Get partial
  {
    PartialCallback method = options->getPartial;

    char* value = ToData(node);

    int delta = PreCall(0, 1);

    MovFromContext(rdi, -24); // template
    MovImm(rsi, reinterpret_cast<const uint64_t>(value)); // partial name
    Call(*reinterpret_cast<void**>(&method));

    AddImm(rsp, delta);
  }

  Label skipPush;
  CmpImm(rax, 0);
  Je(&skipPush);

  // Invoke partial
  {
    InvokePartialType method = InvokePartial;
    int delta = PreCall(0, 3);

    Mov(rdi, rax);
    MovFromContext(rsi, -16); // obj
    MovFromContext(rdx, -24); // out
    Call(*reinterpret_cast<void**>(&method));

    AddImm(rsp, delta);
  }

  Bind(&skipPush);
}

} // namespace hogan
