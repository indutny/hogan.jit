#ifndef _SRC_ASSEMBLER_H_
#define _SRC_ASSEMBLER_H_

#include "compiler.h"
#include "stdint.h" // uintXX_t

namespace hogan {

class Assembler {
 public:
  Assembler(Code* code_) : code(code_), offset(0) {
  }

  class Label {
   public:
    struct Pos {
      int32_t offset;
      uint8_t size;
    };

    Label() : offset(-1) {
    }

    int32_t offset;
    Queue<Pos*> queue;
  };

  inline void Immediate(uint64_t imm) {
    *reinterpret_cast<uint64_t*>(code->code + offset) = imm;
    offset += sizeof(imm);
  }


  inline void Immediate(const uint32_t imm) {
    *reinterpret_cast<uint32_t*>(code->code + offset) = imm;
    offset += sizeof(imm);
  }


  inline void Immediate(const uint16_t imm) {
    *reinterpret_cast<uint16_t*>(code->code + offset) = imm;
    offset += sizeof(imm);
  }


  inline void Immediate(const uint8_t imm) {
    *reinterpret_cast<uint8_t*>(code->code + offset) = imm;
    offset += sizeof(imm);
  }

  inline void Immediate(const uint32_t imm, const uint8_t size) {
    if (size == 1) {
      Immediate(static_cast<const uint8_t>(imm));
    } else if (size == 2) {
      Immediate(static_cast<const uint16_t>(imm));
    } else if (size == 4) {
      Immediate(static_cast<const uint32_t>(imm));
    } else if (size == 8) {
      Immediate(static_cast<const uint64_t>(imm));
    }
  }

  void Push(int reg);
  void PushImm(uint32_t imm);
  void Pop(int reg);
  void Mov(int dst, int src);
  void MovToContext(uint8_t offset, int src);
  void MovFromContext(int dst, uint8_t offset);
  void MovImm(int dst, uint64_t imm);
  void AddImm(int dst, uint8_t imm);
  void AddImmToContext(int offset, uint32_t imm);
  void AddToContext(int offset, int src);
  void SubImm(int dst, uint8_t imm);
  void Inc(int dst);
  void Xor(int dst, int src);
  void Call(const void* addr);
  void Leave();
  void Return();
  void Cmp(int src, uint32_t imm);
  void Je(Label* lbl);
  void Jmp(Label* lbl);

  inline void Bind(Label* lbl) {
    assert(lbl->offset == -1);

    lbl->offset = offset;
    Label::Pos* pos;
    while ((pos = lbl->queue.Shift()) != NULL) {
      offset = pos->offset;
      Offset(lbl, pos->size);

      delete pos;
    }
    offset = lbl->offset;
  }

  inline void Offset(Label* lbl, int8_t size) {
    if (lbl->offset == -1) {
      Label::Pos* pos = new Label::Pos();
      pos->offset = offset;
      pos->size = size;

      lbl->queue.Push(pos);

      offset += size;
    } else {
      Immediate(lbl->offset - offset - size, size);
    }
  }

  inline int64_t Offset(const void* addr) {
    return reinterpret_cast<int64_t>(
        static_cast<const char*>(addr) -
        offset - reinterpret_cast<int64_t>(code->code));
  }

  inline void emit(int byte) {
    (reinterpret_cast<unsigned char*>(code->code) + offset++)[0] = byte;
  }

  Code* code;
  int32_t offset;
};

} // namespace hogan

#endif // _SRC_ASSEMBLER_H_
