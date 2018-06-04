// Author(s): Thomas Neele
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file pbessymbolicbisim.cpp

#define TOOLNAME "pbessymbolicbisim"
#define AUTHORS "Thomas Neele"

//C++
#include <exception>
#include <cstdio>

//Tool framework
#include "mcrl2/data/bool.h"
#include "mcrl2/data/data_expression.h"
#include "mcrl2/data/lambda.h"
#include "mcrl2/data/parse.h"
#include "mcrl2/data/prover_tool.h"
#include "mcrl2/data/rewriter_tool.h"
#include "mcrl2/pbes/pbes.h"
#include "mcrl2/utilities/input_tool.h"

#include "simplifier_mode.h"
#include "symbolic_bisim.h"

using namespace mcrl2::utilities;
using namespace mcrl2::core;
using mcrl2::data::tools::rewriter_tool;
using mcrl2::utilities::tools::input_tool;
using namespace mcrl2::log;


class pbessymbolicbisim_tool: public rewriter_tool<input_tool>
{

protected:
  typedef rewriter_tool<input_tool> super;

  simplifier_mode m_mode;
  std::size_t m_num_refine_steps;
  bool m_fine_initial_partition;

  /// Parse the non-default options.
  void parse_options(const command_line_parser& parser)
  {
    super::parse_options(parser);

    m_mode = parser.option_argument_as<simplifier_mode>("simplifier");
    if(parser.options.count("refine-steps") > 0)
    {
      m_num_refine_steps = parser.option_argument_as<std::size_t>("refine-steps");
    }
    m_fine_initial_partition = parser.options.count("fine-initial") > 0;
  }

  void add_options(interface_description& desc)
  {
    super::add_options(desc);
    desc.add_option("simplifier", make_enum_argument<simplifier_mode>("MODE")
      .add_value(simplify_fm)
#ifdef DBM_PACKAGE_AVAILABLE
      .add_value(simplify_dbm)
#endif
      .add_value(simplify_finite_domain)
      .add_value(simplify_identity)
      .add_value(simplify_auto, true),
      "set the simplifying strategy for expressions",'s');
    desc.add_option("refine-steps",
               make_mandatory_argument("NUM"),
               "perform the given number of refinement steps between each search for a proof graph",
               'n');
    desc.add_option("fine-initial",
               "use a fine initial partition, such that each block contains only one PBES variable");
  }

public:
  pbessymbolicbisim_tool()
    : super(
      TOOLNAME,
      AUTHORS,
      "Output the minimal LTS under strong bisimulation",
      "Performs partition refinement on "
      "INFILE and outputs the resulting LTS. "
      "This tool is highly experimental. "),
      m_num_refine_steps(1),
      m_fine_initial_partition(false)
  {}

  /// Runs the algorithm.
  /// Reads a specification from input_file,
  /// applies real time abstraction to it and writes the result to output_file.
  bool run()
  {
    mCRL2log(verbose) << "Parameters of pbessymbolicbisim:" << std::endl;
    mCRL2log(verbose) << "  input file:         " << m_input_filename << std::endl;
    mCRL2log(verbose) << "  data rewriter       " << m_rewrite_strategy << std::endl;

    mcrl2::pbes_system::pbes spec;
    std::ifstream in;
    in.open(m_input_filename);
    spec.load(in);
    in.close();

    mcrl2::data::symbolic_bisim_algorithm(spec, m_num_refine_steps, m_rewrite_strategy, m_mode, m_fine_initial_partition).run();

    return true;
  }

};

int main(int argc, char** argv)
{
  return pbessymbolicbisim_tool().execute(argc, argv);
}
