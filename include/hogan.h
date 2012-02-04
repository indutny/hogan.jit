#ifndef _HOGAN_H_
#define _HOGAN_H_

namespace hogan {

//
// A lot of typedefs and classes here
//

class Template;
class TemplateCode;
class Compiler;
class Codegen;
class Options;

typedef const void* (*PropertyCallback)(void* obj, const char* key);
typedef const void* (*NumericPropertyCallback)(void* obj, const int index);
typedef int (*IsArrayCallback)(void* obj);
typedef Template* (*PartialCallback)(Template* tpl, const char* name);

//
// Actual API
//

class Hogan {
 public:
  Hogan(Options* options);
  ~Hogan();

  Template* Compile(const char* source);

 private:
  Options* options_;
};


class Options {
 public:
  Options();

  PropertyCallback getString;
  PropertyCallback getObject;

  IsArrayCallback isArray;
  IsArrayCallback arrayLength;
  NumericPropertyCallback at;

  PartialCallback getPartial;
};


class Template {
 public:
  Template();
  ~Template();

  char* Render(void* obj);

 private:
  TemplateCode* code;
  friend class Compiler;
  friend class Codegen;
};

} // namespace hogan

#endif // _HOGAN_H_
