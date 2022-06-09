#pragma once

#include <ostream>
#include <string>
#include <tuple>
#include <vector>

#include "yaml-cpp/yaml.h"

#include "expr/defs.h"

namespace eqspace {
namespace axpred {

struct AxPredsStr {
  using RuleSetStrTy =
      std::vector<std::pair<std::string, std::vector<std::string>>>;
  using RuleGroupStrTy = std::tuple<std::string, std::string, RuleSetStrTy>;

  std::map<std::string, std::vector<RuleGroupStrTy>> Data;
};

struct AxPreds {
  using RuleGroupTy = std::tuple<std::string, expr_ref, expr_ref>;

  context *Context = nullptr;
  std::map<std::string, std::vector<RuleGroupTy>> Data;

  AxPreds() = default;
  AxPreds(context *Context, const AxPredsStr &AxsStr);

  friend std::ostream &operator<<(std::ostream &Os, const AxPreds &Axs);

private:
  decltype(Data) parseAxPredsStr(const AxPredsStr &AxsStr) const;
};

} // namespace axpred
} // namespace eqspace

namespace YAML {

using namespace eqspace::axpred;

template <> struct convert<AxPredsStr> {
  static Node encode(const AxPredsStr &AxsStr) {
    Node AxsYML;

    for (auto &AxpredGroups : AxsStr.Data) {
      auto &AxpredName = AxpredGroups.first;
      auto &Groups = AxpredGroups.second;

      AxsYML["AxPredNames"].push_back(AxpredGroups.first);

      for (auto &GroupConclRuleset : Groups) {
        auto &GroupName = std::get<0>(GroupConclRuleset);
        auto &Concl = std::get<1>(GroupConclRuleset);
        auto &Ruleset = std::get<2>(GroupConclRuleset);

        AxsYML["AxPredRules"][AxpredName][GroupName]["Conclusion"] = Concl;

        for (auto &RulePremises : Ruleset) {
          auto &RuleName = std::get<0>(RulePremises);
          auto &Premises = std::get<1>(RulePremises);

          for (auto &Premise : Premises) {
            AxsYML["AxPredRules"][AxpredName][GroupName]["Rules"][RuleName]
                .push_back(Premise);
          }
        }
      }
    }

    return AxsYML;
  }

  static bool decode(const Node &AxsYML, AxPredsStr &AxsStr) {
    for (auto &AxpredNameYML : AxsYML["AxPredNames"]) {
      auto AxpredName = AxpredNameYML.as<std::string>();

      AxsStr.Data.emplace(AxpredName, decltype(AxsStr.Data)::mapped_type());

      for (auto &AxpredGroupYML : AxsYML["AxPredRules"][AxpredName]) {
        auto GroupName = AxpredGroupYML.first.as<std::string>();
        auto Concl = AxpredGroupYML.second["Conclusion"].as<std::string>();

        AxPredsStr::RuleSetStrTy RulesStr;

        for (auto &AxpredRuleYML : AxpredGroupYML.second["Rules"]) {
          auto RuleName = AxpredRuleYML.first.as<std::string>();

          RulesStr.emplace_back(std::move(RuleName),
                                decltype(RulesStr)::value_type::second_type());

          for (auto &PremiseYML : AxpredRuleYML.second) {
            auto Premise = PremiseYML.as<std::string>();

            RulesStr.back().second.emplace_back(std::move(Premise));
          }
        }

        AxPredsStr::RuleGroupStrTy GroupStr(
            std::move(GroupName), std::move(Concl), std::move(RulesStr));

        AxsStr.Data.at(AxpredName).emplace_back(std::move(GroupStr));
      }
    }

    return true;
  }
};

} // namespace YAML
