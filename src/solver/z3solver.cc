// Copyright 2013-2019 Stanford University
//
// Licensed under the Apache License, Version 2.0 (the License);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an AS IS BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <iostream>
#include <fstream>
#include <chrono>

#include "src/solver/z3solver.h"
#include "src/symstate/axiom_visitor.h"
#include "src/symstate/bitvector.h"
#include "src/symstate/typecheck_visitor.h"
#include "src/symstate/memo_visitor.h"
#include "src/symstate/visitor.h"
#include "src/validator/md5.h"

using namespace stoke;
using namespace z3;
using namespace std;
using namespace std::chrono;

//#define STOKE_Z3_DEBUG_LAST_HASH YESPLEASE
//#define DEBUG_Z3_PERFORMANCE 1
#define DEBUG_Z3(X) { if(0) { X } }

#ifdef DEBUG_Z3_INTERFACE_PERFORMANCE
uint64_t Z3Solver::number_queries_ = 0;
uint64_t Z3Solver::typecheck_time_ = 0;
uint64_t Z3Solver::convert_time_ = 0;
uint64_t Z3Solver::solver_time_ = 0;
#endif

vector<SymBool> split_constraints(const vector<SymBool>& constraints) {
  vector<SymBool> split;
  for (auto it : constraints) {
    if (it.type() == SymBool::AND) {
      auto val = static_cast<const SymBoolAnd*>(it.ptr);
      SymBool a(val->a_);
      SymBool b(val->b_);
      vector<SymBool> temp = {a, b};
      auto result = split_constraints(temp);
      split.insert(split.begin(), result.begin(), result.end());
    } else {
      split.push_back(it);
    }
  }
  return split;
}



bool Z3Solver::is_sat(const vector<SymBool>& constraints) {

#ifdef DEBUG_Z3_INTERFACE_PERFORMANCE
  number_queries_++;
#endif

  /* Reset state. */
  error_ = "";
  model_ = 0;
  stop_now_.store(false);
  solver_.reset();

  /** Get all the axioms we need. */
  SymAxiomVisitor av;
  for (auto it : constraints)
    av(it);
  auto all_constraints = av.get_axioms();
  all_constraints.insert(all_constraints.begin(), constraints.begin(), constraints.end());

  /* Convert constraints and query to z3 object */
  SymTypecheckVisitor tc;

  const vector<SymBool> split = split_constraints(all_constraints);
  const vector<SymBool>* current = &split;
  vector<SymBool>* new_constraints = 0;
  bool free_it = false;

  auto check_abort = [&]() -> bool {
    if (stop_now_) {
      error_ = "External interrupt.";
      return true;
    } else
      return false;
  };

  while (current->size() != 0) {
    if (check_abort()) return false;

    new_constraints = new vector<SymBool>();

    ExprConverter ec(context_, *new_constraints);

    for (auto it : *current) {
      if (check_abort()) return false;

#ifdef DEBUG_Z3_INTERFACE_PERFORMANCE
      number_queries_++;
      microseconds typecheck_start = duration_cast<microseconds>(system_clock::now().time_since_epoch());
#endif
      if (tc(it) != 1) {
        stringstream ss;
        ss << "Typechecking failed for constraint: " << it << endl;
        if (tc.has_error())
          ss << "error: " << tc.error() << endl;
        else
          ss << "(no typechecking error message given)" << endl;
        error_ = ss.str();
        return false;
      }
#ifdef DEBUG_Z3_INTERFACE_PERFORMANCE
      microseconds typecheck_end = duration_cast<microseconds>(system_clock::now().time_since_epoch());
      typecheck_time_ += (typecheck_end - typecheck_start).count();
#endif

      auto constraint = ec(it);
      if (ec.has_error()) {
        error_ = ec.error();
        return false;
      }

#ifdef DEBUG_Z3_INTERFACE_PERFORMANCE
      microseconds convert_end = duration_cast<microseconds>(system_clock::now().time_since_epoch());
      convert_time_ += (convert_end - typecheck_end).count();
#endif
      DEBUG_Z3(
        cout << it << endl;
        cout << constraint << endl;)
      solver_.add(constraint);
    }

    if (free_it)
      delete current;
    free_it = true;

    current = new_constraints;
  }
  delete current;

  /* Run the solver and see */
  try {
#if defined(DEBUG_Z3_INTERFACE_PERFORMANCE) || defined(DEBUG_Z3_PERFORMANCE)
    microseconds solver_start = duration_cast<microseconds>(system_clock::now().time_since_epoch());
#endif
    if (check_abort()) return false;

    DEBUG_Z3(
      static int debug_count = 0;
      stringstream ss;
      ss << "z3-smtlib-" << debug_count++;
      ofstream ofs(ss.str());
      string smtlib = solver_.to_smt2();
      ofs << smtlib << endl;
      ofs.close();)

#ifdef STOKE_Z3_DEBUG_LAST_HASH
    string smt = solver_.to_smt2();
    last_hash_ = md5(smt);
    last_text_ = smt;
#endif

    auto result = solver_.check();
#if defined(DEBUG_Z3_INTERFACE_PERFORMANCE) || defined(DEBUG_Z3_PERFORMANCE)
    microseconds solver_end = duration_cast<microseconds>(system_clock::now().time_since_epoch());
#endif
#ifdef DEBUG_Z3_PERFORMANCE
    cout << "[z3perf] " << (solver_end-solver_start).count()/1000 << "ms" << endl;
#endif
#ifdef DEBUG_Z3_INTERFACE_PERFORMANCE
    solver_time_ += (solver_end - solver_start).count();
#endif

    switch (result) {
    case unsat: {
      return false;
    }

    case sat: {
      if (model_ != NULL)
        delete model_;
      model_ = new z3::model(solver_.get_model());
      DEBUG_Z3(
        cout << "MODEL: " << endl;
        cout << *model_ << endl;
      )
      return true;
    }

    case unknown: {
      error_ = "z3 gave up.";
      return false;
    }

    default: {
      assert(false);
      return false;
    }
    }
  } catch (std::exception e) {
    std::stringstream ss;
    ss << "Z3 encountered error: " << e.what() << endl;
    error_ = ss.str();
    return false;
  }

  assert(false);
  return false;
}

