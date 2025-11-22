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

#include "processing_info.h"
#include "../ext/x64asm/src/reg_set.h"
#include "../symstate/memory/flat.h"
#include "regexOperations/empty.h"
#include "regexOperations/starOperation.h"

using namespace std;
using namespace x64asm;

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
    // returns id of last cfg_state and bool if the
    static void process_regex(SymState* sym_state, std::shared_ptr<Operation> regex, const Cfg* cfg,
      //size_t previous_state,
      ProcessingInfo& info, bool is_star, SymBitVector& loop_count, SymRegs beginning_state, bool star_zero
      ) {
      //std::cout << "process regex: " << *regex << "  is_star: " << info.is_star << std::endl;

      if (regex->isEmpty()) { return; } // std::numeric_limits<size_t>::max(); }

      if (auto casted_regex = std::dynamic_pointer_cast<Symbol>(regex)) {
        size_t cfg_state = casted_regex->getNumber();

        //bool is_jump;
        /*if (previous_state != cfg_state and cfg->has_fallthrough_target(previous_state) and cfg->fallthrough_target(previous_state) != cfg_state){
          is_jump = true;
        } else {is_jump = false;}*/

        //if (is_star) {is_jump = true;} // out of the core of what even is star operation
        //previous state jump condition handling

        if (info.prev_state_ends_with_jump and info.previous_state != cfg_state
            and cfg->has_fallthrough_target(info.previous_state) and cfg->fallthrough_target(info.previous_state) == cfg_state) {
          auto last = sym_state->constraints.back();
          sym_state->constraints.pop_back();
          sym_state->add_constraint(!last);
        }

        info.previous_state = cfg_state;
        info.prev_state_ends_with_jump = false;

        auto instruction = cfg->instr_begin(cfg_state);
        while (instruction!= cfg->instr_end(cfg_state)) {
          std::cout << *instruction << std::endl;
          process_instruction(sym_state, &*instruction, info, is_star, loop_count, beginning_state);
          instruction++;
        }

        return;
      }

      if (auto star_operation = std::dynamic_pointer_cast<StarOperation>(regex)) {
        is_star = true;
        loop_count = SymBitVector::var(64, star_operation->get_loop_var()); //* loop_count;
        info.star_variable_names.push_back(star_operation->get_loop_var());
        if (star_zero) {
          info.star_constraints.push_back(loop_count.s_ge(SymBitVector::constant(64, 0)));
        } else {
          info.star_constraints.push_back(loop_count.s_ge(SymBitVector::constant(64, 1)));
        }
        info.star_constraints.push_back(loop_count.s_lt(SymBitVector::constant(64, 1000000))); // needed because of overfloing
        beginning_state = copy_sym_state(sym_state, beginning_state); //TODO: fix copping of sym_state
        std::cout << "Is star operation: " << std::endl;
        /*f (star_operation->is_plus_one()) {
          std::cout << "Is plus one: " << star_operation->is_plus_one() << std::endl;
          SymBool condition = loop_count > SymBitVector::constant(8*8, 0); // ZF = 3
          star_constraints.push_back(condition);
        }*/
        std::cout << "loop_count: " << loop_count << std::endl;
      }

      // concatenation or star operation
      for (auto operation : regex->getSubexpressions()) {
        //previous_state =
        process_regex(sym_state, operation, cfg, info, is_star, loop_count, beginning_state, true);
      }

      //return previous_state;
    }

