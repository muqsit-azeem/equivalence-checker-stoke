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

#include "starOperation.h"
#include "empty.h"
#include "plusOperation.h"
#include "concatenationOperation.h"

using namespace std;
using namespace stoke;

std::shared_ptr<Operation> ConcatenationOperation::connect_regex(std::shared_ptr<Operation> operation) {
  if (operation->getType() == OperationEnum::EMPTY) {
    return std::make_shared<ConcatenationOperation>(subexpressions_);
  }
  if (operation->getType() == OperationEnum::SYMBOL || operation->getType() == OperationEnum::CONCATENATION || operation->getType() == OperationEnum::STAR) {
    auto ret_op = std::make_shared<ConcatenationOperation>(subexpressions_);
    ret_op->add_subexpression(operation);
    return ret_op;
  }
  if (operation->getType() == OperationEnum::PLUS) {
    std::vector<std::shared_ptr<Operation>> plus_vector;
    for (auto op : operation->getSubexpressions()) {
      auto connected = std::make_shared<ConcatenationOperation>(subexpressions_);
      connected->add_subexpression(op);
      plus_vector.push_back(connected);
    }

    auto ret_op = std::make_shared<PlusOperation>(plus_vector);
    return ret_op;
  }
  return operation;
}