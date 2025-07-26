//
// Created by andrea on 6/9/25.
//

#include "smt_alignment_checker.h"
using namespace stoke;
using namespace x64asm;

bool SmtAlignmentChecker::check( std::shared_ptr<Invariant> inv,
              const Cfg* target, const Cfg* rewrite,
              std::shared_ptr<Operation> p, std::shared_ptr<Operation> q,
              bool override_separate_stack
              ) {
  bool separate_stack = separate_stack_ || override_separate_stack;

  SymState target_sym_state("target");
  FlatMemory target_memory(separate_stack);
  target_sym_state.memory = &target_memory;

  SymState rewrite_sym_state("rewrite");
  FlatMemory rewrite_memory(separate_stack);
  rewrite_sym_state.memory = &rewrite_memory;

  std::cout << " PROCESSING REGEXES "<< std::endl;
  SymbolicInstructionProcessor::process_regex(&target_sym_state, p, target, false, false);
  SymbolicInstructionProcessor::process_regex(&rewrite_sym_state,q, rewrite, false, false);

  size_t number = 0;
  std::vector<SymBool> bool_vector;
  auto sym_bool = (*inv)(target_sym_state, rewrite_sym_state, number);
  bool_vector.push_back(sym_bool);
  //auto bool_vector = target_sym_state.equality_constraints(rewrite_sym_state);
  // TODO: initialize regexes with result from sat

  //for (auto i: bool_vector) {std::cout << i << std::endl;}

  bool result = solver_->is_sat(bool_vector); //failing
  //bool result = false;

  std::cout << "RESULT: " << result << std::endl;
  return result;
}
