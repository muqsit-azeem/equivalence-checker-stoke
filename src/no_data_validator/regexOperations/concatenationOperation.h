//
// Created by andrea on 3/26/25.
//

#ifndef CONCATENATIONOPERATION_H
#define CONCATENATIONOPERATION_H

#include <iostream>
#include <memory>
#include <vector>

#include "src/no_data_validator/operation.h"

namespace stoke {

class ConcatenationOperation : public Operation {
public:
  ConcatenationOperation(std::vector<std::shared_ptr<Operation>> subexpressions) {
    subexpressions_ = subexpressions;
    symbol_ = '.';
  }

  // prev_subexpresion . subexpresion
  void add_subexpression(std::shared_ptr<Operation> subexpression) {
    if (subexpression->isEmpty()) {return;}
    if (auto concat_operation = std::dynamic_pointer_cast<ConcatenationOperation>(subexpression)) {
      subexpressions_.insert(subexpressions_.end(), concat_operation->subexpressions_.begin(), concat_operation->subexpressions_.end());
      return;
    }
    subexpressions_.push_back(subexpression);
  }

  std::vector<std::shared_ptr<Operation>> get_subexpressions() {
    return subexpressions_;
  }

};
}

#endif //CONCATENATIONOPERATION_H
