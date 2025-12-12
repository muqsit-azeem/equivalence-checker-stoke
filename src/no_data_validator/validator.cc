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

#include "validator.h"
#include "tools/io/state_diff.h"
#include "src/no_data_validator/RegEx.h"
#include "src/no_data_validator/operation.h"
#include "src/no_data_validator/regexOperations/plusOperation.h"
#include "symbolic_instruction_processor.h"

#include <set>
#include <queue>

using namespace std;
using namespace stoke;
using namespace x64asm;
using State = stoke::ProgramAlignmentAutomata::State;
using Edge = stoke::ProgramAlignmentAutomata::Edge;

#define INPUT_STOP(X) { std::cout << "****************" << X << "****************" << std::endl; } // std::string input; std::cin >> input; }
#define SPACE() { std::cout << " " << std::endl;}

bool NoDataValidator::build_paa_for_alignment_predicate(std::shared_ptr<Invariant> inv, ProgramAlignmentAutomata& paa) {
  std::cout << "Building PAA" << std::endl;
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
      //std::cout << "qi_1: "<< state.first << "    qj_1: " << state.second << std::endl;
      //std::cout << "qi_2: "<< pair.first << "    qj_2: " << pair.second << std::endl;
      size_t qi_2 = pair.first;
      size_t qj_2 = pair.second;
      RegEx target_Regex(target_);
      RegEx rewrite_Regex(rewrite_);
      std::shared_ptr<Operation> target_regex;
      std::shared_ptr<Operation> rewrite_regex;

      if (!target_Regex.getRegex(qi_1, qi_2, target_regex) ||
          !rewrite_Regex.getRegex(qj_1, qj_2, rewrite_regex)) {
        continue;
      }


      if (qi_1 == qi_2 && qj_1 == qj_2 && (target_regex->isEmpty() || rewrite_regex->isEmpty())) {
        continue;
      }

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
          //std::cout << "r_i: " << *r_i << "   r_j: " << *r_j << std::endl;
          std::map<string, uint64_t> star_map;

          if (alignment_checker_->check(inv, &target_, &rewrite_, r_i, r_j, false, qi_1, qi_2, qj_1, qj_2, star_map)) {
            State from_state(qi_1, qj_1);
            State to_state(qi_2, qj_2);

            CfgPath target_path;
            CfgPath rewrite_path;

            target_Regex.get_CfgPath(target_path, r_i, star_map);
            rewrite_Regex.get_CfgPath(rewrite_path, r_j, star_map);

            Edge edge(to_state, target_path, rewrite_path);
            edge.from = from_state;

            paa.add_edge(edge);

            reach.push(std::make_pair((size_t) qi_2, qj_2));

          }
        }
      }
    }
  }

  paa.simplify();
  //paa.print_all();
  return true;
}

//debuging function
void NoDataValidator::printing_cfg() {
  auto target = rewrite_;
  cout << "********************" << "PRINTING CFG" << "********************" << endl;

  cout << "Summary:" << endl;

  auto states = target.reachable_begin();
  SymState sym_state("test", true);
  FlatMemory sym_memory(false);
  sym_state.memory = &sym_memory;

  auto count = target.num_reachable();
  while (states != target.reachable_end())
  {
    cout << "For block: " << *states << endl;
    auto instruction = target.instr_begin(*states);
    while (instruction != target.instr_end(*states))
    {
      const x64asm::Instruction& inst = *instruction;
      cout << "Instruction: " <<*instruction << endl;
      std::cout << endl;
      //std::cout << sym_state << endl;
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
      //SymbolicInstructionProcessor::process_instruction(&sym_state, &inst, false, false);
      std::cout << endl;
      //std::cout << sym_state << endl;
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
  //std::string input; std::cin >> input;

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