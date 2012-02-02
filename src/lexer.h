#ifndef _SRC_LEXER_H_
#define _SRC_LEXER_H_

#include <stdint.h> // uint32_t
#include <stdlib.h> // NULL

namespace hogan {

class Lexer {
 public:
  class Token {
   public:
    enum TokenType {
      kString,
      kProp,
      kRawProp,
      kIf,
      kElse,
      kEndIf,
      kPartial,
      kEnd
    };

    Token(TokenType type_) : type(type_), value(NULL), length(0) {
    }

    Token(TokenType type_, const void* value_, uint32_t length_) : type(type_) {
      value = value_;
      length = length_;
    }
    ~Token() {}

    TokenType type;
    const void* value;
    uint32_t length;
  };

  Lexer(const char* source_, uint32_t length_) : source(source_),
                                                 offset(0),
                                                 length(length_) {
  }
  ~Lexer() {}

  Token* Consume() {
    if (offset == length) return new Token(Token::kEnd);

    // Consume string
    if (source[offset] != '{' ||
        offset + 1 >= length ||
        source[offset + 1] != '{') {

      uint32_t start = offset++;
      while (offset < length) {
        if (offset + 1 < length &&
            source[offset] == '{' &&
            source[offset + 1] == '{') {
          break;
        }
        offset++;
      }
      return new Token(Token::kString, source + start, offset - start);
    }

    // Skip '{{'
    offset += 2;
    if (offset == length) return new Token(Token::kEnd);

    // Handle modificators: '#', '^', '/'
    Token::TokenType type;
    do {
      if (source[offset] == '#') {
        type = Token::kIf;
      } else if (source[offset] == '^') {
        type = Token::kElse;
      } else if (source[offset] == '/') {
        type = Token::kEndIf;
      } else if (source[offset] == '>') {
        type = Token::kPartial;
      } else if (source[offset] == '{') {
        type = Token::kRawProp;
      } else {
        type = Token::kProp;
        break;
      }
      offset++;
    } while(0);

    // Parse until '}}'
    uint32_t start = offset;
    while (offset + 2 < length &&
           (source[offset + 1] != '}' || source[offset + 2] != '}')) {
      offset++;
    }

    // '{{...' without '}}' or '{{}}'
    if (offset + 2 >= length) return new Token(Token::kEnd);
    offset++;

    // '}}}' for kRawProp
    if (type == Token::kRawProp &&
        (offset + 3 >= length || source[offset + 3] != '}')) {
      return new Token(Token::kEnd);
    }

    Token* prop = new Token(type, source + start, offset - start);

    if (type != Token::kRawProp) {
      // Skip '}}'
      offset += 2;
    } else {
      // Skip '}}}' for kRawProp
      offset += 3;
    }

    return prop;
  }

 private:
  const char* source;
  uint32_t offset;
  uint32_t length;
};

} // namespace hogan

#endif // _SRC_LEXER_H_
