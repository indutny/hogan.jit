#include "codegen.h"
#include "assembler-x64.h"
#include "assembler.h"
#include "parser.h" // AstNode
#include "queue.h" // Queue
#include "hogan.h" // ObjectTemplate

#include <assert.h> // assert
#include <string.h> // memcpy
#include <stdlib.h> // NULL

namespace hogan {

void Codegen::GeneratePrologue() {
  Push(rbp);
  Mov(rbp, rsp);
  Push(rbx);

  // Reserve space for 3 pointers
  SubImm(rsp, 24);

  MovToContext(-24, rsi); // store `out`
  MovToContext(-16, rdi); // store `obj`
  MovImm(rax, 0); // nullify return value
  MovToContext(-8, rax);
}


void Codegen::GenerateEpilogue() {
  MovFromContext(rax, -8);
  AddImm(rsp, 24);
  Pop(rbx);
  Leave();
  Return();
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


void Codegen::GenerateString(AstNode* node) {
  Queue<char*>::PushType method = &Queue<char*>::Push;

  char* value = new char[node->length + 1];
  memcpy(value, node->value, node->length);
  value[node->length] = 0;
  data->Push(value);

  MovFromContext(rdi, -24); // out
  MovImm(rsi, reinterpret_cast<const uint64_t>(value)); // push value
  Call(*reinterpret_cast<void**>(&method));

  AddImmToContext(-8, node->length);
}


typedef const char* (*GetObjectStrPropType)(ObjectTemplate*, const char*);
static const char* GetObjectStrProp(ObjectTemplate* obj, const char* prop) {
  return obj->GetString(prop);
}

typedef size_t (*StrLenType)(const char*);
static size_t strlen_wrap(const char* str) {
  return strlen(str);
}


void Codegen::GenerateProp(AstNode* node) {
  {
    GetObjectStrPropType method = &GetObjectStrProp;

    char* value = new char[node->length + 1];
    memcpy(value, node->value, node->length);
    value[node->length] = 0;
    data->Push(value);

    MovFromContext(rdi, -16); // obj
    MovImm(rsi, reinterpret_cast<const uint64_t>(value)); // get prop value
    Call(*reinterpret_cast<void**>(&method));
  }

  // Store pointer to value
  Push(rax);

  {
    Queue<char*>::PushType method = &Queue<char*>::Push;

    MovFromContext(rdi, -24); // out
    Mov(rsi, rax); // result of get prop
    Call(*reinterpret_cast<void**>(&method)); // push
  }

  // Restore it to calculate strlen
  Pop(rax);

  {
    StrLenType method = &strlen_wrap;

    Mov(rdi, rax); // str
    Call(*reinterpret_cast<void**>(&method)); // strlen

    AddToContext(-8, rax);
  }
}


typedef ObjectTemplate* (*GetObjectPropType)(ObjectTemplate*, const char*);
static ObjectTemplate* GetObjectProp(ObjectTemplate* obj, const char* prop) {
  return obj->GetObject(prop);
}


void Codegen::GenerateIf(AstNode* node) {
  AstNode* main_block = node->Shift();
  AstNode* else_block = node->Shift();

  MovFromContext(rdi, -16); // save obj
  Push(rdi);

  {
    GetObjectPropType method = &GetObjectProp;

    char* value = new char[node->length + 1];
    memcpy(value, node->value, node->length);
    value[node->length] = 0;
    data->Push(value);

    MovImm(rsi, reinterpret_cast<const uint64_t>(value)); // get prop value
    Call(*reinterpret_cast<void**>(&method));

    MovToContext(-16, rax); // Replace context var

    Label Start;
    Label Else;
    Label EndIf;

    Cmp(rax, NULL);
    Je(&Else);
    Bind(&Start);

    GenerateBlock(main_block);

    Jmp(&EndIf);

    Bind(&Else);

    if (else_block != NULL) GenerateBlock(else_block);

    Bind(&EndIf);
  }

  Pop(rdi);
  MovToContext(-16, rdi); // restore obj
}

} // namespace hogan
