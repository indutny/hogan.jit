#include "parser.h"

#include <assert.h> // assert

namespace hogan {

void Parser::Parse() {
  Lexer::Token* tok = Consume();

  while (tok->type != Lexer::Token::kEnd) {
    switch (tok->type) {
     case Lexer::Token::kString:
      Enter(AstNode::kString);
      SetValue(tok);
      Leave(AstNode::kString);
      break;
     case Lexer::Token::kProp:
      Enter(AstNode::kProp);
      SetValue(tok);
      Leave(AstNode::kProp);
      break;
     case Lexer::Token::kIf:
      Enter(AstNode::kIf);
      SetValue(tok);
      Enter(AstNode::kBlock);
      break;
     case Lexer::Token::kElse:
      Leave(AstNode::kBlock);
      Enter(AstNode::kBlock);
      SetValue(tok);
      break;
     case Lexer::Token::kEndIf:
      Leave(AstNode::kBlock);
      Leave(AstNode::kIf);
      break;
     default:
      assert(false && "Unexpected");
    }
    delete tok;
    tok = Consume();
  }

  // Delete kEnd token too
  delete tok;
}

} // namespace hogan
