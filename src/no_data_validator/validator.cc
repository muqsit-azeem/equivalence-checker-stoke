//
// Created by andrea on 3/18/25.
//
#include "validator.h"

#include "tools/io/state_diff.h"
#include "src/no_data_validator/RegEx.h"
#include "src/no_data_validator/validator.h"
#include "src/no_data_validator/operation.h"
#include "src/no_data_validator/regexOperations/plusOperation.h"
#include "src/no_data_validator/regexOperations/empty.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <set>
#include <queue>

#include "symbolic_instruction_processor.h"

using namespace std;
using namespace std::chrono;
using namespace stoke;
using namespace x64asm;
using State = stoke::ProgramAlignmentAutomata::State;

#define INPUT_STOP(X) { std::cout << "****************" << X << "****************" << std::endl; std::string input; std::cin >> input; }
#define SPACE() { std::cout << " " << std::endl;}

bool NoDataValidator::build_paa_for_alignment_predicate(std::shared_ptr<Invariant> inv, ProgramAlignmentAutomata& paa) {
  //printing_cfg();
  std::map<std::tuple<State,State>, std::tuple<std::shared_ptr<Operation>, std::shared_ptr<Operation>>> T;

  auto S_1 = target_.reachable_begin();
  size_t num_s_1 = target_.num_reachable();
  auto S_2 = rewrite_.reachable_begin();
  size_t num_s_2 = rewrite_.num_reachable();

  std::set<std::pair<size_t, size_t>> S;
  for (size_t i = 0; i < num_s_1; ++i) {
    for (size_t j = 0; j < num_s_2; ++j) {
      S.insert(std::make_pair(*S_1, *S_2));
      ++S_2;
    }
    ++S_1;
    S_2 = rewrite_.reachable_begin();
  }


  std::queue<std::pair<size_t, size_t>> reach;
  std::set<std::pair<size_t, size_t>> visited;

  reach.push(std::make_pair((size_t) target_.get_entry(), rewrite_.get_entry()));

  while (!reach.empty()) {
    std::pair<size_t, size_t> state = reach.front();
    reach.pop();

    if (visited.count(state) > 0) { // if chosen state is already visited
      continue;
    }

    size_t qi_1 = state.first;
    size_t qj_1 = state.second;
    visited.insert(state);

    for (auto pair: S) {
      size_t qi_2 = pair.first;
      size_t qj_2 = pair.second;
      RegEx target_Regex(target_);
      RegEx rewrite_Regex(rewrite_);
      std::shared_ptr<Operation> target_regex;
      std::shared_ptr<Operation> rewrite_regex;

      INPUT_STOP(".getRegex")
      if (!target_Regex.getRegex(qi_1, qi_2, target_regex, true) || !rewrite_Regex.getRegex(qj_1, qj_2, rewrite_regex, true)) {
        std::cout << "FALSE 01" << std::endl;
        continue;
      }

      if (qi_1 == qi_2 && qj_1 == qj_2 && (target_regex->isEmpty() || rewrite_regex->isEmpty())) {
        std::cout << "FALSE 02" << std::endl;
        continue;
      }

      //splitting
      std::vector<std::shared_ptr<Operation>> R_i, R_j;

      if (std::dynamic_pointer_cast<PlusOperation>(target_regex)) {
        R_i = target_regex->getSubexpressions();
      } else {
        R_i.push_back(target_regex);
      }

      if (std::dynamic_pointer_cast<PlusOperation>(rewrite_regex)) {
        R_j = rewrite_regex->getSubexpressions();
      } else {
        R_j.push_back(rewrite_regex);
      }

      //comparing
      for (auto r_i : R_i) {
        for (auto r_j : R_j) {
          std::shared_ptr<Operation> rc_i, rc_j; // may not be needed, if SAT initialized in check
          if (alignment_checker_->check(inv, &target_, &rewrite_, r_i, r_j, false)) {
            State from_state(qi_1, qj_1);
            State to_state(qi_2, qj_2);
            T[std::make_pair(from_state, to_state)] = std::make_pair(r_i, r_j);
            reach.push(std::make_pair((size_t) qi_2, qj_2));

          }
        }
      }

    }
  }
  return false;
}
/*
bool NoDataValidator::smt_solution(shared_ptr<Operation> r_i, shared_ptr<Operation> r_j) {
  // memory set up
  SymState target_sym_state;
  FlatMemory target_memory(false);
  target_sym_state.memory = &target_memory;

  SymState rewrite_sym_state;
  FlatMemory rewrite_memory(false);
  rewrite_sym_state.memory = &rewrite_memory;

  auto bool_vector = target_sym_state.equality_constraints(rewrite_sym_state);
  return solver_->is_sat(bool_vector);
}
*/

