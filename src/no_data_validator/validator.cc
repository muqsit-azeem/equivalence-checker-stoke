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
using Edge = stoke::ProgramAlignmentAutomata::Edge;

#define INPUT_STOP(X) { std::cout << "****************" << X << "****************" << std::endl; } // std::string input; std::cin >> input; }
#define SPACE() { std::cout << " " << std::endl;}

bool NoDataValidator::build_paa_for_alignment_predicate(std::shared_ptr<Invariant> inv, ProgramAlignmentAutomata& paa) {
  /*SymBitVector bit_1 = SymBitVector::constant(1, 5);
  SymBitVector bit_32 = SymBitVector::constant(32, 4);
  SymBitVector bit_64 = SymBitVector::constant(64, 8);

  std::cout << "overflow bit_1: " << bit_1 << " with size: " << bit_1.width() << std::endl;
  std::cout << "overflow bit_1 resized: " << bit_1.sign_extend(32)  << " with size: " << bit_1.sign_extend(32).width() << std::endl;

  auto temp_multiply = bit_32 * bit_64;
  std::cout << "bit_32 * bit_64: " <<  temp_multiply << " with size: " << temp_multiply.width() << std::endl;
  temp_multiply = bit_64 * bit_32 ;
  std::cout << " bit_64 * bit_32: " <<  temp_multiply << " with size: " << temp_multiply.width() << std::endl;

  auto temp_plus = bit_32 + bit_64;
  std::cout << "bit_32 + bit_64: " <<  temp_plus << " with size: " << temp_plus.width() << std::endl;
  temp_plus = bit_64 + bit_32;
  std::cout << "bit_64 + bit_32: " <<  temp_plus << " with size: " << temp_plus.width() << std::endl;

  auto temp_minus = bit_32 - bit_64;
  std::cout << "bit_32 - bit_64: " <<  temp_minus << " with size: " << temp_minus.width() << std::endl;
  temp_minus = bit_64 - bit_32;
  std::cout << "bit_64 - bit_32: " <<  temp_minus << " with size: " << temp_minus.width() << std::endl;

  std::cout << "bit_1: " << bit_1 << " with size: " << bit_1.width() << std::endl;
  std::cout << "bit_32: " << bit_32 << " with size: " << bit_32.width() << std::endl;
  std::cout << "bit_64: " << bit_64 << " with size: " << bit_64.width() << std::endl;
  return true;*/
  //printing_cfg();

  //std::map<std::tuple<State,State>, std::tuple<std::shared_ptr<Operation>, std::shared_ptr<Operation>>> T; // no support for multiple paths from one state to other

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

  /*
  std::cout << "Pairs: " << std::endl;
  for (auto i : S) {
    std::cout << "(" << i.first << ", " << i.second << "), ";
  }
  std::cout << std::endl;
  */

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
      INPUT_STOP("NEW PAIRING")
      std::cout << "qi_1: "<< state.first << "    qj_1: " << state.second << std::endl;
      std::cout << "qi_2: "<< pair.first << "    qj_2: " << pair.second << std::endl;
      size_t qi_2 = pair.first;
      size_t qj_2 = pair.second;
      RegEx target_Regex(target_);
      RegEx rewrite_Regex(rewrite_);
      std::shared_ptr<Operation> target_regex;
      std::shared_ptr<Operation> rewrite_regex;

      if (!target_Regex.getRegex(qi_1, qi_2, target_regex) ||
          !rewrite_Regex.getRegex(qj_1, qj_2, rewrite_regex)) {
        std::cout << "FALSE 01" << std::endl;
        //if (qi_1 == 0 && qj_1 == 5 && qi_2 == 0 && qj_2 == 5) { std::string input; std::cin >> input; }
        continue;
      }


      //continue; //DEBUG

      if (qi_1 == qi_2 && qj_1 == qj_2 && (target_regex->isEmpty() || rewrite_regex->isEmpty())) {
        std::cout << "FALSE 02" << std::endl;
        //if (qi_1 == 0 && qj_1 == 5 && qi_2 == 0 && qj_2 == 5) { std::string input; std::cin >> input; }
        continue;
      }

      //std::cout << std::endl;
      //continue;

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
          std::cout << "r_i: " << *r_i << "   r_j: " << *r_j << std::endl;

          //std::shared_ptr<Operation> rc_i, rc_j; // may not be needed, if SAT initialized in check
          if (alignment_checker_->check(inv, &target_, &rewrite_, r_i, r_j, false, qi_1, qi_2, qj_1, qj_2)) {
            State from_state(qi_1, qj_1);
            State to_state(qi_2, qj_2);

            CfgPath target_path;
            CfgPath rewrite_path;
            std::cout << "BEFORE" << std::endl;

            target_Regex.get_CfgPath(target_path, r_i, *solver_);
            rewrite_Regex.get_CfgPath(rewrite_path, r_j, *solver_);

            //if (!target_path.empty()) { target_path.pop_back();}
            //if (!rewrite_path.empty()) { rewrite_path.pop_back(); } // CfgPath doesn't contain to State but regex does TODO:fix

            Edge edge(to_state, target_path, rewrite_path);
            edge.from = from_state;
            //if (qi_1 == 0 && qj_1 == 5 && edge.from.ts == 0 && edge.from.rs == 5)
            //{
              //std::cout << "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE" << std::endl;
              //std::string input; std::cin >> input;
            //}
            //edge.from = from_state;
            //add_edge_to_paa(edge, paa);
            //if (qi_1 == 3 && qj_1 == 4 && qi_2 == 3 && qj_2 == 4) { std::string input; std::cin >> input; }
            paa.add_edge(edge); // should substitute T

            //std::cout << "start" << from_state << std::endl;
            //std::cout << "end" << to_state << std::endl;
            //std::cout << edge << std::endl;
            //std::cout << "********************************" << std::endl; std::string input; std::cin >> input;

            //T[std::make_pair(from_state, to_state)] = std::make_pair(r_i, r_j);

            reach.push(std::make_pair((size_t) qi_2, qj_2));

          }
          //std::string input; std::cin >> input;
        }
      }

      if (qi_1 == 3 && qj_1 == 3 && qi_2 == 3 && qj_2 == 5) { std::string input; std::cin >> input; }

    }
  }

  std::cout << "************PAA*********"<< std::endl;
  //
  //paa.simplify();
  //paa.remove_prefixes();
  paa.print_all();
  std::string input; std::cin >> input;
  //paa.remove_prefixes();
  //
  return true;
}

void simplify_paa(ProgramAlignmentAutomata& paa, std::set<std::pair<size_t, size_t>>& reach) {
  std::set<std::pair<size_t, size_t>> temp;

}

//void NoDataValidator::add_edge_to_paa(Edge& edge, ProgramAlignmentAutomata& paa) {}

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
      //SymbolicInstructionProcessor::process_instruction(&sym_state, &inst, false, false);
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
  std::string input; std::cin >> input;

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