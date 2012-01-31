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

  BENCH_START(basic, 5000000)
  for (int i = 0; i < 5000000; i++) {
    out = t->Render(&data);
    delete out;
  }
  BENCH_END(basic, 5000000)

  delete t;

TEST_END("bench basic")