//debuging function
void NoDataValidator::printing_cfg() {
  auto target = target_;
  cout << "********************" << "PRINTING CFG" << "********************" << endl;

  cout << "Summary:" << endl;

  auto states = target.reachable_begin();
  SymState sym_state("test", true);
  FlatMemory sym_memory(false);
  sym_state.memory = &sym_memory;

  auto count = target.num_reachable();
  for (size_t _ = 0; _ < count; ++_)
  {
    cout << "For block: " << *states << endl;
    auto instruction = target.instr_begin(*states);
    while (instruction != target.instr_end(*states))
    {
      const x64asm::Instruction& inst = *instruction;
      cout << "Instruction: " <<*instruction << endl;
      std::cout << endl;
      std::cout << sym_state << endl;
      std::cout << std::endl;
      for (auto reg: sym_state.rf)
      {
        std::cout << reg << " ";
      }
      std::cout << endl;
      std::cout << std::endl;
      std::cout << "Instruction processing" << std::endl;
      /*std::cout << "  Must Read:    " << target.must_read_set(*instruction) << std::endl;
      std::cout << "  Must Write:   " << target.must_write_set(*instruction) << std::endl;
      std::cout << "  Must Undef:   " << target.must_undef_set(*instruction) << std::endl;
      std::cout << "  Maybe Read:   " << target.maybe_read_set(*instruction) << std::endl;
      std::cout << "  Maybe Write:  " << target.maybe_write_set(*instruction) << std::endl;
      std::cout << "  Maybe Undef:  " << target.maybe_undef_set(*instruction) << std::endl;*/
      SymbolicInstructionProcessor::process_instruction(&sym_state, &inst, false, false);
      std::cout << endl;
      std::cout << sym_state << endl;
      std::cout << std::endl;
      for (auto reg: sym_state.rf)
      {
        std::cout << reg << " ";
      }
      std::cout << endl;
      std::cout << std::endl;
      ++instruction;
    }
    ++states;
  }

  cout << "Target: " << endl;
  cout << target.get_code() << endl;

  auto S_1 = target.reachable_begin();
  size_t num_s_1 = target.num_reachable();

  SPACE()
  cout<< "number of blocks: " << num_s_1 << endl;
  cout << "Entry block: " << target.get_entry() <<endl;
  cout << "Exit block: " << target.get_exit() <<endl;

  SPACE()
  S_1 = target.reachable_begin();
  num_s_1 = target.num_reachable();
  cout << "All neighbors: " << endl;
  for (size_t i = 0; i < num_s_1; ++i) {
    auto neighbour = target.succ_begin(*S_1);
    size_t succ_size = target.succ_size(*S_1);
    cout << "neightours of " << *S_1 << " -> ";
    for (size_t j = 0; j < succ_size; ++j) {
      cout << *(neighbour++) << " ";
    }
    cout << endl;
    ++S_1;
  }

  SPACE()
  S_1 = target.reachable_begin();
  num_s_1 = target.num_reachable();
  cout << "Instructions: " << endl;
  for (size_t i = 0; i < num_s_1; ++i) {
    size_t num = target.num_instrs(*S_1);
    auto instruc = target.instr_begin(*S_1);
    cout << "instructions of " << *S_1 << endl;
    for (size_t j = 0; j < num; ++j)
    {
      cout << *(instruc++) << endl;
    }

    cout << endl;
    ++S_1;
  }

  SPACE()
  cout << "Fallthrough blocks: " << endl;
  S_1 = target.reachable_begin();
  num_s_1 = target.num_reachable();
  for (size_t i = 0; i < num_s_1; ++i) {
    cout << "Fallthrough of " << *S_1 << ": ";
    if (target.has_fallthrough_target(*S_1))
    {
      cout << target.fallthrough_target(*S_1);
    }
    else
    {
      cout << "(empty)";
    }
    cout << endl;
    ++S_1;
  }

  SPACE()
  cout << "Conditional blocks: " << endl;
  S_1 = target.reachable_begin();
  num_s_1 = target.num_reachable();
  for (size_t i = 0; i < num_s_1; ++i) {
    cout << "Conditional of " << *S_1 << ": ";
    if (target.has_conditional_target(*S_1))
    {
      cout << target.conditional_target(*S_1);
    }
    else
    {
      cout << "(empty)";
    }
    cout << endl;
    ++S_1;
  }

  INPUT_STOP("finished printing the CFG")
}