//
// Created by andrea on 6/10/25.
//

#ifndef SYMBOLIC_INSTRUCTION_PROCESSOR_H
#define SYMBOLIC_INSTRUCTION_PROCESSOR_H

#include "../symstate/state.h"

#include <cassert>
#include "src/assembler.h"
#include "src/fail.h"
#include "src/instruction.h"
#include "src/opcode.h"

#include <regex>
#include "regexOperations/empty.h"
#include "regexOperations/starOperation.h"

using namespace std;
using namespace cpputil;

namespace stoke {

  typedef std::vector<uint16_t> ImmSizes;
  typedef uint16_t TargetSize;
  typedef std::pair<TargetSize, ImmSizes> SizeInfo;
  typedef std::pair<x64asm::Opcode, std::vector<x64asm::Type>> Entry;
  typedef std::pair<SizeInfo, std::vector<Entry>> Row;
  typedef std::map<std::string, Row> Table;



class SymbolicInstructionProcessor final{
public:
    // Regex is already split therefore doesn't contain any Plus Operations
    static size_t process_regex(SymState* sym_state, std::shared_ptr<Operation> regex, const Cfg* cfg, size_t falltrought, bool is_star, SymBitVector& loop_count) {
      std::cout << "process regex: " << *regex << "is_star: " << is_star << std::endl;

      if (regex->isEmpty()) { return std::numeric_limits<size_t>::max(); }
      if (auto casted_regex = std::dynamic_pointer_cast<Symbol>(regex)) {
        size_t cfg_state = casted_regex->getNumber();

        bool is_jump;
        if (cfg->has_conditional_target(cfg_state)){
          cfg->fallthrough_target(falltrought) == cfg_state ? is_jump = false : is_jump = true;
        } else {is_jump = false;}

        auto instruction = cfg->instr_begin(cfg_state);
        while (instruction!= cfg->instr_end(cfg_state)) {
          std::cout << *instruction << std::endl;
          process_instruction(sym_state, &*instruction, is_jump, is_star, loop_count);
          instruction++;
        }
        return cfg_state;
      }

      if (auto star_operation = std::dynamic_pointer_cast<StarOperation>(regex)) {
        is_star = true;
        loop_count = SymBitVector::var(8*8, star_operation->get_loop_var());
        std::cout << "loop_count: " << loop_count << std::endl;
      }

      size_t last_block = std::numeric_limits<size_t>::max();
      // concatenation or star operation
      for (auto operation : regex->getSubexpressions()) {
        last_block = process_regex(sym_state, operation, cfg, falltrought, is_star, loop_count);
        falltrought = cfg->has_fallthrough_target(last_block) ? cfg->fallthrough_target(last_block) : std::numeric_limits<size_t>::max();
      }
      return last_block;
    }

//private:   // due to testing
  // TODO: add star function
  static bool process_instruction(SymState* sym_state, const x64asm::Instruction* instruction, bool is_condition, bool is_star, SymBitVector& loop_count) {
    auto opcode = instruction->get_opcode();
    auto arity = instruction->arity();

    //std::cout << "Opcode: " << opcode << " Arity: " << arity << std::endl;
    //std::cout << "loop_count: "  << loop_count << std::endl;

    switch (opcode)
    {
    case x64asm::TEST_R32_R32: {
        auto reg_1 = instruction->get_operand<x64asm::R32>(0);
        auto reg_2 = instruction->get_operand<x64asm::R32>(1);
        auto temp =   sym_state->gp[reg_1]  & sym_state->gp[reg_2];
        sym_state->set_szp_flags(temp);
        break;
    }
    case x64asm::TEST_R8_IMM8: {
        auto reg = instruction->get_operand<x64asm::R8>(0);
        auto num = instruction->get_operand<x64asm::Imm8>(1);
        auto temp = sym_state->gp[reg] & SymBitVector::constant(8, num);
        sym_state->set_szp_flags(temp);
        break;
    }
    case x64asm::JNE_LABEL:
      is_condition = !is_condition;
    case x64asm::JE_LABEL:
      {
        SymBool condition = is_condition ? sym_state->rf[3]== SymBool::_true() : sym_state->rf[3]== SymBool::_false(); // ZF = 3
        sym_state->add_constraint(condition);
        break;
    }
    case x64asm::NOT_M32:
      {
        auto mem = instruction->get_operand<x64asm::M32>(0);
        auto addres = sym_state->get_addr(mem);
        //std::cout << "M32: " << mem << " On the adress: " << addres << endl;
        DereferenceInfo dereference;
        auto value = (sym_state->memory)->read(addres, 32, dereference).first;


        auto false_value = value^SymBitVector::constant(32, 0xFFFFFFFF);
        auto true_value = value;
        auto bool_value = loop_count.s_mod(SymBitVector::constant(8*8, 2)) == SymBitVector::constant(8*8, 0);
        auto ite = bool_value.ite(true_value, false_value);

        //std::cout << "bool_value: " << bool_value << std::endl;
        //std::cout << "ite: " << ite << std::endl;

        sym_state->memory->write(addres, ite, 32, dereference);
      //std::cout << "What on the mem: " << (sym_state->memory)->read(addres, 32, dereference).first << endl;
      break;
    }
    case x64asm::NOT_M64: {
        auto mem = instruction->get_operand<x64asm::M64>(0);
        auto addres = sym_state->get_addr(mem);
        DereferenceInfo dereference;
        auto value = (sym_state->memory)->read(addres, 64, dereference).first;
        sym_state->memory->write(addres, value^SymBitVector::constant(64, 0xFFFFFFFFFFFFFFFF), 64, dereference);
        break;
    }

    case x64asm::ADD_AL_IMM8:
    case x64asm::ADD_AX_IMM16:
    case x64asm::ADD_EAX_IMM32:
    case x64asm::ADD_M16_IMM16:
    case x64asm::ADD_M16_IMM8:
    case x64asm::ADD_M16_R16:
    case x64asm::ADD_M32_IMM32:
    case x64asm::ADD_M32_IMM8:
    case x64asm::ADD_M32_R32:
    case x64asm::ADD_M64_IMM32:
    case x64asm::ADD_M64_IMM8:
    case x64asm::ADD_M64_R64:
    case x64asm::ADD_M8_IMM8:
    case x64asm::ADD_M8_R8:
    case x64asm::ADD_M8_RH:
      break;

    case x64asm::ADD_R16_IMM16:
    case x64asm::ADD_R16_IMM8:
    case x64asm::ADD_R16_M16:
    case x64asm::ADD_R16_R16:
    case x64asm::ADD_R16_R16_1:
    case x64asm::ADD_R32_IMM32:
    case x64asm::ADD_R32_IMM8:
    case x64asm::ADD_R32_M32:
    case x64asm::ADD_R32_R32:
    case x64asm::ADD_R32_R32_1:
      break;
    case x64asm::ADD_R64_IMM32:
      {
        auto reg = instruction->get_operand<x64asm::R64>(0);
        auto num = instruction->get_operand<x64asm::Imm32>(1);
        add_subtract_operations(sym_state, reg,  SymBitVector::constant(64, num), true, is_star, loop_count);
        break;
      }
    case x64asm::ADD_R64_IMM8:
      {

        auto reg = instruction->get_operand<x64asm::R64>(0);
        auto num = instruction->get_operand<x64asm::Imm8>(1);
        add_subtract_operations(sym_state, reg,  SymBitVector::constant(64, num), true, is_star, loop_count);
        break;
      }
    case x64asm::ADD_R64_M64:
    case x64asm::ADD_R64_R64: { //addq_r64_r64
        x64asm::R64 reg_1 = instruction->get_operand<x64asm::R64>(0);
        x64asm::R64 reg_2 = instruction->get_operand<x64asm::R64>(1);
        add_subtract_operations(sym_state, reg_1, sym_state->gp[reg_2], true, is_star, loop_count);
        break;
    }
    case x64asm::ADD_R64_R64_1:
    case x64asm::ADD_R8_IMM8:
    case x64asm::ADD_R8_M8:
    case x64asm::ADD_R8_R8:
    case x64asm::ADD_R8_R8_1:
    case x64asm::ADD_R8_RH:
    case x64asm::ADD_R8_RH_1:
    case x64asm::ADD_RAX_IMM32:
    case x64asm::ADD_RH_IMM8:
    case x64asm::ADD_RH_M8:
    case x64asm::ADD_RH_R8:
    case x64asm::ADD_RH_R8_1:
    case x64asm::ADD_RH_RH:
    case x64asm::ADD_RH_RH_1:
      break;

    case x64asm::SUB_R32_IMM8: {
        auto reg = instruction->get_operand<x64asm::R32>(0);
        SymBitVector num = SymBitVector::constant(32,instruction->get_operand<x64asm::Imm8>(1));
        add_subtract_operations(sym_state, reg,  num, false, is_star, loop_count);
    }
    case x64asm::FNOP:
    case x64asm::NOP: //nop
    case x64asm::LABEL_DEFN: // check this up
    case x64asm::RET:
      break;
    case x64asm::MOV_R64_R64: { //movq_r64_r64 Arity: 2
        auto reg_1 = instruction->get_operand<x64asm::R64>(0);
        auto reg_2 = instruction->get_operand<x64asm::R64>(1);
        sym_state->set(reg_1, sym_state->gp[reg_2]);
        break;
    }
    case x64asm::LEA_R32_M16: { //leal_r32_m16 Arity: 2
      auto reg = instruction->get_operand<x64asm::R32>(0);
      auto mem = instruction->get_operand<x64asm::M16>(1);
      auto mem_loc = sym_state->get_addr(mem);
      DereferenceInfo dereference_info;
      sym_state->set(reg, (sym_state->memory->read(mem_loc, 16, dereference_info)).first);
      //std::cout << "Reg: " << reg << " Num: " << num << std::endl;
      break;
    }
    case x64asm::LEA_R64_M16: { //leaq_r64_m16 Arity: 2
      auto reg = instruction->get_operand<x64asm::R64>(0);
      auto mem = instruction->get_operand<x64asm::M16>(1);
      auto mem_loc = sym_state->get_addr(mem);
      DereferenceInfo dereference_info;
      sym_state->set(reg, (sym_state->memory->read(mem_loc, 16, dereference_info)).first);
      break;
    }
    case x64asm::CMP_R64_R64: {
      auto reg_1 = instruction->get_operand<x64asm::R64>(0);
      auto reg_2 = instruction->get_operand<x64asm::R64>(1);
      auto temp = sym_state->gp[reg_2] - sym_state->gp[reg_1];
      sym_state->set_szp_flags(temp);
      break;
    }
    default:
      std::cout << "Unhandled instruction type" << std::endl;
			assert(false);
		}
    return true;
  }

private:
  SymbolicInstructionProcessor() {}

  template<typename T1>
  static void add_subtract_operations(SymState* sym_state, const T1 reg, const SymBitVector num, bool is_add, bool is_star, SymBitVector& loop_count) {
    if (std::is_same<T1, x64asm::R64>::value || std::is_same<T1, x64asm::R32>::value
      || std::is_same<T1, x64asm::R16>::value || std::is_same<T1, x64asm::R8>::value) {
      SymBitVector temp;
      if (is_add) {
        is_star ? temp = sym_state->gp[reg] + loop_count * num : temp = sym_state->gp[reg]+ num;
      } else {
        is_star ? temp = sym_state->gp[reg] - loop_count * num : temp = sym_state->gp[reg] - num;
      }
      sym_state->set(reg, temp);
      sym_state->set_szp_flags(temp);
    } else
    {
      /*
      SymBitVector temp;
      is_add
        ? temp = sym_state->gp[reg]+ num
        : temp = sym_state->gp[reg] - num;
      sym_state->set(reg, temp);
      */
    }
  }
};
} //namespace


#endif //SYMBOLIC_INSTRUCTION_PROCESSOR_H
