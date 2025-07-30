//
// Created by andrea on 6/9/25.
//

#include "smt_alignment_checker.h"

#include <tr1/memory>

#include "../symstate/bitvector.h"
#include "../symstate//memory/flat.h"

using namespace stoke;
using namespace x64asm;

bool SmtAlignmentChecker::check( std::shared_ptr<Invariant> inv,
              const Cfg* target, const Cfg* rewrite,
              std::shared_ptr<Operation> p, std::shared_ptr<Operation> q,
              bool override_separate_stack,
              size_t start_1, size_t end_1,
              size_t start_2, size_t end_2
              ) {
  std::cout << "start_1: " << start_1 << " end_1 " << end_1 << std::endl;
  std::cout << "start_2: " << start_2 << " end_2 " << end_2 << std::endl;
  bool separate_stack = separate_stack_ || override_separate_stack;

  SymState target_sym_state("target");
  FlatMemory target_memory(separate_stack);
  target_sym_state.memory = &target_memory;

  SymState rewrite_sym_state("rewrite");
  FlatMemory rewrite_memory(separate_stack);
  rewrite_sym_state.memory = &rewrite_memory;

  std::cout << " PROCESSING REGEXES "<< std::endl;
  SymBitVector one = SymBitVector::constant(1, 1); //neutral value
  SymbolicInstructionProcessor::process_regex(&target_sym_state, p, target, true, false, one);
  SymbolicInstructionProcessor::process_regex(&rewrite_sym_state,q, rewrite, true, false, one);
  /*std::cout << target_sym_state << std::endl;
  for (auto i : target_memory.get_constraints())
  {
    std::cout << i << std::endl;
  }
  std::cout << rewrite_sym_state << std::endl;
  for (auto i : rewrite_memory.get_constraints())
  {
    std::cout << i << std::endl;
  }*/


  size_t number = 0;
  std::vector<SymBool> bool_vector;
  auto sym_bool = (*inv)(target_sym_state, rewrite_sym_state, number);
  bool_vector.push_back(sym_bool);

  //std::cout << "SYM Bool: " << std::endl;
  //std::cout << sym_bool << std::endl;
  //auto bool_vector = target_sym_state.equality_constraints(rewrite_sym_state);

  //for (auto i: bool_vector) {std::cout << i << std::endl;}

  bool result = solver_->is_sat(bool_vector); //failing
  //bool result = false;

  std::cout << "RESULT: " << result << std::endl;
  return result;
}
