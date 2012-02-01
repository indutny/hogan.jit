#ifndef _HOGAN_H_
#define _HOGAN_H_

namespace hogan {

class Template;
class TemplateCode;
class Compiler;
class Options;

class Hogan {
 public:
  Hogan(Options* options);
  ~Hogan();

  Template* Compile(const char* source);
 private:
  Options* options_;
};

typedef const void* (*PropertyCallback)(void* obj, const char* key);
typedef const void* (*NumericPropertyCallback)(void* obj, const int index);
typedef bool (*IsArrayCallback)(void* obj);

class Options {
 public:
  Options(PropertyCallback getString_,
          PropertyCallback getObject_,
          NumericPropertyCallback at_,
          IsArrayCallback isArray_) {
    getString = getString_;
    getObject = getObject_;
    at = at_;
    isArray = isArray_;
  }

  ~Options() {
  }

  PropertyCallback getString;
  PropertyCallback getObject;
  NumericPropertyCallback at;
  IsArrayCallback isArray;
};


class Template {
 public:
  Template();
  ~Template();

  char* Render(void* obj);

 private:
  TemplateCode* code;
  friend class Compiler;
};

} // namespace hogan

#endif // _HOGAN_H_
