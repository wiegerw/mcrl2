// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/pbes/detail/lps2pbes_rhs.h
/// \brief add your file description here.

#ifndef MCRL2_PBES_DETAIL_LPS2PBES_RHS_H
#define MCRL2_PBES_DETAIL_LPS2PBES_RHS_H

#include "mcrl2/atermpp/detail/aterm_list_utility.h"
#include "mcrl2/lps/replace.h"
#include "mcrl2/modal_formula/find.h"
#include "mcrl2/modal_formula/traverser.h"
#include "mcrl2/pbes/pbes.h"
#include "mcrl2/pbes/replace.h"
#include "mcrl2/pbes/detail/lps2pbes_par.h"
#include "mcrl2/pbes/detail/lps2pbes_sat.h"
#include "mcrl2/pbes/detail/lps2pbes_utility.h"

namespace mcrl2 {

namespace pbes_system {

namespace detail {

inline
pbes_expression RHS(const state_formulas::state_formula& x0,
                    const state_formulas::state_formula& x,
                    const lps::linear_process& lps,
                    data::set_identifier_generator& id_generator,
                    const data::variable& T = data::variable()
                   );

template <typename Derived>
struct rhs_traverser: public state_formulas::state_formula_traverser<Derived>
{
  typedef state_formulas::state_formula_traverser<Derived> super;
  using super::enter;
  using super::leave;
  using super::operator();

#if BOOST_MSVC
#include "mcrl2/core/detail/traverser_msvc.inc.h"
#endif

  const state_formulas::state_formula& phi0; // the original formula
  const lps::linear_process& lps;
  data::set_identifier_generator& id_generator;
  const data::variable& T;
  atermpp::vector<pbes_expression> result_stack;

  rhs_traverser(const state_formulas::state_formula& phi0_,
                const lps::linear_process& lps_,
                data::set_identifier_generator& id_generator_,
                const data::variable& T_ = data::variable()
               )
    : phi0(phi0_), lps(lps_), id_generator(id_generator_), T(T_)
  {}

  Derived& derived()
  {
    return static_cast<Derived&>(*this);
  }

  void push(const pbes_expression& x)
  {
    result_stack.push_back(x);
  }

  pbes_expression& top()
  {
    return result_stack.back();
  }

  const pbes_expression& top() const
  {
    return result_stack.back();
  }

  pbes_expression pop()
  {
    pbes_expression result = top();
    result_stack.pop_back();
    return result;
  }

  void push_variables(const data::variable_list& v)
  {
    for (data::variable_list::const_iterator i = v.begin(); i != v.end(); ++i)
    {
      id_generator.add_identifier(i->name());
    }
  }

  void pop_variables(const data::variable_list& v)
  {
    for (data::variable_list::const_iterator i = v.begin(); i != v.end(); ++i)
    {
      id_generator.remove_identifier(i->name());
    }
  }

  bool is_timed() const
  {
    return T != data::variable();
  }

  void leave(const data::data_expression& x)
  {
    push(x);
  }

  void leave(const state_formulas::true_&)
  {
    push(true_());
  }

  void leave(const state_formulas::false_&)
  {
    push(false_());
  }

  void operator()(const state_formulas::not_&)
  {
    throw mcrl2::runtime_error("rhs_traverser: negation is not supported!");
  }

  void leave(const state_formulas::and_&)
  {
    pbes_expression right = pop();
    pbes_expression left = pop();
    push(pbes_expr_optimized::and_(left, right));
  }

  void leave(const state_formulas::or_&)
  {
    pbes_expression right = pop();
    pbes_expression left = pop();
    push(pbes_expr_optimized::or_(left, right));
  }

  void operator()(const state_formulas::imp&)
  {
    throw mcrl2::runtime_error("rhs_traverser: implication is not supported!");
  }

  void operator()(const state_formulas::forall& x)
  {
    push_variables(x.variables());
    derived()(x.body());
    top() = forall(x.variables(), top());
    //pop_variables(x.variables());
  }

  void operator()(const state_formulas::exists& x)
  {
    push_variables(x.variables());
    derived()(x.body());
    top() = exists(x.variables(), top());
    //pop_variables(x.variables());
  }

