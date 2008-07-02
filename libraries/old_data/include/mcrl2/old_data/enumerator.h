// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/old_data/enumerator.h
/// \brief The class enumerator.

#ifndef MCRL2_OLD_DATA_ENUMERATOR_H
#define MCRL2_OLD_DATA_ENUMERATOR_H

#include <utility>
#include <boost/shared_ptr.hpp>
#include "mcrl2/atermpp/vector.h"
#include "mcrl2/atermpp/set.h"
#include "mcrl2/atermpp/aterm_access.h"
#include "mcrl2/core/sequence.h"
#include "mcrl2/old_data/enum.h"
#include "mcrl2/old_data/rewriter.h"
#include "mcrl2/old_data/data_specification.h"
#include "mcrl2/old_data/replace.h"
#include "mcrl2/utilities/aterm_ext.h"

namespace mcrl2 {

namespace old_data {

/// A data expression that contains additional information for the enumerator.
class enumerator_expression
{
  protected:
    data_expression m_expression;
    data_variable_list m_variables;
  
  public:
    /// Constructor.
    enumerator_expression()
    {}
    
    /// Constructor.
    enumerator_expression(data_expression expression, data_variable_list variables)
      : m_expression(expression),  m_variables(variables)
    {}
    
    /// The contained data expression.
    data_expression expression() const
    {
      return m_expression;
    }
    
    /// The unbound variables of the contained expression.
    data_variable_list variables() const
    {
      return m_variables;
    }
    
    bool is_constant() const
    {
      return m_variables.empty();
    }
};

/// \cond INTERNAL_DOCS
namespace detail {

  template <typename VariableContainer, typename EnumeratorExpressionContainer>
  struct data_enumerator_replace_helper
  {
    const VariableContainer& variables_;
    const EnumeratorExpressionContainer& replacements_;
    
    data_enumerator_replace_helper(const VariableContainer& variables,
                                   const EnumeratorExpressionContainer& replacements
                                  )
      : variables_(variables), replacements_(replacements)
    {
      assert(variables.size() == replacements.size());
    }
    
    data_expression operator()(data_variable t) const
    {
      typename VariableContainer::const_iterator i = variables_.begin();
      typename EnumeratorExpressionContainer::const_iterator j = replacements_.begin();
      for (; i != variables_.end(); ++i, ++j)
      {
        if (*i == t)
        {
          return j->expression();
        }
      }
      return t;
    }
  };

  struct data_enumerator_helper
  {
    const enumerator_expression& e_;
    const atermpp::vector<enumerator_expression>& values_;
    atermpp::vector<enumerator_expression>& result_;

    data_enumerator_helper(const enumerator_expression& e,
                           const atermpp::vector<enumerator_expression>& values,
                           atermpp::vector<enumerator_expression>& result
                          )
     : e_(e), values_(values), result_(result)
    {}
    
    void operator()()
    {
      data_expression d = replace_data_variables(e_.expression(), data_enumerator_replace_helper<data_variable_list, atermpp::vector<enumerator_expression> >(e_.variables(), values_));
      std::vector<data_variable> v;
      for (atermpp::vector<enumerator_expression>::const_iterator i = values_.begin(); i != values_.end(); ++i)
      {
        v.insert(v.end(), i->variables().begin(), i->variables().end());
      }       
      result_.push_back(enumerator_expression(d, data_variable_list(v.begin(), v.end())));
    }
  };

} // namespace detail
/// \endcond

/// A class that enumerates data expressions.
template <typename DataRewriter, typename IdentifierGenerator>
class data_enumerator
{
  protected:
    typedef std::map<sort_expression, std::vector<data_operation> > constructor_map;

    const data_specification* m_data;
    DataRewriter* m_rewriter;
    IdentifierGenerator* m_generator;
    constructor_map m_constructors;

    /// Returns the constructors with target s.
    const std::vector<data_operation>& constructors(sort_expression s)
    {
      constructor_map::const_iterator i = m_constructors.find(s);
      if (i != m_constructors.end())
      {
        return i->second;
      }
      data_operation_list d = m_data->constructors(s);
      std::vector<data_operation> v(d.begin(), d.end());
      m_constructors[s] = v;
      return m_constructors[s];
    }

  public:
    /// Constructor.
    data_enumerator(const data_specification& data_spec,
                    DataRewriter& rewriter,
                    IdentifierGenerator& generator)
     : m_data(&data_spec), m_rewriter(&rewriter), m_generator(&generator)
    {}

    /// Enumerates a data variable.
    atermpp::vector<enumerator_expression> enumerate(const data_variable& v)
    {
      atermpp::vector<enumerator_expression> result;
      const std::vector<data_operation>& c = constructors(v.sort());

      for (std::vector<data_operation>::const_iterator i = c.begin(); i != c.end(); ++i)
      {
        sort_expression_list dsorts = domain_sorts(i->sort());
        std::vector<data_variable> variables;
        for (sort_expression_list::iterator j = dsorts.begin(); j != dsorts.end(); ++j)
        {
          variables.push_back(data_variable((*m_generator)(), *j));
        }
        data_variable_list w(variables.begin(), variables.end());
        data_expression_list w1 = make_data_expression_list(w);
        result.push_back(enumerator_expression((*m_rewriter)((*i)(w1)), w));
      }

      return result;
    }

    /// Enumerates a data expression. Only the variables of the enumerator
    /// expression are expanded. Fresh variables are created using the
    /// identifier generator that was passed in the constructor.
    atermpp::vector<enumerator_expression> enumerate(const enumerator_expression& e)
    {
      atermpp::vector<enumerator_expression> result;

      // Compute the instantiations for each variable of e.
      std::vector<atermpp::vector<enumerator_expression> > enumerated_values;
      for (data_variable_list::iterator i = e.variables().begin(); i != e.variables().end(); ++i)
      {
        enumerated_values.push_back(enumerate(*i));     
      }
      
      atermpp::vector<enumerator_expression> values(enumerated_values.size());
      core::foreach_sequence(enumerated_values, values.begin(), detail::data_enumerator_helper(e, values, result));
      return result;
    }
};

} // namespace old_data

} // namespace mcrl2

#endif // MCRL2_OLD_DATA_ENUMERATOR_H
