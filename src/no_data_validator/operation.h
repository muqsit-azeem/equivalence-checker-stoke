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

#ifndef OPERATION_H
#define OPERATION_H

#include <vector>

#include "operation_enum.h"
#include "src/validator/invariant.h"

namespace stoke {

  class Operation {
  public:
    Operation() {};
    virtual ~Operation() {};

    virtual bool isEmpty() const {
      return false;
    }

    std::vector<std::shared_ptr<Operation>> getSubexpressions() const {return subexpressions_;}

    char getSymbol() const {return symbol_;}

    virtual bool equals(const Operation& other) const = 0;
    virtual OperationEnum getType() const = 0;
    //connects regex such that splitting would be easy (if there is plus operation then it is the outermost operation)
    virtual std::shared_ptr<Operation> connect_regex(std::shared_ptr<Operation> operation) = 0;

  protected:
    static size_t static_star_count;
    char symbol_;
    std::vector<std::shared_ptr<Operation>> subexpressions_;
  };

}

namespace std {
  std::ostream& operator<<(std::ostream& os, const stoke::Operation&);
}

#endif //OPERATION_H
