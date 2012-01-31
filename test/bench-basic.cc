#include "test.h"

static const char* adjective = "fast";

class Object : public ObjectTemplate {
  public:
   const char* GetString(const char* key) {
     return adjective;
   }

   ObjectTemplate* GetObject(const char* key) { return NULL; }
   Object* At(const int index) { return NULL; }
   bool IsArray() { return false; }
};

TEST_START("bench basic")

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
