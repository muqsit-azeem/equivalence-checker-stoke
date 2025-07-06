//
// Created by andrea on 3/26/25.
//

#ifndef PLUSOPERATION_H
#define PLUSOPERATION_H

#include <iostream>
#include <memory>
#include <vector>

#include "concatenationOperation.h"
#include "empty.h"
#include "starOperation.h"
#include "src/no_data_validator/operation.h"

namespace stoke {

  class PlusOperation : public Operation {
  public:
    PlusOperation(std::vector<std::shared_ptr<Operation>> subexpressions) {
      subexpressions_ = subexpressions;
      symbol_ = '+';
    }

    // prev_subexpresion + subexpresion
    void add_subexpression(std::shared_ptr<Operation> subexpression) {
      if (subexpression->isEmpty()) {return;}
      if (auto plus_operation = std::dynamic_pointer_cast<PlusOperation>(subexpression)) {
        subexpressions_.insert(subexpressions_.end(), plus_operation->subexpressions_.begin(), plus_operation->subexpressions_.end());
        return;
      }
      subexpressions_.push_back(subexpression);
    }

    // doesnt work for input of PlusOperation
    //TODO: refine to work for all cases
    void add_to_every_subexpression(std::shared_ptr<Operation> subexpression) {
      if (subexpression->isEmpty() || std::dynamic_pointer_cast<PlusOperation>(subexpression)) {return;}

      int i = -1;
      for (auto subex : subexpressions_) {
        i++;
        if (std::dynamic_pointer_cast<PlusOperation>(subex)) { // there should not be PlusOperation in PlusOperation
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
        if (auto casted_operation = std::dynamic_pointer_cast<StarOperation>(subex))
        {
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
  };
}

#endif //PLUSOPERATION_H