/** Get the satisfying assignment for a bit-vector from the model.
    NOTE: This function is very brittle right now.  If you pass in the wrong
    variable/size, there's no way to know and the result you get back is
    undefined. */
cpputil::BitVector Z3Solver::get_model_bv(const std::string& var, uint16_t bits) {
  auto octs = (bits+63)/64;

  auto type = context_.bv_sort(bits);
  auto v = z3::expr(context_, Z3_mk_const(context_, get_symbol(var), type));

  cpputil::BitVector result(bits);

  for (int i = 0; i < octs; ++i) {
    uint64_t oct;

    size_t max_bits = i*64+63 > bits ? bits-1 : i*64+63;
    expr extract = to_expr(context_, Z3_mk_extract(context_, max_bits, i*64, v));
    expr number = to_expr(context_, Z3_mk_bv2int(context_, extract, true));
    expr eval = model_->eval(number, true);
    Z3_get_numeral_int64(context_, eval, (long long int*)&oct);

    assert((max_bits + 1) % 8 == 0);
    size_t k = 0;
    for (size_t j = i*8; j < (max_bits+1)/8; ++j) {
      result.get_fixed_byte(j) = (oct >> (k*8)) & 0xff;
      k++;
    }
  }

  assert(result.num_bits() == bits);

  return result;
}

/** Get the satisfying assignment for a bit from the model.
    NOTE: This function is very brittle right now.  If you pass in the wrong
    variable/size, there's no way to know and the result you get back is
    undefined. */
bool Z3Solver::get_model_bool(const std::string& var) {
  auto type = Z3_mk_bool_sort(context_);
  auto v = z3::expr(context_, Z3_mk_const(context_, get_symbol(var), type));

  expr e = model_->eval(v, true);
  int n = Z3_get_bool_value(context_, e);

  if (n == 1)
    return true;
  else if (n == -1)
    return false;
  else {
    //Error!  (We need a better way of handling this)
    error_ = "Z3 returned invalid value " + to_string(n) + " for boolean " + var + ".";
    return false;
  }
}


