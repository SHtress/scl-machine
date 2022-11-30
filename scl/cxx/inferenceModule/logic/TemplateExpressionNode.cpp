/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "TemplateExpressionNode.hpp"

TemplateExpressionNode::TemplateExpressionNode(
    ScMemoryContext * context,
    ScAddr const & formulaTemplate,
    TemplateSearcher * templateSearcher)
  : context(context)
  , formulaTemplate(formulaTemplate)
  , templateSearcher(templateSearcher)
{
}
TemplateExpressionNode::TemplateExpressionNode(
    ScMemoryContext * context,
    ScAddr const & formulaTemplate,
    TemplateSearcher * templateSearcher,
    TemplateManager * templateManager,
    ScAddr const & outputStructure)
  : context(context)
  , formulaTemplate(formulaTemplate)
  , templateSearcher(templateSearcher)
  , templateManager(templateManager)
  , outputStructure(outputStructure)
{
}

LogicExpressionResult TemplateExpressionNode::check(ScTemplateParams & params) const
{
  std::vector<ScTemplateSearchResultItem> searchResult = templateSearcher->searchTemplate(formulaTemplate, params);
  std::string result = (!searchResult.empty() ? "true" : "false");
  SC_LOG_DEBUG("Atomic logical formula " + context->HelperGetSystemIdtf(formulaTemplate) + " " + result);

  if (!searchResult.empty())
  {
    return {true, true, searchResult[0], formulaTemplate};
  }
  else
  {
    return {false, false, {nullptr, nullptr}, formulaTemplate};
  }
}

LogicFormulaResult TemplateExpressionNode::compute(LogicFormulaResult & result) const
{
  std::string const formulaIdentifier = context->HelperGetSystemIdtf(formulaTemplate);
  SC_LOG_DEBUG("Checking atomic logical formula " + formulaIdentifier);
  result.replacements = templateSearcher->searchTemplate(formulaTemplate);
  SC_LOG_ERROR("Result replacements size =" + to_string(result.replacements.size()));
  result.value = !result.replacements.empty();
  std::string formulaValue = (result.value ? " true" : " false");
  SC_LOG_DEBUG("Compute atomic logical formula " + formulaIdentifier + formulaValue);

  SC_LOG_ERROR("Result value = " + to_string(result.value));
  return result;
}

LogicFormulaResult TemplateExpressionNode::find(Replacements & replacements) const
{
  LogicFormulaResult result;
  std::vector<ScTemplateParams> paramsVector = ReplacementsUtils::getReplacementsToScTemplateParams(replacements);
  result.replacements = templateSearcher->searchTemplate(formulaTemplate, paramsVector);
  result.value = !result.replacements.empty();

  std::string const idtf = context->HelperGetSystemIdtf(formulaTemplate);
  std::string ending = (result.value ? " true" : " false");
  SC_LOG_DEBUG("Find Statement " + idtf + ending);

  return result;
}

LogicFormulaResult TemplateExpressionNode::generate(Replacements & replacements) const
{
  LogicFormulaResult result;
  std::vector<ScTemplateParams> paramsVector = ReplacementsUtils::getReplacementsToScTemplateParams(replacements);
  if (paramsVector.empty())
  {
    result.isGenerated = false;
    SC_LOG_DEBUG("Atomic logical formula " + context->HelperGetSystemIdtf(formulaTemplate) + " is not generated");
    return compute(result);
  }

  for (ScTemplateParams const & scTemplateParams : paramsVector)
  {
    std::vector<ScTemplateSearchResultItem> searchResult =
        templateSearcher->searchTemplate(formulaTemplate, scTemplateParams);
    if (searchResult.empty())
    {
      ScTemplate generatedTemplate;
      context->HelperBuildTemplate(generatedTemplate, formulaTemplate, scTemplateParams);

      ScTemplateGenResult generationResult;
      ScTemplate::Result const & genTemplate = context->HelperGenTemplate(generatedTemplate, generationResult);
      result.isGenerated = true;
      SC_LOG_DEBUG("Atomic logical formula " + context->HelperGetSystemIdtf(formulaTemplate) + " is generated");
      bool outputIsValid = outputStructure.IsValid();
      for (size_t i = 0; i < generationResult.Size(); ++i)
      {
        templateSearcher->addParamIfNotPresent(generationResult[i]);
        if (outputIsValid)
          context->CreateEdge(ScType::EdgeAccessConstPosPerm, outputStructure, generationResult[i]);
      }
    }
  }
  return compute(result);
}
