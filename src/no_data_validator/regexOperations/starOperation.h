//
// Created by andrea on 3/26/25.
//

#ifndef STAROPERATION_H
#define STAROPERATION_H

#include <iostream>
#include <memory>
#include <vector>

#include "../operation.h"

namespace stoke {

class StarOperation : public Operation {
public:

  StarOperation(std::vector<std::shared_ptr<Operation>> subexpressions){
    static_star_count += 1;
    loop_variable_name = "star_vector_num_" + std::to_string(static_star_count);
    plus_one_ = false;
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

  void set_plus_one(bool plus_one) {
    plus_one_ = plus_one;
  }

  bool is_plus_one() {
    return plus_one_;
  }

  OperationEnum getType() const {
    return OperationEnum::STAR;
  }

  std::shared_ptr<Operation> connect_regex(std::shared_ptr<Operation> operation);

private:

  std::string loop_variable_name;
  bool plus_one_;

};
}

#endif //STAROPERATION_H
