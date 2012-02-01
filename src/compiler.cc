#include "hogan.h"
#include "compiler.h"
#include "codegen.h"
#include "queue.h"

#include <assert.h> // assert
#include <stdlib.h> // NULL, abort
#include <string.h> // memset
#include <sys/mman.h> // mmap, munmap
#include <sys/types.h> // size_t
#include <stdint.h> // uint32_t

namespace hogan {

Template* Compiler::Compile(AstNode* ast,
                            Options* options,
                            const char* source) {
  Template* t = new Template();

  t->code->source = source;
  Codegen codegen(t->code, options);

  codegen.GeneratePrologue();
  codegen.GenerateBlock(ast);
  codegen.GenerateEpilogue();

  t->code->data = codegen.data;

  return t;
}


char* Template::Render(void* obj) {
  Queue<char*> out;
  size_t length = code->AsFunction()(obj, &out);
  if (length == 0) return NULL;

  char* result = new char[length + 1];
  result[length] = 0;

  char* chunk;
  off_t offset = 0;
  while ((chunk = out.Shift()) != NULL) {
    size_t size = strlen(chunk);
    memcpy(result + offset, chunk, size);
    offset += size;
  }

  return result;
};


Template::Template() {
  code = new TemplateCode();
}


Template::~Template() {
  delete code;
}


Code::Code(uint32_t size_) {
  size = size_;
  void* data = mmap(0,
                    size,
                    PROT_READ | PROT_WRITE | PROT_EXEC,
                    MAP_ANON | MAP_PRIVATE,
                    -1,
                    0);

  if (data == MAP_FAILED) abort();

  // Fill code with `nops`
  memset(data, 0x90, static_cast<size_t>(size));

  code = reinterpret_cast<char*>(data);
}


Code::~Code() {
  assert(munmap(code, static_cast<size_t>(size)) == 0);
  delete data;
}


} // namespace hogan
