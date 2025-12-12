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

#ifndef PLUSOPERATION_H
#define PLUSOPERATION_H

#include <iostream>
#include <memory>
#include <vector>

#include "src/no_data_validator/operation.h"
#include "concatenationOperation.h"
#include "starOperation.h"
#include "empty.h"

namespace stoke {

  class PlusOperation : public Operation {
  public:
    PlusOperation(std::vector<std::shared_ptr<Operation>> subexpressions) {
      subexpressions_ = subexpressions;
      symbol_ = '+';
    }

    PlusOperation(std::shared_ptr<Operation> subexpression)
  : PlusOperation(std::vector<std::shared_ptr<Operation>>{subexpression}) {}

    // prev_subexpresion + subexpresion
    void add_subexpression(std::shared_ptr<Operation> subexpression) {
      if (subexpression->isEmpty()) {return;}
      if (auto plus_operation = std::dynamic_pointer_cast<PlusOperation>(subexpression)) {
        subexpressions_.insert(subexpressions_.end(), plus_operation->subexpressions_.begin(), plus_operation->subexpressions_.end());
        return;
      }
      subexpressions_.push_back(subexpression);
    }

    // prev_subex_1.subexpression + prev_subex_2.subexpression, doesn't work for input of PlusOperation
    //TODO: refine to work for all cases
    void add_to_every_subexpression(std::shared_ptr<Operation> subexpression) {
      if (subexpression->isEmpty() || subexpression->getType() == OperationEnum::PLUS) {return;}

      int i = -1;
      for (auto subex : subexpressions_) {
        i++;
        if (subex->getType() == OperationEnum::PLUS) { // there should not be PlusOperation in PlusOperation
          assert(false);
          return;
        }
        if (auto casted_operation = std::dynamic_pointer_cast<ConcatenationOperation>(subex)) {
          std::vector<std::shared_ptr<Operation>> concat_vector (casted_operation->getSubexpressions());
          std::shared_ptr<ConcatenationOperation> concat_operation = std::make_shared<ConcatenationOperation>(concat_vector);
          concat_operation->add_subexpression(subexpression);
          subexpressions_[i] = concat_operation;
          continue;
        }
        if (std::dynamic_pointer_cast<Symbol>(subex)) {
          if (subex->isEmpty()) {
            subexpressions_[i] = subexpression;
          } else {
            std::vector<std::shared_ptr<Operation>> concat_vector;
            concat_vector.push_back(subex);
            std::shared_ptr<ConcatenationOperation> concat_operation = std::make_shared<ConcatenationOperation>(concat_vector);
            concat_operation->add_subexpression(subexpression);
            subexpressions_[i] = concat_operation;
          }
          continue;
        }
        if (auto casted_operation = std::dynamic_pointer_cast<StarOperation>(subex)) {
          std::vector<std::shared_ptr<Operation>> concat_vector;
          concat_vector.push_back(casted_operation);
          if (std::dynamic_pointer_cast<ConcatenationOperation>(subexpression)) {
            concat_vector.insert(concat_vector.end(), subexpression->getSubexpressions().begin(), subexpression->getSubexpressions().end());
          } else { // either Star or Symbol
            concat_vector.push_back(subexpression);
          }
          std::shared_ptr<ConcatenationOperation> concat_operation = std::make_shared<ConcatenationOperation>(concat_vector);
          subexpressions_[i] = concat_operation;
          continue;
        }
      }
    }

    bool equals(const Operation& other) const {
      if (other.getType() != OperationEnum::PLUS) { return false; }

      auto range = subexpressions_.size();
      auto other_subexpressions = other.getSubexpressions();

      if (range != other_subexpressions.size()) { return false; }

      for (size_t i = 0; i < range; i++) {
        if (!subexpressions_[i]->equals(*other_subexpressions[i])) { return false;}
      }
      return true;
    }

    OperationEnum getType() const {
      return OperationEnum::PLUS;
    }

    std::shared_ptr<Operation> connect_regex(std::shared_ptr<Operation> operation);
  };
}

#endif //PLUSOPERATION_H
