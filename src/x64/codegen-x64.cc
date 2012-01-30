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


typedef const char* (*GetStrPropType)(ObjectTemplate*, const char*);
static const char* GetStrProp(ObjectTemplate* obj, const char* prop) {
  return obj->GetString(prop);
}

typedef size_t (*StrLenType)(const char*);
static size_t strlen_wrap(const char* str) {
  return strlen(str);
}


void Codegen::GenerateProp(AstNode* node) {
  {
    GetStrPropType method = &GetStrProp;

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


typedef ObjectTemplate* (*GetPropType)(ObjectTemplate*, const char*);
static ObjectTemplate* GetProp(ObjectTemplate* obj, const char* prop) {
  return obj->GetObject(prop);
}

typedef ObjectTemplate* (*GetAtType)(ObjectTemplate*, const int);
static ObjectTemplate* GetAt(ObjectTemplate* obj, const int index) {
  return obj->At(index);
}

typedef bool (*IsArrayType)(ObjectTemplate* obj);
static bool IsArray(ObjectTemplate* obj) {
  return obj->IsArray();
}


void Codegen::GenerateIf(AstNode* node) {
  AstNode* main_block = node->Shift();
  AstNode* else_block = node->Shift();

  MovFromContext(rdi, -16); // save obj
  Push(rdi);

  {
    GetPropType method = &GetProp;

    char* value = new char[node->length + 1];
    memcpy(value, node->value, node->length);
    value[node->length] = 0;
    data->Push(value);

    MovImm(rsi, reinterpret_cast<const uint64_t>(value)); // get prop value
    Call(*reinterpret_cast<void**>(&method));

    MovToContext(-16, rax); // Replace context var
  }

  Label Start, Else, EndIf;

  // Check if object has that prop
  Cmp(rax, NULL);
  Je(&Else);

  // Push property (needed to restore after iteration loop)
  Push(rax);

  // Check if we need to iterate props
  {
    IsArrayType method = &IsArray;

    Mov(rdi, rax);
    Call(*reinterpret_cast<void**>(&method));
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
    GetAtType method = &GetAt;

    Pop(rax);
    Pop(rdi);

    Mov(rsi, rax);
    Inc(rax);

    Push(rdi);
    Push(rax);

    Call(*reinterpret_cast<void**>(&method));

    // If At() returns NULL - we reached end of array
    Cmp(rax, 0);
    Je(&EndIterate);

    // Replace context var
    MovToContext(-16, rax);
  }

  Bind(&Start);

  GenerateBlock(main_block);

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
