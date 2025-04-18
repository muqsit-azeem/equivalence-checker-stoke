//
// Created by andrea on 4/9/25.
//

#ifndef EMPTY_H
#define EMPTY_H

#include <boost/filesystem/path_traits.hpp>
#include <boost/move/detail/meta_utils.hpp>

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

  private:
    bool empty;
    size_t number_;
  };
}
#endif //EMPTY_H