//private:   // due to testing
  static bool process_instruction(SymState* sym_state, const x64asm::Instruction* instruction, ProcessingInfo& info,
      bool is_star, SymBitVector& loop_count, SymRegs& beginning_state) {
    auto opcode = instruction->get_opcode();
    auto arity = instruction->arity();

    std::cout << "Opcode: " << opcode << " Arity: " << arity << std::endl;

    //std::cout << "loop_count: "  << loop_count << std::endl;

    switch (opcode)
    {
    case x64asm::TEST_AL_IMM8: {
      auto al = instruction->get_operand<x64asm::Al>(0);
      auto num = instruction->get_operand<x64asm::Imm8>(1);
      auto masked_reg = sym_state->gp[al] & SymBitVector::constant(8, 0xFF).zero_extend(64);

      auto temp = masked_reg  & SymBitVector::constant(8, num).zero_extend(64);
      //auto temp_64 = temp.sign_extend(64);
      sym_state->set_szp_flags(temp, 64);
      break;
    }
    case x64asm::TEST_R8_IMM8: {
      auto reg = instruction->get_operand<x64asm::R8>(0);
      auto num = instruction->get_operand<x64asm::Imm8>(1);
      auto masked_reg = sym_state->gp[reg] & SymBitVector::constant(8, 0xFF).zero_extend(64);

      auto temp = masked_reg  & SymBitVector::constant(8, num).zero_extend(64);
      //auto temp_64 = temp.sign_extend(64);
      sym_state->set_szp_flags(temp, 64);
      break;
    }
    case x64asm::TEST_R32_R32: {
      auto reg_1 = instruction->get_operand<x64asm::R32>(0);
      auto reg_2 = instruction->get_operand<x64asm::R32>(1);
      auto reg32_1 = sym_state->gp[reg_1][31][0];
      auto reg32_2 = sym_state->gp[reg_2][31][0];
      //auto mask = SymBitVector::constant(32, 0xFFFFFFFF).zero_extend(64);

      auto temp = reg32_1  & reg32_2;
      sym_state->set_szp_flags(temp);
      break;
    }
    case x64asm::TEST_R64_R64: {
      auto reg_1 = instruction->get_operand<x64asm::R64>(0);
      auto reg_2 = instruction->get_operand<x64asm::R64>(1);

      auto temp = sym_state->gp[reg_1]  & sym_state->gp[reg_2];
      sym_state->set_szp_flags(temp, 64);
      break;
    }
    case x64asm::JLE_LABEL: {
      sym_state->add_constraint(!(*sym_state)[x64asm::eflags_zf] | (*sym_state)[eflags_sf] );
      info.prev_state_ends_with_jump = true;
      break;
    }
    case x64asm::JNE_LABEL: {
        sym_state->add_constraint(!(*sym_state)[x64asm::eflags_zf]);
        info.prev_state_ends_with_jump = true;
        break;
    }
    case x64asm::JE_LABEL: {
      sym_state->add_constraint((*sym_state)[x64asm::eflags_zf]);
      info.prev_state_ends_with_jump = true;
      break;
    }
    case x64asm::JA_LABEL: {
      sym_state->add_constraint(!(*sym_state)[x64asm::eflags_cf] & !(*sym_state)[x64asm::eflags_zf]);
      info.prev_state_ends_with_jump = true;
      break;
    }
    case x64asm::JS_LABEL: {
      sym_state->add_constraint((*sym_state)[x64asm::eflags_sf]);
      info.prev_state_ends_with_jump = true;
      break;
    }
    case x64asm::JG_LABEL: {
      sym_state->add_constraint(((*sym_state)[x64asm::eflags_sf] == (*sym_state)[x64asm::eflags_of]) & !(*sym_state)[x64asm::eflags_zf]);
      info.prev_state_ends_with_jump = true;
      break;
    }
    case x64asm::AND_R64_R64: {
        auto reg_1 = instruction->get_operand<x64asm::R64>(0);
        auto reg_2 = instruction->get_operand<x64asm::R64>(1);
        auto temp = sym_state->gp[reg_1] & sym_state->gp[reg_2];

        sym_state->set_szp_flags(temp, 64);
        sym_state->set(reg_1, temp);
        break;
    }
    case x64asm::NOT_R64:{
        auto reg = instruction->get_operand<x64asm::R64>(0);

        if (is_star) {
          auto bool_value = loop_count.s_mod(SymBitVector::constant(64, 2)) == SymBitVector::constant(64, 0);
          sym_state->set(reg, bool_value.ite(sym_state->gp[reg], sym_state->gp[reg]^SymBitVector::constant(64, 0xFFFFFFFFFFFFFFFF)));
          break;
        }
        sym_state->set(reg, sym_state->gp[reg]^SymBitVector::constant(64, 0xFFFFFFFFFFFFFFFF));
        break;
    }
    case x64asm::NOT_M32: {
        auto mem = instruction->get_operand<x64asm::M32>(0);
        auto addres = sym_state->get_addr(mem);
        //std::cout << "M32: " << mem << " On the adress: " << addres << endl;
        DereferenceInfo dereference;
        SymBitVector value = (sym_state->memory)->read(addres, 32, dereference).first;
        //std::cout << "value: " << value << " size: " << value.width() << std::endl;

        SymBitVector write_value;

        if (is_star) {
          auto false_value = value^SymBitVector::constant(32, 0xFFFFFFFF);
          auto true_value = value;
          auto bool_value = loop_count.s_mod(SymBitVector::constant(64, 2)) == SymBitVector::constant(64, 0);
          write_value = bool_value.ite(true_value, false_value);
          info.number_of_bits_changed = info.number_of_bits_changed + SymBitVector::constant(64, 32) * loop_count;
        } else {
          write_value = value^SymBitVector::constant(32, 0xFFFFFFFF);
          info.number_of_bits_changed = info.number_of_bits_changed + SymBitVector::constant(64, 32);
        }


        std::cout << "Write value: " << write_value << " write value size: " << write_value.width() << std::endl;
        SymBool memory_constrain = sym_state->memory->write(addres, write_value, 32, dereference);
        sym_state->add_constraint(!memory_constrain);
        info.memory_axioms.push_back(value == write_value);

        //std::cout << "What on the mem: " << (sym_state->memory)->read(addres, 32, dereference).first << endl;
        break;
    }
    case x64asm::NOT_M64: {
        auto mem = instruction->get_operand<x64asm::M64>(0);
        auto addres = sym_state->get_addr(mem);
        DereferenceInfo dereference;

        auto value = (sym_state->memory)->read(addres, 64, dereference).first;
        std::cout << "NOT M64 break?" << std::endl;

        SymBitVector write_value;

        if (is_star) {
          auto false_value = value^SymBitVector::constant(64, 0xFFFFFFFFFFFFFFFF);
          auto true_value = value;
          auto bool_value = loop_count.s_mod(SymBitVector::constant(64, 2)) == SymBitVector::constant(64, 0);
          write_value = bool_value.ite(true_value, false_value);
          info.number_of_bits_changed = info.number_of_bits_changed + SymBitVector::constant(64, 64) * loop_count;
        } else {
          write_value = value^SymBitVector::constant(64, 0xFFFFFFFFFFFFFFFF);
          info.number_of_bits_changed = info.number_of_bits_changed + SymBitVector::constant(64, 64);
        }

        SymBool memory_constrain = sym_state->memory->write(addres, write_value, 64, dereference);
        sym_state->add_constraint(!memory_constrain);
        info.memory_axioms.push_back(value == write_value);

        break;
    }
    case x64asm::XOR_R32_R32: {
        auto destination = instruction->get_operand<x64asm::R32>(0);
        auto source = instruction->get_operand<x64asm::R32>(1);

        auto temp = sym_state->gp[destination][31][0] ^ sym_state->gp[source][31][0];
        sym_state->set(destination, temp);
        sym_state->set_szp_flags(temp);
        break;
    }
    case x64asm::SHL_R32_IMM8: {
        auto reg = instruction->get_operand<x64asm::R32>(0);
        auto reg_32 = sym_state->gp[reg][31][0];
        auto num = SymBitVector::constant(32, instruction->get_operand<x64asm::Imm8>(1));
        auto temp = reg_32 << num;

        sym_state->set(reg, temp[31][0]);
        sym_state->set_szp_flags(temp[31][0]);
        break;
    }
    case x64asm::SHR_R32_IMM8: {
        auto reg = instruction->get_operand<x64asm::R32>(0);
        auto reg_32 = sym_state->gp[reg][31][0];
        auto num = SymBitVector::constant(32, instruction->get_operand<x64asm::Imm8>(1));
        auto temp = reg_32 >> num;
        std::cout << "SHR_R32_IMM8 width: " << temp.width() << std::endl;
        sym_state->set(reg, temp[31][0]);
        sym_state->set_szp_flags(temp[31][0]);
        break;
    }
    case x64asm::ADD_R32_IMM8: {
        auto reg = instruction->get_operand<x64asm::R32>(0);
        SymBitVector reg32 = (sym_state->gp[reg])[31][0];
        SymBitVector num = SymBitVector::constant(8,instruction->get_operand<x64asm::Imm8>(1)).sign_extend(32);

        SymBitVector temp = is_star ? (reg32) + (num * (loop_count[31][0])) : (reg32) + num;

        sym_state->set(reg, temp);
        sym_state->set_szp_flags(temp);
        break;
    }
    case x64asm::ADD_R32_M32: {
        auto reg = instruction->get_operand<x64asm::R32>(0);
        SymBitVector reg32 = (sym_state->gp[reg])[31][0];
        auto mem = instruction->get_operand<x64asm::M32>(1);
        auto mem_loc = sym_state->get_addr(mem);
        DereferenceInfo dereference_info;
        auto num = sym_state->memory->read(mem_loc, 32, dereference_info).first;

        SymBitVector temp = is_star ? (reg32) + (num * (loop_count[31][0])) : (reg32) + num;

        sym_state->set(reg, temp);
        sym_state->set_szp_flags(temp);
        break;
    }
    case x64asm::ADD_R64_IMM32: {
        auto reg = instruction->get_operand<x64asm::R64>(0);
        auto num = instruction->get_operand<x64asm::Imm32>(1);
        add_subtract_operations(sym_state, reg,  SymBitVector::constant(64, num), true, is_star, loop_count);
        break;
    }
    case x64asm::ADD_R64_IMM8: {
        auto reg = instruction->get_operand<x64asm::R64>(0);
        SymBitVector num = SymBitVector::constant(8,instruction->get_operand<x64asm::Imm8>(1)).sign_extend(64);

        SymBitVector temp = is_star ? num * loop_count + sym_state->gp[reg] : num + sym_state->gp[reg];

        sym_state->set(reg, temp);
        sym_state->set_szp_flags(temp, 64);
        break;
    }
    case x64asm::ADD_R64_R64: { //addq_r64_r64
      x64asm::R64 reg_1 = instruction->get_operand<x64asm::R64>(0);
      x64asm::R64 reg_2 = instruction->get_operand<x64asm::R64>(1);
      //std::cout << "reg_1 = " << reg_1 << " reg_2 = " << reg_2 << std::endl;
      add_subtract_operations(sym_state, reg_1, sym_state->gp[reg_2], true, is_star, loop_count);
      break;
    }
    case x64asm::ADD_M32_R32: {
      auto mem = instruction->get_operand<x64asm::M32>(0);
      auto mem_loc = sym_state->get_addr(mem);
      auto reg = instruction->get_operand<x64asm::R32>(1);
      SymBitVector reg32 = (sym_state->gp[reg])[31][0];

      DereferenceInfo dereference_info_r;
      auto num = sym_state->memory->read(mem_loc, 32, dereference_info_r).first;

      SymBitVector temp = is_star ? (reg32) + (num * (loop_count[31][0])) : (reg32) + num;

      DereferenceInfo dereference_info_w;
      sym_state->memory->write(mem_loc, temp, 32, dereference_info_w);
      is_star ? info.number_of_bits_changed = info.number_of_bits_changed + SymBitVector::constant(64, 32) * loop_count
              : info.number_of_bits_changed = info.number_of_bits_changed +  SymBitVector::constant(64, 32);
      sym_state->set_szp_flags(temp);
      break;
    }
    case x64asm::PADDD_XMM_XMM: {
      auto xmm_1 = instruction->get_operand<x64asm::Xmm>(0);
      auto xmm_2 = instruction->get_operand<x64asm::Xmm>(1);

      auto xmm_1_num = sym_state->sse[xmm_1];
      auto xmm_2_num = sym_state->sse[xmm_2];

      SymBitVector temp = is_star ? xmm_1_num * loop_count.zero_extend(128) + xmm_2_num : xmm_1_num + xmm_2_num;

      sym_state->set(xmm_1, temp);
      sym_state->set_szp_flags(temp, 128);
      break;
    }
    case x64asm::SUB_R32_IMM8: {
      auto reg = instruction->get_operand<x64asm::R32>(0);
      SymBitVector reg64 = sym_state->gp[reg];
      SymBitVector reg32 = reg64[31][0];

      std::cout << "reg " << reg << std::endl;
      std::cout << "reg in: " << reg32 << " width: " << reg32.width() << std::endl;

      SymBitVector num = SymBitVector::constant(8,instruction->get_operand<x64asm::Imm8>(1)).sign_extend(32);
      SymBitVector temp = is_star ? (reg32) - (num * (loop_count[31][0])) : (reg32) - num;
      //temp = mask & temp;
      std::cout << "temp: " << temp << " width: " << temp.width() << std::endl;
      //break;

      sym_state->set(reg, temp);
      sym_state->set_szp_flags(temp);
      break;
    }
    case x64asm::SUB_R64_R64: {
      auto reg_1 = instruction->get_operand<x64asm::R64>(0);
      auto reg_2 = instruction->get_operand<x64asm::R64>(1);

      SymBitVector temp = is_star ? - (sym_state->gp[reg_2]) * (loop_count) + (sym_state->gp[reg_1]) : - (sym_state->gp[reg_2]) + (sym_state->gp[reg_1]);
      sym_state->set(reg_1, temp);
      sym_state->set_szp_flags(temp);

      break;
    }
    case x64asm::JMP_LABEL:
    case x64asm::FNOP:
    case x64asm::NOP: //nop
    case x64asm::LABEL_DEFN: // check this up
    case x64asm::RET:
      break;
    case x64asm::MOV_R32_IMM32: {
      auto reg_1 = instruction->get_operand<x64asm::R32>(0);
      auto num = SymBitVector::constant(32,instruction->get_operand<x64asm::Imm32>(1));
      sym_state->set(reg_1, num);
      break;
    }
    case x64asm::MOV_R32_R32: {
      auto reg_1 = instruction->get_operand<x64asm::R32>(0);
      auto reg_2 = instruction->get_operand<x64asm::R32>(1);
      auto reg_32 = sym_state->gp[reg_2][31][0];
      sym_state->set(reg_1, reg_32);
      break;
    }
    case x64asm::MOV_R32_M32: {
      auto reg_1 = instruction->get_operand<x64asm::R32>(0);
      auto mem = instruction->get_operand<x64asm::M32>(1);

      auto mem_loc = sym_state->get_addr(mem);
      DereferenceInfo dereference_info;
      auto num = sym_state->memory->read(mem_loc, 32, dereference_info).first;

      sym_state->set(reg_1, num);
      break;
    }
    case x64asm::MOVSXD_R64_R32: {
      auto reg_1 = instruction->get_operand<x64asm::R64>(0);
      auto reg_2 = instruction->get_operand<x64asm::R32>(1);
      auto reg_s64_2 = (sym_state->gp[reg_2][31][0]).sign_extend(64);
      sym_state->set(reg_1, reg_s64_2);
      break;
    }
    case x64asm::MOV_R64_R64: { //movq_r64_r64 Arity: 2
      auto reg_1 = instruction->get_operand<x64asm::R64>(0);
      auto reg_2 = instruction->get_operand<x64asm::R64>(1);
      sym_state->set(reg_1, sym_state->gp[reg_2]);
      break;
    }
    case x64asm::MOV_R64_IMM64: {
      auto reg_1 = instruction->get_operand<x64asm::R64>(0);
      auto num = SymBitVector::constant(64,instruction->get_operand<x64asm::Imm64>(1));
      sym_state->set(reg_1, num);
      break;
    }
    case x64asm::MOV_R64_M64: {
      auto reg_1 = instruction->get_operand<x64asm::R64>(0);
      auto mem = instruction->get_operand<x64asm::M64>(1);

      auto mem_loc = sym_state->get_addr(mem);
      DereferenceInfo dereference_info;
      auto num = sym_state->memory->read(mem_loc, 64, dereference_info).first;

      sym_state->set(reg_1, num);
      break;
    }
    case x64asm::MOV_M32_R32: {
      auto mem = instruction->get_operand<x64asm::M32>(0);
      auto reg = instruction->get_operand<x64asm::R32>(1);
      auto reg_32 = (sym_state->gp[reg])[31][0];

      auto mem_loc = sym_state->get_addr(mem);
      DereferenceInfo dereference_info;
      sym_state->memory->write(mem_loc, reg_32, 32, dereference_info);

      is_star ? info.number_of_bits_changed = info.number_of_bits_changed + SymBitVector::constant(64, 32) * loop_count
              : info.number_of_bits_changed = info.number_of_bits_changed +  SymBitVector::constant(64, 32);
      break;
    }
    case x64asm::MOVDQA_XMM_M128: {
      auto reg_1 = instruction->get_operand<x64asm::Xmm>(0);
      auto mem = instruction->get_operand<x64asm::M128>(1);

      auto mem_loc = sym_state->get_addr(mem);
      DereferenceInfo dereference_info;
      auto num = sym_state->memory->read(mem_loc, 128, dereference_info).first;

      sym_state->set(reg_1, num);
      break;
    }
    case x64asm::MOVAPS_M128_XMM: {
      auto xmm = instruction->get_operand<x64asm::Xmm>(1);
      auto mem = instruction->get_operand<x64asm::M128>(0);

      auto mem_loc = sym_state->get_addr(mem);
      DereferenceInfo dereference_info;
      sym_state->memory->write(mem_loc, sym_state->sse[xmm], 128, dereference_info);
      is_star ? info.number_of_bits_changed = info.number_of_bits_changed + SymBitVector::constant(64, 128) * loop_count
              : info.number_of_bits_changed = info.number_of_bits_changed +  SymBitVector::constant(64, 128);
      break;
    }
    case x64asm::LEA_R32_M16: { //leal_r32_m16 Arity: 2
      auto reg = instruction->get_operand<x64asm::R32>(0);
      auto mem = instruction->get_operand<x64asm::M16>(1);
      auto mem_loc = sym_state->get_addr(mem);
      DereferenceInfo dereference_info;
      sym_state->set(reg, (sym_state->memory->read(mem_loc, 16, dereference_info)).first.sign_extend(32));
      //std::cout << "Reg: " << reg << " Num: " << num << std::endl;
      break;
    }
    case x64asm::LEA_R64_M16: { //leaq_r64_m16 Arity: 2
      auto reg = instruction->get_operand<x64asm::R64>(0);
      auto mem = instruction->get_operand<x64asm::M16>(1);
      auto mem_loc = sym_state->get_addr(mem);
      DereferenceInfo dereference_info;
      sym_state->set(reg, (sym_state->memory->read(mem_loc, 16, dereference_info)).first.sign_extend(64));
      break;
    }
    case x64asm::CMP_M8_IMM8: {
      auto mem = instruction->get_operand<x64asm::M8>(0);
      SymBitVector num = SymBitVector::constant(8,instruction->get_operand<x64asm::Imm8>(1));
      if (is_star) {
        //TODO: fix the loop
        /*SymBitVector loop_halting_constrain;

        SymBool mem_1_constant = (beginning_state[reg_1] == sym_state->gp[reg_1]).ite(SymBool::constant(true), SymBool::constant(false));

        SymBool mem_1_increase = (beginning_state[reg_1].s_lt(sym_state->gp[reg_1])).ite(SymBool::constant(true), SymBool::constant(false));


        SymBool if_1 = (reg_1_constant == reg_2_constant) | (reg_1_increase == reg_2_increase); //
        SymBitVector then_1 = SymBitVector::tmp_var(64);

        SymBool if_2 = ((reg_1_constant == SymBool::_true()) & (reg_2_increase == SymBool::_true()))
                      | ((reg_1_increase == SymBool::_false()) & (reg_2_constant == SymBool::_true()));
        SymBitVector then_2 = sym_state->gp[reg_1] - sym_state->gp[reg_2];
        SymBitVector else_2 = sym_state->gp[reg_2] - sym_state->gp[reg_1];

        SymBitVector ite_2 = if_2.ite(then_2, else_2);

        loop_halting_constrain = if_1.ite(then_1, ite_2);
        sym_state->add_constraint(loop_halting_constrain.s_gt(SymBitVector::constant(64, 0)));

        SymBitVector temp = sym_state->gp[reg_2] - sym_state->gp[reg_1];
        sym_state->set_szp_flags(temp, 64);
        */

        break;
      }
      auto mem_loc = sym_state->get_addr(mem);
      DereferenceInfo dereference_info;
      auto temp = sym_state->memory->read(mem_loc, 8, dereference_info).first - num;
      sym_state->set_szp_flags(temp, 8);
      break;
    }
    case x64asm::CMP_R32_R32: {
      auto reg_1 = instruction->get_operand<x64asm::R32>(0);
      SymBitVector reg32_1 = (sym_state->gp[reg_1])[31][0];
      auto reg_2 = instruction->get_operand<x64asm::R32>(1);
      SymBitVector reg32_2 = (sym_state->gp[reg_2])[31][0];

      if (is_star)
      {
        SymBitVector loop_halting_constrain;
        auto begin_reg32_1 = beginning_state[reg_1][31][0];
        auto begin_reg32_2 = beginning_state[reg_2][31][0];

        SymBool reg_1_constant = (begin_reg32_1 == reg32_1).ite(SymBool::constant(true), SymBool::constant(false));
        SymBool reg_2_constant = (begin_reg32_2 == reg32_2).ite(SymBool::constant(true), SymBool::constant(false));

        SymBool reg_1_increase = (begin_reg32_1.s_lt(reg32_1)).ite(SymBool::constant(true), SymBool::constant(false));
        SymBool reg_2_increase = (begin_reg32_2.s_lt(reg32_2)).ite(SymBool::constant(true), SymBool::constant(false));

        SymBool if_1 = (reg_1_constant == reg_2_constant) | (reg_1_increase == reg_2_increase); //
        SymBitVector then_1 = SymBitVector::tmp_var(32);
        //SymBitVector else_1;

        SymBool if_2 = ((reg_1_constant == SymBool::_true()) & (reg_2_increase == SymBool::_true()))
                      | ((reg_1_increase == SymBool::_false()) & (reg_2_constant == SymBool::_true()));
        SymBitVector then_2 = reg32_1 - reg32_2;
        SymBitVector else_2 = reg32_2 - reg32_1;

        SymBitVector ite_2 = if_2.ite(then_2, else_2);

        loop_halting_constrain = if_1.ite(then_1, ite_2);
        sym_state->add_constraint(loop_halting_constrain.s_gt(SymBitVector::constant(32, 0)));

        SymBitVector temp = reg32_2 - reg32_1;
        sym_state->set_szp_flags(temp);

        break;
      }

      auto temp = reg32_2 - reg32_1;
      sym_state->set_szp_flags(temp, 32);
      break;
    }
    case x64asm::CMP_R32_IMM8: {
      auto reg = instruction->get_operand<x64asm::R32>(0);
      SymBitVector reg32 = (sym_state->gp[reg])[31][0];
      auto num = SymBitVector::constant(8,instruction->get_operand<x64asm::Imm8>(1)).zero_extend(32);

      if (is_star)
      {
        SymBitVector loop_halting_constrain;
        auto begin_reg32 = beginning_state[reg][31][0];

        SymBool reg_1_constant = (begin_reg32 == reg32).ite(SymBool::constant(true), SymBool::constant(false));
        SymBool reg_1_increase = (begin_reg32.s_lt(reg32)).ite(SymBool::constant(true), SymBool::constant(false));

        SymBool if_1 = (reg_1_constant);
        SymBitVector then_1 = SymBitVector::tmp_var(32); //sus

        SymBool if_2 = (reg_1_increase == SymBool::_true());
        SymBitVector then_2 = num - reg32;
        SymBitVector else_2 = reg32 - num;

        SymBitVector ite_2 = if_2.ite(then_2, else_2);

        loop_halting_constrain = if_1.ite(then_1, ite_2);
        sym_state->add_constraint(loop_halting_constrain.s_gt(SymBitVector::constant(32, 0)));

        SymBitVector temp = reg32 - num;
        sym_state->set_szp_flags(temp);

        break;
      }

      auto temp = reg32 - num;
      sym_state->set_szp_flags(temp, 32);
      break;
    }
    case x64asm::CMP_R32_IMM32: {
        auto reg = instruction->get_operand<x64asm::R32>(0);
        SymBitVector reg32 = (sym_state->gp[reg])[31][0];
        auto num = SymBitVector::constant(32,instruction->get_operand<x64asm::Imm32>(1));

        if (is_star)
        {
          SymBitVector loop_halting_constrain;
          auto begin_reg32 = beginning_state[reg][31][0];

          SymBool reg_1_constant = (begin_reg32 == reg32).ite(SymBool::constant(true), SymBool::constant(false));
          SymBool reg_1_increase = (begin_reg32.s_lt(reg32)).ite(SymBool::constant(true), SymBool::constant(false));

          SymBool if_1 = (reg_1_constant);
          SymBitVector then_1 = SymBitVector::tmp_var(32);

          SymBool if_2 = (reg_1_increase == SymBool::_true());
          SymBitVector then_2 = num - reg32;
          SymBitVector else_2 = reg32 - num;

          SymBitVector ite_2 = if_2.ite(then_2, else_2);

          loop_halting_constrain = if_1.ite(then_1, ite_2);
          sym_state->add_constraint(loop_halting_constrain.s_gt(SymBitVector::constant(32, 0)));

          SymBitVector temp = reg32 - num;
          sym_state->set_szp_flags(temp);

          break;
        }

        auto temp = reg32 - num;
        sym_state->set_szp_flags(temp, 32);
        break;
    }
    case x64asm::CMP_R64_R64: {
      auto reg_1 = instruction->get_operand<x64asm::R64>(0);
      auto reg_2 = instruction->get_operand<x64asm::R64>(1);
      if (is_star)
      {
        SymBitVector loop_halting_constrain;
        std::cout << "begining reg_1 " << beginning_state[reg_1] << std::endl;
        std::cout << "current reg_1 " << sym_state->gp[reg_1] << std::endl;

        SymBool reg_1_constant = (beginning_state[reg_1] == sym_state->gp[reg_1]).ite(SymBool::constant(true), SymBool::constant(false));
        SymBool reg_2_constant = (beginning_state[reg_2] == sym_state->gp[reg_2]).ite(SymBool::constant(true), SymBool::constant(false));

        SymBool reg_1_increase = (beginning_state[reg_1].s_lt(sym_state->gp[reg_1])).ite(SymBool::constant(true), SymBool::constant(false));
        SymBool reg_2_increase = (beginning_state[reg_2].s_lt(sym_state->gp[reg_2])).ite(SymBool::constant(true), SymBool::constant(false));

        SymBool if_1 = (reg_1_constant == reg_2_constant) | (reg_1_increase == reg_2_increase); //
        SymBitVector then_1 = SymBitVector::tmp_var(64);
        //SymBitVector else_1;

        SymBool if_2 = ((reg_1_constant == SymBool::_true()) & (reg_2_increase == SymBool::_true()))
                      | ((reg_1_increase == SymBool::_false()) & (reg_2_constant == SymBool::_true()));
        SymBitVector then_2 = sym_state->gp[reg_1] - sym_state->gp[reg_2];
        SymBitVector else_2 = sym_state->gp[reg_2] - sym_state->gp[reg_1];

        SymBitVector ite_2 = if_2.ite(then_2, else_2);

        loop_halting_constrain = if_1.ite(then_1, ite_2);
        sym_state->add_constraint(loop_halting_constrain.s_gt(SymBitVector::constant(64, 0)));

        SymBitVector temp = sym_state->gp[reg_2] - sym_state->gp[reg_1];
        sym_state->set_szp_flags(temp, 64);

        break;
      }
      std::cout << "reg_1 = " << reg_1 << std::endl;
      std::cout << sym_state->gp[reg_1] << std::endl;
      std::cout << "reg_2 = " << reg_2 << std::endl;
      std::cout << sym_state->gp[reg_2] << std::endl;
      auto temp = sym_state->gp[reg_2] - sym_state->gp[reg_1];
      sym_state->set_szp_flags(temp, 64);
      break;
    }
    default:
      std::cout << "Unhandled instruction type" << std::endl;
      //std::string input; std::cin >> input;
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
        is_star ? temp = sym_state->gp[reg] + num * loop_count : temp = sym_state->gp[reg]+ num;
      } else {
        is_star ? temp = sym_state->gp[reg] - num * loop_count  : temp = sym_state->gp[reg] - num;
      }
      sym_state->set(reg, temp);
      auto temp_64 = temp.sign_extend(64);
      sym_state->set_szp_flags(temp_64, 64);
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

  static SymRegs copy_sym_state(SymState* sym_state, SymRegs copy) {
    std::cout << "copying symbol state" << std::endl;
    for (int i = 0; i < 16; i++) {
      copy[i] = sym_state->gp[i];
    }
    return copy;
  }
};
} //namespace


#endif //SYMBOLIC_INSTRUCTION_PROCESSOR_H
