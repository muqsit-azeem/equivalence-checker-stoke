//
// Created by andrea on 3/26/25.
//

#ifndef OPERATION_H
#define OPERATION_H

#include <vector>
#include <ostream>

#include "src/cfg/sccs.h"
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
