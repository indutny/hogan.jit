#include "codegen.h"
#include "assembler-ia32.h"
#include "assembler.h"
#include "parser.h" // AstNode
#include "output.h" // TemplateOutput
#include "hogan.h" // Options

#include <assert.h> // assert
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


void Codegen::GenerateEpilogue() {
  MovFromContext(eax, -4);
  Mov(esp, ebp);
  Pop(ebp);
  Return(0);
}


typedef void (TemplateOutput::*PushCallback)(const char*, const size_t);


void Codegen::GenerateString(AstNode* node) {
  PushCallback method = &TemplateOutput::Push;

  char* value = ToData(node);

  int align = PreCall(0, 3);

  MovFromContext(eax, -12);
  PushImm(node->length); // length
  PushImm(reinterpret_cast<const uint64_t>(value)); // str to push
  Push(eax); // out
  Call(*reinterpret_cast<void**>(&method));

  AddImm(esp, align);

  AddImmToContext(-4, node->length);
}


typedef size_t (*StrLenType)(const char*);


void Codegen::GenerateProp(AstNode* node) {
  {
    PropertyCallback method = options->getString;

    char* value = ToData(node);

    int align = PreCall(0, 2);

    MovFromContext(eax, -8);
    PushImm(reinterpret_cast<const uint64_t>(value)); // str to push
    Push(eax); // obj
    Call(*reinterpret_cast<void**>(&method));

    // Unwind stack and unalign
    AddImm(esp, align);
  }

  {
    PushCallback method = &TemplateOutput::Push;

    int align = PreCall(4, 3);

    PushImm(0); // length
    Push(eax); // value
    MovFromContext(eax, -12);
    Push(eax); // out
    Call(*reinterpret_cast<void**>(&method)); // push

    AddImm(esp, align); // unalign stack
  }
}


void Codegen::GenerateIf(AstNode* node) {
  AstNode* main_block = node->Shift();
  AstNode* else_block = node->Shift();

  MovFromContext(eax, -8); // save obj
  Push(eax);

  {
    PropertyCallback method = options->getObject;

    char* value = ToData(node);

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
    IsArrayCallback method = options->isArray;

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
    NumericPropertyCallback method = options->at;

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


void Codegen::GeneratePartial(AstNode* node) {
  // Get partial
  {
    PartialCallback method = options->getPartial;

    char* value = ToData(node);

    int delta = PreCall(0, 1);

    PushImm(reinterpret_cast<const uint64_t>(value)); // partial name
    Call(*reinterpret_cast<void**>(&method));

    AddImm(esp, delta);
  }

  // Invoke partial
  {
    InvokePartialType method = InvokePartial;
    int delta = PreCall(0, 3);

    MovFromContext(ecx, -12); // out
    Push(ecx);
    MovFromContext(ecx, -8); // obj
    Push(ecx);
    Push(eax); // partial
    Call(*reinterpret_cast<void**>(&method));

    AddImm(esp, delta);
  }
}

} // namespace hogan
