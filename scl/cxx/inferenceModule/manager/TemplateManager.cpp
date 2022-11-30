/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "TemplateManager.hpp"

#include <algorithm>

using namespace inference;

TemplateManager::TemplateManager(ScMemoryContext * ms_context)
{
  this->context = ms_context;
}

std::vector<ScTemplateParams> TemplateManager::createTemplateParamsList(
    ScAddr const & scTemplate,
    ScAddrVector const & argumentList)
{
  return createTemplateParams(scTemplate, argumentList);
}

std::vector<ScTemplateParams> TemplateManager::createTemplateParams(
    ScAddr const & scTemplate,
    ScAddrVector const & argumentList)
{
  std::map<std::string, std::set<ScAddr, AddrComparator>> replacementsMultimap;
  std::vector<ScTemplateParams> templateParamsVector;

  ScIterator3Ptr varIterator = context->Iterator3(scTemplate, ScType::EdgeAccessConstPosPerm, ScType::NodeVar);
  while (varIterator->Next())
  {
    ScAddr var = varIterator->Get(2);
    std::string varName = context->HelperGetSystemIdtf(var);
    if (!replacementsMultimap[varName].empty())
    {
      continue;
    }
    ScAddr argumentOfVar;
    ScIterator5Ptr classesIterator = context->Iterator5(
        ScType::NodeConstClass, ScType::EdgeAccessVarPosPerm, var, ScType::EdgeAccessConstPosPerm, scTemplate);
    while (classesIterator->Next())
    {
      ScAddr varClass = classesIterator->Get(0);
      for (ScAddr const & argument : argumentList)  // this block is executed if inputStructure is valid
      {
        if (context->HelperCheckEdge(varClass, argument, ScType::EdgeAccessConstPosPerm))
          replacementsMultimap[varName].insert(argument);
      }
      if (argumentList.empty())  // this block is executed if inputStructure is not valid
      {
        ScIterator3Ptr iterator3 = context->Iterator3(varClass, ScType::EdgeAccessConstPosPerm, ScType::Unknown);
        while (iterator3->Next())
          replacementsMultimap[varName].insert(iterator3->Get(2));
      }
    }
    if (templateParamsVector.empty())
    {
      std::set<ScAddr, AddrComparator> addresses = replacementsMultimap[varName];
      templateParamsVector.reserve(replacementsMultimap[varName].size());
      for (ScAddr const & address : addresses)
      {
        ScTemplateParams params;
        params.Add(varName, address);
        templateParamsVector.push_back(params);
      }
    }
    else
    {
      std::set<ScAddr, AddrComparator> addresses = replacementsMultimap[varName];
      int amountOfAddressesForVar = addresses.size();
      size_t oldParamsSize = templateParamsVector.size();

      if (amountOfAddressesForVar - 1 < 0)
        amountOfAddressesForVar = 1;

      size_t amountOfNewElements = oldParamsSize * (amountOfAddressesForVar - 1);
      templateParamsVector.reserve(amountOfNewElements);
      size_t beginOfCopy = 0;
      size_t endOfCopy = oldParamsSize;
      for (ScAddr const & address : addresses)
      {
        copy_n(templateParamsVector.begin() + beginOfCopy, oldParamsSize, back_inserter(templateParamsVector));
        for (size_t i = 0; i < oldParamsSize; ++i)
          templateParamsVector[beginOfCopy + i].Add(varName, address);
        beginOfCopy = endOfCopy;
        endOfCopy += oldParamsSize;
      }
    }
  }

  return templateParamsVector;
}
