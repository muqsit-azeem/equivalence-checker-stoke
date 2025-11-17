//
// Created by andrea on 6/9/25.
//

#include "smt_alignment_checker.h"

#include <tr1/memory>

#include "../symstate/bitvector.h"
#include "../symstate/bool.h"
#include "../symstate/regs.h"
#include "../symstate//memory/flat.h"
#include "cvc4/smt/smt_engine.h"
#include "cvc4/util/bitvector.h"
#include "regexOperations/concatenationOperation.h"
#include "src/validator/invariants/conjunction.h"

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
  std::cout << "start_1: " << start_1 << " end_1 " << end_1 << std::endl;
  std::cout << "start_2: " << start_2 << " end_2 " << end_2 << std::endl;

  /*if (start_1 == end_1 && !p->isEmpty() && start_2 != end_2) {
    auto vector = p->getSubexpressions();
    vector.pop_back();
    p = make_shared<ConcatenationOperation>(vector);
  }
  if (start_1 != end_1 && !q->isEmpty() && start_2 == end_2) {
    auto vector = q->getSubexpressions();
    vector.pop_back();
    q = make_shared<ConcatenationOperation>(vector);
  }
  */

  std::cout << std::endl << "p = " << *p << " q: " << *q << std::endl;

  bool separate_stack = separate_stack_ || override_separate_stack;

  SymState target_sym_state("target");
  FlatMemory target_memory(separate_stack);
  target_memory.set_parent(&target_sym_state);
  target_sym_state.memory = &target_memory;


  SymState rewrite_sym_state("rewrite");
  FlatMemory rewrite_memory(separate_stack);
  rewrite_memory.set_parent(&rewrite_sym_state);
  rewrite_sym_state.memory = &rewrite_memory;

  std::cout << " PROCESSING REGEXES "<< std::endl;
  SymBitVector one = SymBitVector::constant(64, 1); //neutral value

  bool target_star_zero = true, rewrite_star_zero = true;

  if (start_1 == end_1 && start_2 == end_2) {
    target_star_zero = false; rewrite_star_zero = false;
  }
  /*
  if (start_1 != end_1 && dynamic_pointer_cast<StarOperation>(q) && start_2 == end_2) {
    rewrite_star_zero = true;
  }
  */

  ProcessingInfo target_processing_info, rewrite_processing_info;
  SymbolicInstructionProcessor::process_regex(&target_sym_state, p, target, target_processing_info, false,
                                          one,  SymRegs(16, 64), target_star_zero);
  SymbolicInstructionProcessor::process_regex(&rewrite_sym_state,q, rewrite, rewrite_processing_info, false,
                                          one,  SymRegs(16, 64), rewrite_star_zero);
  if (start_1 == end_1) {
    if (target_sym_state.constraints.size() != 0)
    {
      SymBool last = target_sym_state.constraints.back();
      target_sym_state.constraints.pop_back();
      target_sym_state.add_constraint(!last);
    }
  } else if (target_processing_info.prev_state_ends_with_jump) {
    target_sym_state.constraints.pop_back();
  }

  if (start_2 == end_2) {
    if (rewrite_sym_state.constraints.size() != 0)
    {
      SymBool last = rewrite_sym_state.constraints.back();
      rewrite_sym_state.constraints.pop_back();
      rewrite_sym_state.add_constraint(!last);
    }
  } else if (rewrite_processing_info.prev_state_ends_with_jump) {rewrite_sym_state.constraints.pop_back();}


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
  //auto sym_bool = SymBool::_true();
  auto sym_bool = (*inv)(target_sym_state, rewrite_sym_state, number);

  for (auto cons : target_processing_info.star_constraints)
  {
    std::cout << "Star CONSTRAINT "<< cons <<std::endl;
    //sym_bool = sym_bool & cons;
    bool_vector.push_back(cons);
  }
  for (auto cons : rewrite_processing_info.star_constraints)
  {
    std::cout << "Star CONSTRAINT "<< cons <<std::endl;
    //sym_bool = sym_bool & cons;
    bool_vector.push_back(cons);
  }

  for (auto cons : target_sym_state.constraints)
  {
    std::cout << " CONSTRAINT "<< cons <<std::endl;
    //sym_bool = sym_bool & cons;
    bool_vector.push_back(cons);
  }
  for (auto cons : rewrite_sym_state.constraints)
  {
    std::cout << " CONSTRAINT "<< cons <<std::endl;
    //sym_bool = sym_bool & cons;
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

  if (start_1 == 0 && end_1 == 0 && start_2 == 0 && end_2 == 5)
  {
    std::vector<string> star_variables = target_processing_info.star_variable_names;
    star_variables.insert(star_variables.end(), rewrite_processing_info.star_variable_names.begin(), rewrite_processing_info.star_variable_names.end());
    bool result = smallestStar(star_variables, star_map,bool_vector);
    std::cout << "RESULT: " << result << std::endl;
    std::cout << "ERROR: " << solver_->has_error() << std::endl;
    if (solver_->has_error())
    {
      std::cout << solver_->get_error()<< std::endl;
    }
    std::string input; std::cin >> input;

    //std::cout << " 33, 35  LOOP TRY"<< std::endl;
    //std::string input; std::cin >> input;
    //SymBitVector variable = SymBitVector::var(64, "star_vector_num_487");
    //SymBool bool_bool = (variable == SymBitVector::constant(64, 0));
    //bool_vector.push_back(bool_bool);

    /*std::vector<SymBool> bool_vec;
    SymBool bool_bool = target_processing_info.memory_axioms[0];//SymBool::_true() & target_sym_state.constraints[1];
    bool_vec.push_back(bool_bool);
    std::cout << "bool_bool: " << bool_bool << std::endl;
    bool_bool = rewrite_processing_info.memory_axioms[0];//SymBool::_true() & target_sym_state.constraints[1];
    bool_vec.push_back(bool_bool);
    std::cout << "bool_bool: " << bool_bool << std::endl;
    bool_vec.push_back(sym_bool);

    bool result = solver_->is_sat(bool_vec);

    std::cout << "RESULT: " << result << std::endl;
    return result;*/
  }

  std::cout << "SYM Bool: " << std::endl;
  std::cout << sym_bool << std::endl;

  //\std::cout << "MEM Bool: " << std::endl;
  //std::cout << mem_bool << std::endl;

  std::vector<string> star_variables = target_processing_info.star_variable_names;
  star_variables.insert(star_variables.end(), rewrite_processing_info.star_variable_names.begin(), rewrite_processing_info.star_variable_names.end());
  bool result = smallestStar(star_variables, star_map,bool_vector);

  std::cout << "RESULT: " << result << std::endl;
  std::cout << "ERROR: " << solver_->has_error() << std::endl;
  if (solver_->has_error()) {
    std::cout << solver_->get_error()<< std::endl;
    std::string input; std::cin >> input;
  }


  if (start_1 == 3 && end_1 == 3 && start_2 == 2 && end_2 == 4)
  {
    std::cout << " 03, 03  LOOP TRY"<< std::endl;
    for (auto i : bool_vector)
    {
      std::cout << i << std::endl;
    }
    for (auto i : star_map)
    {
      std::cout << i.first << ": " << i.second << std::endl;
    }
    std::string input; std::cin >> input;
  }

  return result;
}


