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

#ifndef NO_DATA_SMT_ALIGNMENT_CHECKER_H
#define NO_DATA_SMT_ALIGNMENT_CHECKER_H


#include "operation.h"
#include "RegEx.h"
#include "symbolic_instruction_processor.h"
#include "../cfg/cfg.h"
#include "../solver/smtsolver.h"
#include "../validator/invariant.h"
#include "../validator/obligation_checker.h"

namespace stoke {
class SmtAlignmentChecker {
public:
  using Result = ObligationChecker::Result;


  SmtAlignmentChecker(SMTSolver* solver) :
  solver_(solver)
  {
    separate_stack_ = false;
  }

  ~SmtAlignmentChecker() {}

  bool check( std::shared_ptr<Invariant> inv,
              const Cfg* target, const Cfg* rewrite,
              std::shared_ptr<Operation> p, std::shared_ptr<Operation> q,
              bool override_separate_stack,
              size_t start_1, size_t end_1,
              size_t start_2, size_t end_2,
              std::map<string, uint64_t>& star_map
              );

  /** Set whether we are going to use separate stack. */
  SmtAlignmentChecker& set_separate_stack(bool b) {
    separate_stack_ = b;
    return *this;
  }

private:
  bool separate_stack_;
  SMTSolver* solver_;

  bool smallestStar(std::vector<string> star_variables, std::map<string, uint64_t>& star_map, std::vector<SymBool>& bool_vector);
};

} // stoke

#endif //NO_DATA_SMT_ALIGNMENT_CHECKER_H
