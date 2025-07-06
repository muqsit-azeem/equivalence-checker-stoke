//
// Created by andrea on 3/18/25.
//

#ifndef REGEX_H
#define REGEX_H

#include "src/no_data_validator/operation.h"

#include <unordered_set>

namespace stoke {

class RegEx {
public:
  ~RegEx(){}

  RegEx(Cfg& cfg): cfg_(cfg) {}

  bool isEmpty(size_t start, size_t end) {
    return false;
  }

  bool containsJustEmptyWord(size_t start, size_t end) {
    return false;
  }

  bool getRegex(size_t start, size_t end, std::shared_ptr<Operation>& regex, bool split = false);

private:
  Cfg& cfg_;

  bool getPath(size_t start, size_t end, std::map<std::tuple<size_t,size_t>, std::shared_ptr<Operation>>& result,
    std::set<size_t>& nodes_between, std::map<size_t, std::set<size_t>>& succs, std::map<size_t, std::set<size_t>>& preds);
  void dfs(size_t curr, std::set<size_t>& visited, bool find_succ);
  void sympifyRegex(size_t start, size_t end, std::map<std::tuple<size_t,size_t>, std::shared_ptr<Operation>>& regex_map,
    std::set<size_t>& nodes_between, std::map<size_t, std::set<size_t>>& succs, std::map<size_t, std::set<size_t>>& preds, bool split);
  void joinEdges(size_t pred, size_t succ, size_t node, std::map<std::tuple<size_t,size_t>, std::shared_ptr<Operation>>& regex_map, bool contains_loop);
  void joinEdgesSplit(size_t pred, size_t succ, size_t node, std::map<std::tuple<size_t,size_t>, std::shared_ptr<Operation>>& regex_map, bool contains_loop);
};

} // namespace stoke

#endif //REGEX_H