  // share code between must and may
  template <typename Expr>
  void handle_must_may(const Expr& x, bool is_must)
  {
    bool timed = is_timed();
    atermpp::vector<pbes_expression> v;
    pbes_expression rhs0 = RHS(phi0, x.operand(), lps, id_generator, T);
    action_formulas::action_formula alpha = x.formula();

    const lps::action_summand_vector& asv = lps.action_summands();
    for (lps::action_summand_vector::const_iterator i = asv.begin(); i != asv.end(); ++i)
    {
      data::data_expression ci = i->condition();
      lps::multi_action ai     = i->multi_action();
      data::assignment_list gi = i->assignments();
      data::variable_list yi   = i->summation_variables();

      pbes_expression rhs = rhs0;
      data::variable_list y = pbes_system::detail::make_fresh_variables(yi, id_generator);
      ci = data::replace_free_variables(ci, data::make_sequence_sequence_substitution(yi, y));
      lps::replace_free_variables(ai, data::make_sequence_sequence_substitution(yi, y));
      gi = data::replace_free_variables(gi, data::make_sequence_sequence_substitution(yi, y));
      data::data_expression ti = ai.time();
      pbes_expression p1 = Sat(ai, alpha);
      pbes_expression p2 = ci;
      rhs = pbes_system::replace_free_variables(rhs, data::assignment_sequence_substitution(gi));
      pbes_expression p = and_(p1, p2);
      if (timed)
      {
        rhs = pbes_system::replace_free_variables(rhs, data::assignment(T, ti));
        p = and_(p, data::greater(ti, T));
      }
      p = is_must ? pbes_expr::forall(y, imp(p, rhs)) : pbes_expr::exists(y, and_(p, rhs));
      v.push_back(p);
    }

    pbes_expression result = is_must ? pbes_expr::join_and(v.begin(), v.end()) : pbes_expr::join_or(v.begin(), v.end());
    push(result);
  }

  void operator()(const state_formulas::must& x)
  {
    handle_must_may(x, true);
  }

  void operator()(const state_formulas::may& x)
  {
    handle_must_may(x, false);
  }

  void leave(const state_formulas::yaled&)
  {
    throw mcrl2::runtime_error("rhs_traverser: yaled is not supported!");
  }

  void leave(const state_formulas::yaled_timed& x)
  {
    data::data_expression t = x.time_stamp();
    atermpp::vector<pbes_expression> v;
    const lps::action_summand_vector& asv = lps.action_summands();
    for (lps::action_summand_vector::const_iterator i = asv.begin(); i != asv.end(); ++i)
    {
      pbes_expression       ci = i->condition();
      data::data_expression ti = i->multi_action().time();
      data::variable_list   yi = i->summation_variables();
      pbes_expression p = forall(yi, or_(data::sort_bool::not_(ci), data::greater(t, ti)));
      v.push_back(p);
    }
    const lps::deadlock_summand_vector& dsv = lps.deadlock_summands();
    for (lps::deadlock_summand_vector::const_iterator j = dsv.begin(); j != dsv.end(); ++j)
    {
      data::data_expression cj = j->condition();
      data::data_expression tj = j->deadlock().time();
      data::variable_list   yj = j->summation_variables();
      pbes_expression p = forall(yj, or_(data::sort_bool::not_(cj), data::greater(t, tj)));
      v.push_back(p);
    }
    push(and_(pbes_expr::join_or(v.begin(), v.end()), data::greater(t, T)));
  }

  void leave(const state_formulas::delay&)
  {
    throw mcrl2::runtime_error("rhs_traverser: delay is not supported!");
  }

  void leave(const state_formulas::delay_timed& x)
  {
    data::data_expression t = x.time_stamp();
    atermpp::vector<pbes_expression> v;
    const lps::action_summand_vector& asv = lps.action_summands();
    for (lps::action_summand_vector::const_iterator i = asv.begin(); i != asv.end(); ++i)
    {
      data::data_expression ci = i->condition();
      data::data_expression ti = i->multi_action().time();
      data::variable_list   yi = i->summation_variables();
      pbes_expression p = exists(yi, and_(ci, data::less_equal(t, ti)));
      v.push_back(p);
    }
    const lps::deadlock_summand_vector& dsv = lps.deadlock_summands();
    for (lps::deadlock_summand_vector::const_iterator j = dsv.begin(); j != dsv.end(); ++j)
    {
      data::data_expression cj = j->condition();
      data::data_expression tj = j->deadlock().time();
      data::variable_list   yj = j->summation_variables();
      pbes_expression p = exists(yj, and_(cj, data::less_equal(t, tj)));
      v.push_back(p);
    }
    push(or_(pbes_expr::join_or(v.begin(), v.end()), data::less_equal(t, T)));
  }