pair<map<uint64_t, cpputil::BitVector>, uint8_t> Z3Solver::get_model_array(
  const std::string& var, uint16_t key_bits, uint16_t value_bits) {

  map<uint64_t, cpputil::BitVector> addr_val_map;

  // get variable / value
  auto type = Z3_mk_array_sort(context_, context_.bv_sort(key_bits), context_.bv_sort(value_bits));
  auto v = z3::expr(context_, Z3_mk_const(context_, get_symbol(var), type));
  expr e = model_->eval(v, true);
  auto ast = e.decl();

  DEBUG_Z3(cout << "Expression for array model: " << e << endl;)
  //cout << *model_ << endl;

  // CREDIT: this was written with A LOT of help from
  // https://stackoverflow.com/questions/22885457/read-func-interp-of-a-z3-array-from-the-z3-model
  // AND
  // https://stackoverflow.com/questions/40903505/extract-value-from-const-array-in-z3

  auto kind = Z3_get_decl_kind(context_, ast);

  while (kind == Z3_OP_STORE) {
    z3::expr k = e.arg(1);
    z3::expr v = e.arg(2);
    uint64_t addr;
    uint64_t value;
    Z3_get_numeral_uint64(context_, k, (long long unsigned int*)&addr);
    Z3_get_numeral_uint64(context_, v, (long long unsigned int*)&value);

    assert(value <= 0xff);
    cpputil::BitVector bv_v(8);
    bv_v.get_fixed_byte(0) = value;
    addr_val_map[addr] = bv_v;
    DEBUG_Z3(cout << hex << "adding " << addr << "->" << value << endl;)

    e = e.arg(0);
    ast = e.decl();
    kind = Z3_get_decl_kind(context_, ast);

  }

  if (kind == Z3_OP_CONST_ARRAY) {

    z3::expr arg = e.arg(0);
    uint64_t value;
    Z3_get_numeral_uint64(context_, arg, (long long unsigned int*)&value);
    assert(value <= 0xff);

    pair<map<uint64_t, cpputil::BitVector>, uint8_t> result;
    result.first = addr_val_map;
    result.second = value;
    return result;

  } else if (kind == Z3_OP_AS_ARRAY) {
    auto z3_model_fd = Z3_get_decl_func_decl_parameter(context_, ast, 0);
    auto model_fd = func_decl(context_, z3_model_fd);
    func_interp fun_interp = model_->get_func_interp(model_fd);


    unsigned num_entries = fun_interp.num_entries();
    for (unsigned i = 0; i < num_entries; i++)
    {
      z3::func_entry entry = fun_interp.entry(i);
      z3::expr k = entry.arg(0);
      z3::expr v = entry.value();

      uint64_t addr;
      uint64_t value;
      Z3_get_numeral_uint64(context_, k, (long long unsigned int*)&addr);
      Z3_get_numeral_uint64(context_, v, (long long unsigned int*)&value);

      assert(value <= 0xff);

      cpputil::BitVector bv_v(8);
      bv_v.get_fixed_byte(0) = value;
      addr_val_map[addr] = bv_v;
      DEBUG_Z3(cout << hex << "adding " << addr << "->" << value << endl;)
    }

    z3::expr default_value = fun_interp.else_value();
    pair<map<uint64_t, cpputil::BitVector>, uint8_t> result;
    result.first = addr_val_map;
    result.second = default_value;
    return result;
  } else if (kind == Z3_OP_ARRAY_MAP) {
    cout << "[z3] Don't know how to handle Z3_OP_ARRAY_MAP" << endl;
  }

  // The counterexample could be spurious, but we'll figure that out later.
  // On the other hand, there might be no memory at all or the memory
  // does not matter
  cout << "[z3] Couldn't parse Z3's AST for array model; may have spurious CEG. " << endl;
  pair<map<uint64_t, cpputil::BitVector>, uint8_t> result(addr_val_map, 0);
  return result;
}

///////  The following is for converting bit-vectors.  Very tedious.  //////////////////////////////

/** Visit a bit-vector AND */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorAnd * const bv) {
  return z3::expr(context_, Z3_mk_bvand(context_, (*this)(bv->a_), (*this)(bv->b_)));
}