bool SmtAlignmentChecker::smallestStar(std::vector<string> star_variables, std::map<string, uint64_t>& star_map, std::vector<SymBool>& bool_vector) {
  std::cout<< std::endl << "******************** Smallest STAR: ******************" << std::endl;
  for (auto bol: bool_vector) {
    std::cout << bol << std::endl;
  }

  if (solver_->is_sat(bool_vector)) {
    std::cout << "the solver found the answer right away" << std::endl;
    for (auto star_ : star_variables) {
      star_map[star_] = solver_->get_model_bv(star_, 64).get_fixed_quad(0);
    }
  } else {std::cout << "the solver failed" << std::endl; return false;}

  // what if star is 0? maybe stop there
  for (auto star : star_variables) {
    std::cout << std::endl;
    std::cout << "Current sym star: " << star_map[star] << std::endl;
    bool min_found = false;
    while (!min_found) {
      uint64_t current_star = star_map[star];
      SymBitVector current_sym_star = SymBitVector::constant(64, current_star);
      SymBitVector symbolic_star = SymBitVector::var(64, star);
      SymBool star_bound = symbolic_star.s_lt(current_sym_star);
      //std::cout << "Current sym star: " << current_sym_star << std::endl;
      bool_vector.push_back(star_bound);
      auto result = solver_->is_sat(bool_vector);
      std::cout << "SMT result in smallest star: " << result << std::endl;
      if (result) {
        star_map[star] = solver_->get_model_bv(star, 64).get_fixed_quad(0);
      } else {
        if (solver_->has_error()) {
          std::cout << "ERROR: " << solver_->get_error()<< std::endl;
          std::string input; std::cin >> input;
          return false;
        }
        for (auto i : bool_vector)
        {
          std::cout << "i: " << i << std::endl;
        }
        std::cout << "Got canceled" <<std::endl;
        bool_vector.pop_back();
        min_found = true;
      }
      bool_vector.pop_back();
    }
  }

  return true;
}