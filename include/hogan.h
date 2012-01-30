#ifndef _HOGAN_H_
#define _HOGAN_H_

namespace hogan {

class Template;
class TemplateCode;
class ObjectTemplate;
class Compiler;

class Hogan {
 public:
  Hogan();
  ~Hogan();

  Template* Compile(const char* source);
};

class Template {
 public:
  Template();
  ~Template();

  char* Render(ObjectTemplate* obj);

 private:
  TemplateCode* code;
  friend class Compiler;
};

class ObjectTemplate {
 public:
  virtual const char* GetString(const char* key) = 0;
  virtual ObjectTemplate* GetObject(const char* key) = 0;
  virtual ObjectTemplate* At(const int index) = 0;
  virtual bool IsArray() = 0;
};

} // namespace hogan

#endif // _HOGAN_H_
