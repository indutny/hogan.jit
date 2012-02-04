#include "test.h"

class Object {
 public:
  static const void* GetObject(void* obj, const char* key) {
    return obj;
  }

  static const void* At(void* obj, const int index) {
    return obj;
  }

  static int IsArray(void* obj) {
    return true;
  }

  static int ArrayLength(void* obj) {
    return 10000;
  }
};

TEST_START("Template output reallocation")
  Options options;

  options.getObject = Object::GetObject;
  options.at = Object::At;
  options.isArray = Object::IsArray;
  options.arrayLength = Object::ArrayLength;

  Hogan hogan(&options);

  Object data;
  Template* t;
  char* out;

  t = hogan.Compile("{{#iterate}} string {{/iterate}}");

  out = t->Render(&data);
  assert(out != NULL);
  assert(strlen(out) == 8 * 10000);

  delete t;
  delete out;

TEST_END("Template output reallocation")
