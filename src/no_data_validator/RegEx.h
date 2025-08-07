//
// Created by andrea on 3/18/25.
//

#ifndef REGEX_H
#define REGEX_H

#include "src/no_data_validator/operation.h"

#include <unordered_set>

#include "src/cfg/paths.h"
#include "src/solver/smtsolver.h"

namespace stoke {

class RegEx {
public:
  ~RegEx(){}

  RegEx(Cfg& cfg): cfg_(cfg) {}

  //creates regex from start to end, if called on start and end before assigns pointer to already calculated regex.
  bool getRegex(size_t start, size_t end, std::shared_ptr<Operation>& regex);

  //creates CfgPath from regex
  bool get_CfgPath(CfgPath& cfg_path, std::shared_ptr<Operation>& regex, SMTSolver& solver);

private:
  Cfg& cfg_;
  std::map<std::tuple<size_t,size_t>, std::shared_ptr<Operation>> regex_in_cfg;

  bool initializeRegEx(size_t start, size_t end, std::map<std::tuple<size_t,size_t>, std::shared_ptr<Operation>>& result,
    std::set<size_t>& nodes_between, std::map<size_t, std::set<size_t>>& succs, std::map<size_t, std::set<size_t>>& preds);
  void dfs(size_t curr, std::set<size_t>& visited, bool find_succ);
  void sympifyRegex(size_t start, size_t end, std::map<std::tuple<size_t,size_t>, std::shared_ptr<Operation>>& regex_map,
    std::set<size_t>& nodes_between, std::map<size_t, std::set<size_t>>& succs, std::map<size_t, std::set<size_t>>& preds);
  void joinEdgesSplit(size_t pred, size_t succ, size_t node, std::map<std::tuple<size_t,size_t>, std::shared_ptr<Operation>>& regex_map, bool contains_loop);
  bool get_CfgPath_base(CfgPath& cfg_path, std::shared_ptr<Operation>& regex, SMTSolver& solver);
};

} // namespace stoke

#endif //REGEX_H
