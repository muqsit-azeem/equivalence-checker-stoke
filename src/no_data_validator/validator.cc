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

using namespace std;
using namespace std::chrono;
using namespace stoke;
using namespace x64asm;

#define INPUT_STOP(X) { std::cout << "****************" << X << "****************" << std::endl; std::string input; std::cin >> input; }
#define SPACE() { std::cout << " " << std::endl;}

bool NoDataValidator::build_paa_for_alignment_predicate(std::shared_ptr<Invariant> inv, ProgramAlignmentAutomata& paa) {
  //printing_cfg();

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

  std::vector<std::pair<size_t, size_t>> reach;
  std::set<std::pair<size_t, size_t>> visited;

  reach.push_back(std::make_pair((size_t) target_.get_entry(), rewrite_.get_entry()));
  size_t reach_size = 1;
  size_t i = 0;

  while (i<reach_size) {
    if (visited.count(reach[i]) > 0) { // if chosen state is already visited
      ++i;
      continue;
    }
    size_t qi_1 = reach[i].first;
    size_t qj_1 = reach[i].second;
    visited.insert(reach[i]);

    for (auto pair: S) {
      size_t qi_2 = pair.first;
      size_t qj_2 = pair.second;
      RegEx target_Regex(target_);
      RegEx rewrite_Regex(rewrite_);
      std::shared_ptr<Operation> target_regex;
      std::shared_ptr<Operation> rewrite_regex;

      INPUT_STOP(".getRegex")
      if (!target_Regex.getRegex(qi_1, qi_2, target_regex) || !rewrite_Regex.getRegex(qj_1, qj_2, rewrite_regex)) {
        std::cout << "FALSE" << std::endl;
        continue;
      }

      /*
      if (qi_1 == qi_2 && qj_1 == qj_2) {
        std::vector<std::shared_ptr<Operation>> exprs;
        exprs.push_back(std::make_shared<Symbol>());
        auto plusOp = std::make_shared<PlusOperation>(exprs);

        if (target_regex == plusOp || rewrite_regex == plusOp) {
          continue;
        }
      }
      */
      // TO DO: finish comparing the regexes and splitting


    }
    ++i;
  }
  return false;
}

void NoDataValidator::printing_cfg()
{
  auto target = rewrite_;
  cout << "********************" << "PRINTING CFG" << "********************" << endl;

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