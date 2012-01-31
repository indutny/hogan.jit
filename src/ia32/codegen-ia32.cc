#include "codegen.h"
#include "assembler-ia32.h"
#include "assembler.h"
#include "parser.h" // AstNode
#include "queue.h" // Queue
#include "hogan.h" // ObjectTemplate

#include <assert.h> // assert
#include <string.h> // memcpy
#include <stdlib.h> // NULL

namespace hogan {

void Codegen::GeneratePrologue() {
  Push(ebp);
  Mov(ebp, esp);

  // Reserve space for 3 pointers
  SubImm(esp, 12 + 12);

  MovFromContext(eax, 8); // get `obj`
  MovToContext(-8, eax); // store `obj`
  MovFromContext(eax, 12); // get `out`
  MovToContext(-12, eax); // store `out`

  Xor(eax, eax); // nullify return value
  MovToContext(-4, eax);
}


void Codegen::GenerateEpilogue() {
  MovFromContext(eax, -4);
  Mov(esp, ebp);
  Pop(ebp);
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


void Codegen::GenerateString(AstNode* node) {
  Queue<char*>::PushType method = &Queue<char*>::Push;

  char* value = new char[node->length + 1];
  memcpy(value, node->value, node->length);
  value[node->length] = 0;
  data->Push(value);

  int align = PreCall(0, 2);

  MovFromContext(eax, -12);
  PushImm(reinterpret_cast<const uint64_t>(value)); // str to push
  Push(eax); // out
  Call(*reinterpret_cast<void**>(&method));

  AddImm(esp, align);

  AddImmToContext(-4, node->length);
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

    int align = PreCall(0, 2);

    MovFromContext(eax, -8);
    PushImm(reinterpret_cast<const uint64_t>(value)); // str to push
    Push(eax); // obj
    Call(*reinterpret_cast<void**>(&method));

    // Unwind stack and unalign
    AddImm(esp, align);
  }

  // Store pointer to value
  Push(eax); // value

  {
    Queue<char*>::PushType method = &Queue<char*>::Push;

    int align = PreCall(4, 2);

    Push(eax); // value
    MovFromContext(eax, -12);
    Push(eax); // out
    Call(*reinterpret_cast<void**>(&method)); // push

    AddImm(esp, align); // unalign stack
  }

  Pop(eax);

  {
    StrLenType method = &strlen_wrap;

    int align = PreCall(0, 1);

    Push(eax); // value
    Call(*reinterpret_cast<void**>(&method)); // strlen

    AddImm(esp, align); // unalign stack

    AddToContext(-4, eax);
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

  MovFromContext(eax, -8); // save obj
  Push(eax);

  {
    GetPropType method = &GetProp;

    char* value = new char[node->length + 1];
    memcpy(value, node->value, node->length);
    value[node->length] = 0;
    data->Push(value);

    int delta = PreCall(4, 2);

    PushImm(reinterpret_cast<const uint64_t>(value)); // prop value
    Push(eax); // obj
    Call(*reinterpret_cast<void**>(&method));

    // Unalign stack
    AddImm(esp, delta);

    MovToContext(-8, eax); // Replace context var
  }

  Label Start, Else, EndIf;

  // Check if object has that prop
  Cmp(eax, 0);
  Je(&Else);

  // Push property (needed to restore after iteration loop)
  Push(eax);

  // Check if we need to iterate props
  {
    IsArrayType method = &IsArray;

    int delta = PreCall(8, 1);

    Push(eax); // prop value
    Call(*reinterpret_cast<void**>(&method));

    // Unalign stack
    AddImm(esp, delta);
  }

  PushImm(0);

  // If not array - skip to if's body
  Cmp(eax, 0);
  Je(&Start);

  // Start of loop
  Label Iterate, EndIterate;
  Bind(&Iterate);

  // Get item at index
  {
    GetAtType method = &GetAt;

    Pop(eax);
    Pop(ecx);

    Mov(edx, eax);
    Inc(eax);

    Push(ecx);
    Push(eax);

    int delta = PreCall(12, 2);

    Push(edx); // index
    Push(ecx); // obj
    Call(*reinterpret_cast<void**>(&method));

    AddImm(esp, delta);

    // If At() returns NULL - we reached end of array
    Cmp(eax, 0);
    Je(&EndIterate);

    // Replace context var
    MovToContext(-8, eax);
  }

  Bind(&Start);

  int delta = PreCall(12, 0);
  GenerateBlock(main_block);
  AddImm(esp, delta);

  Pop(eax);
  Pop(ecx);

  Cmp(eax, 0);
  Je(&EndIf);

  // Store parent and loop index
  Push(ecx);
  Push(eax);

  // And continue iterating
  Jmp(&Iterate);

  Bind(&Else);

  if (else_block != NULL) GenerateBlock(else_block);
  Jmp(&EndIf);

  Bind(&EndIterate);

  Pop(eax);
  Pop(ecx);

  Bind(&EndIf);

  Pop(eax);
  MovToContext(-8, eax); // restore obj
}

} // namespace hogan