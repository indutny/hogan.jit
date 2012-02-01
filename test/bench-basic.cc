#include "test.h"

static const char* adjective = "fast";

class Object {
 public:
  static const void* GetString(void* obj, const char* key) {
    assert(strcmp(key, "adjective") == 0);

    return reinterpret_cast<const void*>(adjective);
  }
};

TEST_START("bench basic")
  Options options(Object::GetString,
                  NULL,
                  NULL,
                  NULL);
  Hogan hogan(&options);

  Object data;
  Template* t;
  char* out;

  t = hogan.Compile("some {{adjective}} template.");

  const int num = 5000000;

  BENCH_START(basic, num)
  for (int i = 0; i < num; i++) {
    out = t->Render(&data);
    delete out;
  }
  BENCH_END(basic, num)

  delete t;

TEST_END("bench basic")
