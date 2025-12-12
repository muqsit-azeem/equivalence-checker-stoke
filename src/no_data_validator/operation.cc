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

#include <fstream>

#include "operation.h"

#include "regexOperations/empty.h"
#include "regexOperations/concatenationOperation.h"
#include "regexOperations/starOperation.h"
#include "regexOperations/plusOperation.h"

using namespace std;

namespace stoke {
  size_t Operation::static_star_count = 0;
}

namespace std {
  std::ostream& operator<<(std::ostream& os, const stoke::Operation& op) {
    if (auto* plus = dynamic_cast<const stoke::PlusOperation*>(&op)) {
      os << "(";
      for (const auto& sub : plus->getSubexpressions()) {
        os << *sub << plus->getSymbol();
      }
      os << ")";
    } else if (auto* star = dynamic_cast<const stoke::StarOperation*>(&op)) {
      os << "(";
      os << *(star->getSubexpressions()[0]);
      os << ")" << star->getSymbol();
    } else if (auto* conca = dynamic_cast<const stoke::ConcatenationOperation*>(&op)) {
      os << "(";
      for (const auto& sub : conca->getSubexpressions()) {
        os << *sub << conca->getSymbol();
      }
      os << ")";
    } else if (auto* symbol = dynamic_cast<const stoke::Symbol*>(&op)) {
      if (symbol->isEmpty()) {
        os << op.getSymbol();
      } else {
        os << symbol->getNumber();
      }
    }
    return os;
  }
}