/** Visit a bit-vector concatenation */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorConcat * const bv) {
  return z3::expr(context_, Z3_mk_concat(context_, (*this)(bv->a_), (*this)(bv->b_)));
}

/** Visit a bit-vector constant */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorConstant * const bv) {
  return z3::expr(context_, context_.bv_val((long long unsigned int)bv->constant_, bv->size_));
}

/** Visit a bit-vector DIV */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorDiv * const bv) {
  return z3::expr(context_, Z3_mk_bvudiv(context_, (*this)(bv->a_), (*this)(bv->b_)));
}

/** Visit a bit-vector extract */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorExtract * const bv) {
  return z3::expr(context_, Z3_mk_extract(context_, bv->high_bit_, bv->low_bit_, (*this)(bv->bv_)));
}

/** Visit an uninterpreted function */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorFunction * const bv) {

  auto f = bv->f_;
  auto args = f.args;
  auto ret = f.return_type;
  char const * name = f.name.c_str();

  // get z3 representation of the argument/return types
  vector<z3::sort> sorts;
  for (uint16_t it : args) {
    sorts.push_back(context_.bv_sort(it));
  }

  z3::sort ret_sort = context_.bv_sort(ret);

  // create z3 function declaration
  switch (sorts.size()) {
  case 0:
    error_ = "Function " + f.name + " has no arguments: " + to_string(sorts.size());
    assert(false);
    break;

  case 1:
    return z3::function(name, sorts[0], ret_sort)(
             (*this)(bv->args_[0]));
    break;

  case 2:
    return z3::function(name, sorts[0], sorts[1], ret_sort)(
             (*this)(bv->args_[0]), (*this)(bv->args_[1]));
    break;

  case 3:
    return z3::function(name, sorts[0], sorts[1], sorts[2], ret_sort)(
             (*this)(bv->args_[0]), (*this)(bv->args_[1]), (*this)(bv->args_[2]));
    break;

  default:
    error_ = "Function " + f.name + " has too many arguments: " + to_string(sorts.size());
    assert(false);
    break;
  }

  assert(false);
  return z3::function(name, sorts[0], ret_sort)(
           (*this)(bv->args_[0])); //keep the compiler happy
}

/** Visit a bit-vector if-then-else */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorIte * const bv) {
  return z3::expr(context_, Z3_mk_ite(context_, (*this)(bv->cond_), (*this)(bv->a_), (*this)(bv->b_)));
}

/** Visit a bit-vector minus */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorMinus * const bv) {
  return z3::expr(context_, Z3_mk_bvsub(context_, (*this)(bv->a_), (*this)(bv->b_)));
}

/** Visit a bit-vector mod */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorMod * const bv) {
  return z3::expr(context_, Z3_mk_bvurem(context_, (*this)(bv->a_), (*this)(bv->b_)));
}

/** Visit a bit-vector mult */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorMult * const bv) {
  return z3::expr(context_, Z3_mk_bvmul(context_, (*this)(bv->a_), (*this)(bv->b_)));
}

/** Visit a bit-vector NOT */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorNot * const bv) {
  return z3::expr(context_, Z3_mk_bvnot(context_, (*this)(bv->bv_)));
}

/** Visit a bit-vector OR */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorOr * const bv) {
  return z3::expr(context_, Z3_mk_bvor(context_, (*this)(bv->a_), (*this)(bv->b_)));
}

/** Visit a bit-vector plus */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorPlus * const bv) {
  return z3::expr(context_, Z3_mk_bvadd(context_, (*this)(bv->a_), (*this)(bv->b_)));
}

/** Visit a bit-vector rotate-left */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorRotateLeft * const bv) {
  return z3::expr(context_, Z3_mk_ext_rotate_left(context_, (*this)(bv->a_), (*this)(bv->b_)));
}

/** Visit a bit-vector rotate-right */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorRotateRight * const bv) {
  return z3::expr(context_, Z3_mk_ext_rotate_right(context_, (*this)(bv->a_), (*this)(bv->b_)));
}

/** Visit a bit-vector shift-left */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorShiftLeft * const bv) {
  return z3::expr(context_, Z3_mk_bvshl(context_, (*this)(bv->a_), (*this)(bv->b_)));
}

