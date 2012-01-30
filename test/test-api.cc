#include "test.h"

static const char* adjective = "neat";

class Object : public ObjectTemplate {
 public:
  Object() {}
  ~Object() {}

  const char* GetString(const char* key) {
    assert(strcmp(key, "adjective") == 0);

    return adjective;
  }

  Object* GetObject(const char* key) {
    if (strcmp(key, "prop") == 0) {
      return this;
    } else {
      return NULL;
    }
  }

  Object* At(const int index) {
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

TEST_START("API test")
  Template* t = hogan.Compile("some {{adjective}} template. "
                               "{{#prop}}yeah {{^prop}}oh noes{{/prop}}");

  Object data;

  char* out = t->Render(&data);
  assert(out != NULL);
  assert(strcmp("some neat template. yeah yeah yeah ", out) == 0);

  delete t;
  delete out;

  t = hogan.Compile("some {{adjective}} template. "
                     "{{#nprop}}yeah{{^nprop}}oh noes{{/nprop}}");

  out = t->Render(&data);
  assert(out != NULL);
  assert(strcmp("some neat template. oh noes", out) == 0);

  delete t;
  delete out;
TEST_END("API test")