  void leave(const state_formulas::variable& x)
  {
    using atermpp::detail::operator+;
    core::identifier_string X = x.name();
    data::data_expression_list d = x.arguments();
    data::variable_list xp = lps.process_parameters();
    data::data_expression_list e = d + xp + Par(X, data::variable_list(), phi0);
    if (is_timed())
    {
      e = T + e;
    }
    push(propositional_variable_instantiation(X, e));
  }

  void operator()(const state_formulas::nu& x)
  {
    using atermpp::detail::operator+;
    core::identifier_string X = x.name();
    data::data_expression_list d = detail::mu_expressions(x);
    data::variable_list xp = lps.process_parameters();
    data::data_expression_list e = d + xp + Par(X, data::variable_list(), phi0);
    if (is_timed())
    {
      e = T + e;
    }
    push(propositional_variable_instantiation(X, e));
  }

  void operator()(const state_formulas::mu& x)
  {
    using atermpp::detail::operator+;
    core::identifier_string X = x.name();
    data::data_expression_list d = detail::mu_expressions(x);
    data::variable_list xp = lps.process_parameters();
    data::data_expression_list e = d + xp + Par(X, data::variable_list(), phi0);
    if (is_timed())
    {
      e = T + e;
    }
    push(propositional_variable_instantiation(X, e));
  }
};

template <template <class> class Traverser>
struct apply_rhs_traverser: public Traverser<apply_rhs_traverser<Traverser> >
{
  typedef Traverser<apply_rhs_traverser<Traverser> > super;
  using super::enter;
  using super::leave;
  using super::operator();

  apply_rhs_traverser(const state_formulas::state_formula& phi0,
                      const lps::linear_process& lps,
                      data::set_identifier_generator& id_generator,
                      const data::variable& T = data::variable()
                     )
    : super(phi0, lps, id_generator, T)
  {}

#ifdef BOOST_MSVC
#include "mcrl2/core/detail/traverser_msvc.inc.h"
#endif
};

inline
pbes_expression RHS(const state_formulas::state_formula& x0,
                    const state_formulas::state_formula& x,
                    const lps::linear_process& lps,
                    data::set_identifier_generator& id_generator,
                    const data::variable& T // = data::variable()
                   )
{
  apply_rhs_traverser<rhs_traverser> f(x0, lps, id_generator, T);
  f(x);
  return f.top();
}

inline
pbes_expression RHS_structured(const state_formulas::state_formula& x0,
                               const state_formulas::state_formula& x,
                               const lps::linear_process& lps,
                               data::set_identifier_generator& id_generator,
                               data::set_identifier_generator& propvar_generator,
                               data::variable_list& variables,
                               const fixpoint_symbol& sigma,
                               atermpp::vector<pbes_equation>& Z,
                               const data::variable& T = data::variable()
                    );

template <typename Derived>
struct rhs_structured_traverser: public rhs_traverser<Derived>
{
  typedef rhs_traverser<Derived> super;
  using super::enter;
  using super::leave;
  using super::operator();
  using super::push;
  using super::top;
  using super::pop;
  using super::is_timed;
  using super::phi0;
  using super::lps;
  using super::id_generator;
  using super::T;

#if BOOST_MSVC
#include "mcrl2/core/detail/traverser_msvc.inc.h"
#endif

  std::multiset<data::variable> variables;
  const fixpoint_symbol& sigma;
  data::set_identifier_generator& propvar_generator;
  atermpp::vector<pbes_equation>& Z; // new equations that are generated on the fly

  rhs_structured_traverser(const state_formulas::state_formula& phi0,
                           const lps::linear_process& lps,
                           data::set_identifier_generator& id_generator,
                           data::set_identifier_generator& propvar_generator_,
                           data::variable_list& variables_,
                           const fixpoint_symbol& sigma_,
                           atermpp::vector<pbes_equation>& Z_,
                           const data::variable& T = data::variable()
               )
    : super(phi0, lps, id_generator, T),
    	variables(variables_.begin(), variables_.end()),
    	sigma(sigma_),
    	propvar_generator(propvar_generator_),
    	Z(Z_)
  {}

  Derived& derived()
  {
    return static_cast<Derived&>(*this);
  }

  void operator()(const state_formulas::forall& x)
  {
  	data::variable_list v = x.variables();
  	variables.insert(v.begin(), v.end());
  	super::operator()(x);
  	for (data::variable_list::iterator i = v.begin(); i != v.end(); ++i)
    {
      variables.erase(*i);
    }
  }

  void operator()(const state_formulas::exists& x)
  {
  	data::variable_list v = x.variables();
  	variables.insert(v.begin(), v.end());
  	super::operator()(x);
  	for (data::variable_list::iterator i = v.begin(); i != v.end(); ++i)
    {
      variables.erase(*i);
    }
  }