/** Visit a bit-vector shift-right */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorShiftRight * const bv) {
  return z3::expr(context_, Z3_mk_bvlshr(context_, (*this)(bv->a_), (*this)(bv->b_)));
}

/** Visit a bit-vector signed divide */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorSignDiv * const bv) {
  // assert second arg non-zero
  auto arg = SymBitVector(bv->b_);
  auto width = arg.width();
  auto zero = SymBitVector::constant(width, 0);
  auto constraint = arg != zero;
  constraints_.push_back(constraint);

  return z3::expr(context_, Z3_mk_bvsdiv(context_, (*this)(bv->a_), (*this)(bv->b_)));

}

/** Visit a bit-vector sign extension */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorSignExtend * const bv) {

  auto child = bv->bv_->width_;

  return z3::expr(context_, Z3_mk_sign_ext(context_, bv->size_ - child, (*this)(bv->bv_)));
}

/** Visit a bit-vector signed mod */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorSignMod * const bv) {
  return z3::expr(context_, Z3_mk_bvsrem(context_, (*this)(bv->a_), (*this)(bv->b_)));
}

/** Visit a bit-vector signed shift-right */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorSignShiftRight * const bv) {
  return z3::expr(context_, Z3_mk_bvashr(context_, (*this)(bv->a_), (*this)(bv->b_)));
}

/** Visit a bit-vector unary minus */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorUMinus * const bv) {
  return z3::expr(context_, Z3_mk_bvneg(context_, (*this)(bv->bv_)));
}

/** Visit a bit-vector variable */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorVar * const bv) {
  return context_.bv_const(bv->name_.c_str(), bv->size_);
}

/** Visit a bit-vector array access */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorArrayLookup * const bv) {
  return z3::expr(context_, Z3_mk_select(context_, (*this)(bv->a_), (*this)(bv->key_)));
}

/** Visit a bit-vector XOR */
z3::expr Z3Solver::ExprConverter::visit(const SymBitVectorXor * const bv) {
  return z3::expr(context_, Z3_mk_bvxor(context_, (*this)(bv->a_), (*this)(bv->b_)));
}

/** Visit a bit-vector ARRAY_EQ */
z3::expr Z3Solver::ExprConverter::visit(const SymBoolArrayEq * const b) {
  return z3::expr(context_, Z3_mk_eq(context_, (*this)(b->a_), (*this)(b->b_)));
}

/** Visit a bit-vector EQ */
z3::expr Z3Solver::ExprConverter::visit(const SymBoolEq * const b) {
  return z3::expr(context_, Z3_mk_eq(context_, (*this)(b->a_), (*this)(b->b_)));
}

/** Visit a bit-vector GE */
z3::expr Z3Solver::ExprConverter::visit(const SymBoolGe * const b) {
  return z3::expr(context_, Z3_mk_bvuge(context_, (*this)(b->a_), (*this)(b->b_)));
}

/** Visit a bit-vector GT */
z3::expr Z3Solver::ExprConverter::visit(const SymBoolGt * const b) {
  return z3::expr(context_, Z3_mk_bvugt(context_, (*this)(b->a_), (*this)(b->b_)));
}

/** Visit a bit-vector LE */
z3::expr Z3Solver::ExprConverter::visit(const SymBoolLe * const b) {
  return z3::expr(context_, Z3_mk_bvule(context_, (*this)(b->a_), (*this)(b->b_)));
}

/** Visit a bit-vector LT */
z3::expr Z3Solver::ExprConverter::visit(const SymBoolLt * const b) {
  return z3::expr(context_, Z3_mk_bvult(context_, (*this)(b->a_), (*this)(b->b_)));
}

/** Visit a bit-vector signed GE */
z3::expr Z3Solver::ExprConverter::visit(const SymBoolSignGe * const b) {
  return z3::expr(context_, Z3_mk_bvsge(context_, (*this)(b->a_), (*this)(b->b_)));
}

/** Visit a bit-vector signed GT */
z3::expr Z3Solver::ExprConverter::visit(const SymBoolSignGt * const b) {
  return z3::expr(context_, Z3_mk_bvsgt(context_, (*this)(b->a_), (*this)(b->b_)));
}

