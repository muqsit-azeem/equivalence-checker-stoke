//Copyright 2025 Andrea Zimovčáková
//
//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#include "smt_alignment_checker.h"

#include "../symstate/bitvector.h"
#include "../symstate/bool.h"
#include "../symstate/regs.h"
#include "../symstate//memory/flat.h"
#include "regexOperations/concatenationOperation.h"

using namespace stoke;
using namespace x64asm;

bool SmtAlignmentChecker::check( std::shared_ptr<Invariant> inv,
              const Cfg* target, const Cfg* rewrite,
              std::shared_ptr<Operation> p, std::shared_ptr<Operation> q,
              bool override_separate_stack,
              size_t start_1, size_t end_1,
              size_t start_2, size_t end_2,
              std::map<string, uint64_t>& star_map
              ) {


  bool separate_stack = separate_stack_ || override_separate_stack;

  SymState target_sym_state("target");
  FlatMemory target_memory(separate_stack);
  target_memory.set_parent(&target_sym_state);
  target_sym_state.memory = &target_memory;

  SymState rewrite_sym_state("rewrite");
  FlatMemory rewrite_memory(separate_stack);
  rewrite_memory.set_parent(&rewrite_sym_state);
  rewrite_sym_state.memory = &rewrite_memory;

  SymBitVector one = SymBitVector::constant(64, 1); //neutral value
  bool target_star_zero = true, rewrite_star_zero = true;

  if (start_1 == end_1 && start_2 == end_2) {
    target_star_zero = false; rewrite_star_zero = false;
  }

  ProcessingInfo target_processing_info, rewrite_processing_info;
  SymbolicInstructionProcessor::process_regex(&target_sym_state, p, target, target_processing_info, false,
                                          one,  SymRegs(16, 64), target_star_zero);
  SymbolicInstructionProcessor::process_regex(&rewrite_sym_state,q, rewrite, rewrite_processing_info, false,
                                          one,  SymRegs(16, 64), rewrite_star_zero);
  if (start_1 == end_1) {
    if (target_sym_state.constraints.size() != 0) {
      SymBool last = target_sym_state.constraints.back();
      target_sym_state.constraints.pop_back();
      target_sym_state.add_constraint(!last);
    }
  } else if (target_processing_info.prev_state_ends_with_jump) {
    target_sym_state.constraints.pop_back();
  }

  if (start_2 == end_2) {
    if (rewrite_sym_state.constraints.size() != 0) {
      SymBool last = rewrite_sym_state.constraints.back();
      rewrite_sym_state.constraints.pop_back();
      rewrite_sym_state.add_constraint(!last);
    }
  } else if (rewrite_processing_info.prev_state_ends_with_jump) {rewrite_sym_state.constraints.pop_back();}


  size_t number = 0;
  std::vector<SymBool> bool_vector;
  auto sym_bool = (*inv)(target_sym_state, rewrite_sym_state, number);

  for (auto cons : target_processing_info.star_constraints) {
    bool_vector.push_back(cons);
  }
  for (auto cons : rewrite_processing_info.star_constraints) {
    bool_vector.push_back(cons);
  }

  for (auto cons : target_sym_state.constraints) {
    bool_vector.push_back(cons);
  }
  for (auto cons : rewrite_sym_state.constraints) {
    bool_vector.push_back(cons);
  }


  /*vector<SymBool> memory_bool;
  auto mem_bool = SymBool::_true();
  std::cout << " Memory Constraints "<< std::endl;
  vector<SymBool> memory_constrains = target_memory.get_constraints();
  for (auto mem_constr : memory_constrains)
  {
    std::cout << mem_constr << std::endl << std::endl;
    mem_bool = mem_bool & mem_constr;
  }*/

  /*for (auto mem_constr : rewrite_memory.get_constraints())
  {
    //std::cout << mem_constr << std::endl;
    mem_bool = mem_bool & mem_constr;
  }*/

  //memory_bool.push_back(mem_bool);

  //sym_bool = sym_bool & mem_bool;

  sym_bool = sym_bool & (target_processing_info.number_of_bits_changed == rewrite_processing_info.number_of_bits_changed);
  bool_vector.push_back(sym_bool);
  //bool_vector.insert(bool_vector.end(), target_processing_info.memory_axioms.begin(), target_processing_info.memory_axioms.end());
  //bool_vector.insert(bool_vector.end(), rewrite_processing_info.memory_axioms.begin(), rewrite_processing_info.memory_axioms.end());


  std::vector<string> star_variables = target_processing_info.star_variable_names;
  star_variables.insert(star_variables.end(), rewrite_processing_info.star_variable_names.begin(), rewrite_processing_info.star_variable_names.end());
  bool result = smallestStar(star_variables, star_map,bool_vector);

  if (solver_->has_error()) {
    std::cout << solver_->get_error()<< std::endl;
    assert(false);
    std::string input; std::cin >> input;
  }

  return result;
}


bool SmtAlignmentChecker::smallestStar(std::vector<string> star_variables, std::map<string, uint64_t>& star_map, std::vector<SymBool>& bool_vector) {
  if (solver_->is_sat(bool_vector)) {
    for (auto star_ : star_variables) {
      star_map[star_] = solver_->get_model_bv(star_, 64).get_fixed_quad(0);
    }
  } else {return false;}

  for (auto star : star_variables) {
    bool min_found = false;
    while (!min_found) {
      uint64_t current_star = star_map[star];
      SymBitVector current_sym_star = SymBitVector::constant(64, current_star);
      SymBitVector symbolic_star = SymBitVector::var(64, star);
      SymBool star_bound = symbolic_star.s_lt(current_sym_star);
      bool_vector.push_back(star_bound);

      auto result = solver_->is_sat(bool_vector);
      if (result) {
        star_map[star] = solver_->get_model_bv(star, 64).get_fixed_quad(0);
      } else {
        if (solver_->has_error()) {
          return false;
        }
        bool_vector.pop_back();
        min_found = true;
      }
      bool_vector.pop_back();
    }
  }

  return true;
}