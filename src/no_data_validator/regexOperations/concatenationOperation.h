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

  void add_subexpression(std::shared_ptr<Operation> subexpression) {
    subexpressions_.push_back(subexpression);
  }

  std::vector<std::shared_ptr<Operation>> get_subexpressions() {
    return subexpressions_;
  }

};
}

#endif //CONCATENATIONOPERATION_H
