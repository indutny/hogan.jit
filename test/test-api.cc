#include "test.h"

static const char* adjective = "neat";

class ArrayObject : public ObjectTemplate {
 public:
  const char* GetString(const char* key) { return NULL; }
  ArrayObject* GetObject(const char* key) { return NULL; }

  ArrayObject* At(const int index) {
    if (index < 3) {
      return this;
    } else {
      return NULL;
    }
  }

  bool IsArray() {
    return true;
  }
};

class Object : public ObjectTemplate {
 public:
  const char* GetString(const char* key) {
    assert(strcmp(key, "adjective") == 0);

    return adjective;
  }

  ObjectTemplate* GetObject(const char* key) {
    if (strcmp(key, "prop") == 0) {
      return this;
    } if (strcmp(key, "arrprop") == 0) {
      return &array;
    } else {
      return NULL;
    }
  }

  Object* At(const int index) {
    return NULL;
  }

  bool IsArray() {
    return false;
  }

  ArrayObject array;
};

TEST_START("API test")
  Object data;
  Template* t;
  char* out;

  t = hogan.Compile("some {{adjective}} template. "
                    "{{#prop}}yeah{{^prop}}oh noes{{/prop}}");

  out = t->Render(&data);
  assert(out != NULL);
  assert(strcmp("some neat template. yeah", out) == 0);

  delete t;
  delete out;

  t = hogan.Compile("some {{adjective}} template."
                    "{{#arrprop}} o{{^arrprop}}oh noes{{/arrprop}}");

  out = t->Render(&data);
  assert(out != NULL);
  assert(strcmp("some neat template. o o o", out) == 0);

  delete t;
  delete out;

  t = hogan.Compile("some {{adjective}} template. "
                     "{{#nprop}}yeah{{^nprop}}oh noes{{/nprop}}");

  out = t->Render(&data);
  assert(out != NULL);
  assert(strcmp("some neat template. oh noes", out) == 0);

  delete out;
  delete t;
TEST_END("API test")