/** Visit a bit-vector signed LE */
z3::expr Z3Solver::ExprConverter::visit(const SymBoolSignLe * const b) {
  return z3::expr(context_, Z3_mk_bvsle(context_, (*this)(b->a_), (*this)(b->b_)));
}

/** Visit a bit-vector signed LT */
z3::expr Z3Solver::ExprConverter::visit(const SymBoolSignLt * const b) {
  return z3::expr(context_, Z3_mk_bvslt(context_, (*this)(b->a_), (*this)(b->b_)));
}

/** Visit a boolean AND */
z3::expr Z3Solver::ExprConverter::visit(const SymBoolAnd * const b) {
  return (*this)(b->a_) && (*this)(b->b_);
}

/** Visit a boolean FALSE */
z3::expr Z3Solver::ExprConverter::visit(const SymBoolFalse * const b) {
  return z3::expr(context_, Z3_mk_false(context_));
}

/** Visit a boolean FOR_ALL */
z3::expr Z3Solver::ExprConverter::visit(const SymBoolForAll * const b) {

  /** Get expressions for variable */
  auto vars = b->vars_;
  vector<expr> exprs;
  for (auto v : vars) {
    exprs.push_back(context_.bv_const(v.name_.c_str(), v.size_));
  }

  /** Get expression for child formula */
  auto inside = (*this)(b->a_);

  /** Find a method that works... */
  if (vars.size() == 1) {
    return z3::forall(exprs[0], inside);
  } else if (vars.size() == 2) {
    return z3::forall(exprs[0], exprs[1], inside);
  } else if (vars.size() == 3) {
    return z3::forall(exprs[0], exprs[1], exprs[2], inside);
  } else {
    // NOTE: this code doesn't work
    assert(false);
    expr_vector vect(context_);
    for (auto e : exprs) {
      vect.push_back(e);
    }
    return z3::forall(vect, inside);
  }
}

/** Visit a boolean IFF */
z3::expr Z3Solver::ExprConverter::visit(const SymBoolIff * const b) {
  return z3::expr(context_, Z3_mk_eq(context_, (*this)(b->a_), (*this)(b->b_)));
}

/** Visit a boolean implies */
z3::expr Z3Solver::ExprConverter::visit(const SymBoolImplies * const b) {
  return z3::expr(context_, Z3_mk_implies(context_, (*this)(b->a_), (*this)(b->b_)));
}

/** Visit a boolean NOT */
z3::expr Z3Solver::ExprConverter::visit(const SymBoolNot * const b) {
  return z3::expr(context_, Z3_mk_not(context_, (*this)(b->b_)));
}

/** Visit a boolean OR */
z3::expr Z3Solver::ExprConverter::visit(const SymBoolOr * const b) {
  return (*this)(b->a_) || (*this)(b->b_);
}

/** Visit a boolean TRUE */
z3::expr Z3Solver::ExprConverter::visit(const SymBoolTrue * const b) {
  return z3::expr(context_, Z3_mk_true(context_));
}

/** Visit a boolean VAR */
z3::expr Z3Solver::ExprConverter::visit(const SymBoolVar * const b) {
  auto type = Z3_mk_bool_sort(context_);
  return z3::expr(context_, Z3_mk_const(context_, get_symbol(b->name_), type));
}

/** Visit a boolean XOR */
z3::expr Z3Solver::ExprConverter::visit(const SymBoolXor * const b) {
  return z3::expr(context_, Z3_mk_xor(context_, (*this)(b->a_), (*this)(b->b_)));
}

/** Visit an array store */
z3::expr Z3Solver::ExprConverter::visit(const SymArrayStore * const a) {
  return z3::expr(context_, Z3_mk_store(context_, (*this)(a->a_), (*this)(a->key_), (*this)(a->value_)));
}

/** Visit an array variable */
z3::expr Z3Solver::ExprConverter::visit(const SymArrayVar * const a) {
  auto key_sort = context_.bv_sort(a->key_size_);
  auto val_sort = context_.bv_sort(a->value_size_);
  auto type = Z3_mk_array_sort(context_, key_sort, val_sort);
  return z3::expr(context_, Z3_mk_const(context_, get_symbol(a->name_), type));
}




