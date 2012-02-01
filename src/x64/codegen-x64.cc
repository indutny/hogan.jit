#include "codegen.h"
#include "assembler-x64.h"
#include "assembler.h"
#include "parser.h" // AstNode
#include "output.h" // TemplateOutput
#include "hogan.h" // Options

#include <assert.h> // assert
#include <string.h> // memcpy
#include <stdlib.h> // NULL

namespace hogan {

void Codegen::GeneratePrologue() {
  Push(rbp);
  Mov(rbp, rsp);

  // Reserve space for 3 pointers
  // and align stack
  SubImm(rsp, 24 + 8);

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
      GenerateProp(descendant);
      break;
     case AstNode::kIf:
      GenerateIf(descendant);
      break;
     default:
      assert(false && "Unexpected");
    }

    delete descendant;
  }

  delete node;
}


typedef void (TemplateOutput::*PushCallback)(const char*, const size_t);


void Codegen::GenerateString(AstNode* node) {
  PushCallback method = &TemplateOutput::Push;

  char* value = new char[node->length + 1];
  memcpy(value, node->value, node->length);
  value[node->length] = 0;
  data->Push(value);

  int delta = PreCall(0, 3);

  MovFromContext(rdi, -24); // out
  MovImm(rsi, reinterpret_cast<const uint64_t>(value)); // value
  MovImm(rdx, node->length); // length
  Call(*reinterpret_cast<void**>(&method));

  AddImm(rsp, delta);
}


void Codegen::GenerateProp(AstNode* node) {
  {
    PropertyCallback method = options->getString;

    char* value = new char[node->length + 1];
    memcpy(value, node->value, node->length);
    value[node->length] = 0;
    data->Push(value);

    int delta = PreCall(0, 2);

    MovFromContext(rdi, -16); // obj
    MovImm(rsi, reinterpret_cast<const uint64_t>(value)); // get prop value
    Call(*reinterpret_cast<void**>(&method));

    AddImm(rsp, delta);
  }

  {
    PushCallback method = &TemplateOutput::Push;

    int delta = PreCall(8, 3);

    MovFromContext(rdi, -24); // out
    Mov(rsi, rax); // result of get prop
    MovImm(rdx, 0); // let output stream determine size
    Call(*reinterpret_cast<void**>(&method)); // push

    AddImm(rsp, delta);
  }
}


void Codegen::GenerateIf(AstNode* node) {
  AstNode* main_block = node->Shift();
  AstNode* else_block = node->Shift();

  MovFromContext(rdi, -16); // save obj
  Push(rdi);

  {
    PropertyCallback method = options->getObject;

    char* value = new char[node->length + 1];
    memcpy(value, node->value, node->length);
    value[node->length] = 0;
    data->Push(value);

    int delta = PreCall(8, 2);

    MovImm(rsi, reinterpret_cast<const uint64_t>(value)); // get prop value
    Call(*reinterpret_cast<void**>(&method));

    AddImm(rsp, delta);

    MovToContext(-16, rax); // Replace context var
  }

  Label Start, Else, EndIf;

  // Check if object has that prop
  Cmp(rax, 0);
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

  PushImm(0);

  // If not array - skip to if's body
  Cmp(rax, 0);
  Je(&Start);

  // Start of loop
  Label Iterate, EndIterate;
  Bind(&Iterate);

  // Get item at index
  {
    NumericPropertyCallback method = options->at;

    Pop(rax);
    Pop(rdi);

    Mov(rsi, rax);
    Inc(rax);

    Push(rdi);
    Push(rax);

    int delta = PreCall(24, 2);

    Call(*reinterpret_cast<void**>(&method));

    AddImm(rsp, delta);

    // If At() returns NULL - we reached end of array
    Cmp(rax, 0);
    Je(&EndIterate);

    // Replace context var
    MovToContext(-16, rax);
  }

  Bind(&Start);

  int delta = PreCall(24, 0);

  GenerateBlock(main_block);

  AddImm(rsp, delta);

  Pop(rax);
  Pop(rdi);

  Cmp(rax, 0);
  Je(&EndIf);

  // Store parent and loop index
  Push(rdi);
  Push(rax);

  // And continue iterating
  Jmp(&Iterate);

  Bind(&Else);

  if (else_block != NULL) GenerateBlock(else_block);
  Jmp(&EndIf);

  Bind(&EndIterate);

  Pop(rax);
  Pop(rdi);

  Bind(&EndIf);

  Pop(rdi);
  MovToContext(-16, rdi); // restore obj
}

} // namespace hogan
