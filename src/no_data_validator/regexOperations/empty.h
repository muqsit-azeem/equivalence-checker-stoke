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

#ifndef EMPTY_H
#define EMPTY_H

#include "src/no_data_validator/operation.h"

namespace stoke {

  class Symbol : public Operation {
  public:
    Symbol() {
      empty = true;
      symbol_ = 'E';
    }

    Symbol(size_t symbol) {
      empty = false;
      number_ = symbol;
    }

    bool isEmpty() const override{
      return empty;
    }

    size_t getNumber() const{
      return number_;
    }

    bool equals(const Operation& other) const {
      const Symbol* symbol = dynamic_cast<const Symbol*>(&other);
      if (!symbol) { return false; }

      if (empty and symbol->isEmpty()) { return true; }
      if (empty or symbol->isEmpty() or number_ != symbol->getNumber()) { return false; }

      return true;
    }

    OperationEnum getType() const {
      return empty ? OperationEnum::EMPTY : OperationEnum::SYMBOL;
    }

    std::shared_ptr<Operation> connect_regex(std::shared_ptr<Operation> operation);

  private:
    bool empty;
    size_t number_;
  };
}
#endif //EMPTY_H
