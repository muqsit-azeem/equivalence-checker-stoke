//
// Created by andrea on 3/26/25.
//

#ifndef STAROPERATION_H
#define STAROPERATION_H

#include <iostream>
#include <memory>
#include <vector>

#include "src/no_data_validator/operation.h"

namespace stoke {
class StarOperation : public Operation {
public:

  StarOperation(std::vector<std::shared_ptr<Operation>> subexpressions){
    static_star_count += 1;
    loop_variable_name = "star_vector_num_" + std::to_string(static_star_count);
    subexpressions_ = subexpressions;
    symbol_ = '*';
  }

  StarOperation(std::shared_ptr<Operation> subexpression)
  : StarOperation(std::vector<std::shared_ptr<Operation>>{subexpression}) {}

  bool equals(const Operation& other) const {
    if (!dynamic_cast<const StarOperation*>(&other)) { return false; }

    auto range = subexpressions_.size();
    auto other_subexpressions = other.getSubexpressions();

    if (range != other_subexpressions.size()) { return false; }

    for (size_t i = 0; i < range; i++) {
      if (!subexpressions_[i]->equals(*other_subexpressions[i])) { return false;}
    }
    return true;
  }

  std::string get_loop_var() {
    return loop_variable_name;
  }

private:

  std::string loop_variable_name;
  SymBitVector loop_count;

};
}

#endif //STAROPERATION_H