  // share code between must and may
  template <typename Expr>
  void handle_must_may(const Expr& x, bool is_must)
  {
    bool timed = is_timed();
    atermpp::vector<pbes_expression> v;

    // TODO: can this call to find_free_variables be eliminated?
    std::set<data::variable> fv = state_formulas::find_free_variables(x.operand());
    fv.insert(variables.begin(), variables.end());
    data::variable_list vars(fv.begin(), fv.end());

    pbes_expression rhs0 = RHS_structured(phi0, x.operand(), lps, id_generator, propvar_generator, vars, sigma, Z, T);
    action_formulas::action_formula alpha = x.formula();

    const lps::action_summand_vector& asv = lps.action_summands();
    for (lps::action_summand_vector::const_iterator i = asv.begin(); i != asv.end(); ++i)
    {
      data::data_expression ci = i->condition();
      lps::multi_action ai     = i->multi_action();
      data::assignment_list gi = i->assignments();
      data::variable_list yi   = i->summation_variables();

      pbes_expression rhs = rhs0;
      data::variable_list y = pbes_system::detail::make_fresh_variables(yi, id_generator);
      ci = data::replace_free_variables(ci, data::make_sequence_sequence_substitution(yi, y));
      lps::replace_free_variables(ai, data::make_sequence_sequence_substitution(yi, y));
      gi = data::replace_free_variables(gi, data::make_sequence_sequence_substitution(yi, y));
      data::data_expression ti = ai.time();
      pbes_expression p1 = Sat(ai, alpha);
      pbes_expression p2 = ci;
      rhs = pbes_system::replace_free_variables(rhs, data::assignment_sequence_substitution(gi));
      pbes_expression p = and_(p1, p2);
      if (timed)
      {
        rhs = pbes_system::replace_free_variables(rhs, data::assignment(T, ti));
        p = and_(p, data::greater(ti, T));
      }
      p = is_must ? pbes_expr::forall(y, imp(p, rhs)) : pbes_expr::exists(y, and_(p, rhs));

      // generate a new equation 'Y(d) = p', and add Y(d) to v
    	core::identifier_string Y = propvar_generator("Y");
      data::variable_list d(variables.begin(), variables.end());
      propositional_variable Yd(Y, d);
      pbes_equation eqn(sigma, Yd, p);
      Z.push_back(eqn);
      v.push_back(propositional_variable_instantiation(Y, data::make_data_expression_list(d)));
    }

    pbes_expression result = is_must ? pbes_expr::join_and(v.begin(), v.end()) : pbes_expr::join_or(v.begin(), v.end());
    push(result);
  }

  void operator()(const state_formulas::must& x)
  {
    handle_must_may(x, true);
  }

  void operator()(const state_formulas::may& x)
  {
    handle_must_may(x, false);
  }
};

template <template <class> class Traverser>
struct apply_rhs_structured_traverser: public Traverser<apply_rhs_structured_traverser<Traverser> >
{
  typedef Traverser<apply_rhs_structured_traverser<Traverser> > super;
  using super::enter;
  using super::leave;
  using super::operator();

  apply_rhs_structured_traverser(const state_formulas::state_formula& phi0,
                                 const lps::linear_process& lps,
                                 data::set_identifier_generator& id_generator,
                                 data::set_identifier_generator& propvar_generator,
                                 data::variable_list& variables,
                                 const fixpoint_symbol& sigma,
                                 atermpp::vector<pbes_equation>& Z,
                                 const data::variable& T = data::variable()
                                )
    : super(phi0, lps, id_generator, propvar_generator, variables, sigma, Z, T)
  {}

#ifdef BOOST_MSVC
#include "mcrl2/core/detail/traverser_msvc.inc.h"
#endif
};

inline
pbes_expression RHS_structured(const state_formulas::state_formula& x0,
                               const state_formulas::state_formula& x,
                               const lps::linear_process& lps,
                               data::set_identifier_generator& id_generator,
                               data::set_identifier_generator& propvar_generator,
                               data::variable_list& variables,
                               const fixpoint_symbol& sigma,
                               atermpp::vector<pbes_equation>& Z,
                               const data::variable& T
                    )
{
  apply_rhs_structured_traverser<rhs_structured_traverser> f(x0, lps, id_generator, propvar_generator, variables, sigma, Z, T);
  f(x);
  return f.top();
}

} // namespace detail

} // namespace pbes_system

} // namespace mcrl2

#endif // MCRL2_PBES_DETAIL_LPS2PBES_RHS_H