// Author(s): Wieger Wesselink, Simon Janssen, Tim Willemse
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file pbesconstelm.cpp

// #define MCRL2_PBES_CONSTELM_DEBUG
// #define MCRL2_PBES_EXPRESSION_BUILDER_DEBUG

#include <iostream>
#include <string>
#include "mcrl2/bes/pbes_rewriter_tool.h"
#include "mcrl2/bes/pbes_input_tool.h"
#include "mcrl2/bes/pbes_output_tool.h"
#include "mcrl2/data/rewriter_tool.h"
#include "mcrl2/pbes/tools.h"
#include "mcrl2/utilities/input_output_tool.h"

using namespace mcrl2;
using namespace mcrl2::log;
using namespace mcrl2::pbes_system;
using namespace mcrl2::core;
using namespace mcrl2::utilities;
using namespace mcrl2::utilities::tools;
using bes::tools::pbes_input_tool;
using bes::tools::pbes_output_tool;
using bes::tools::pbes_rewriter_tool;
using data::tools::rewriter_tool;

class pbes_constelm_tool: public pbes_input_tool<pbes_output_tool<pbes_rewriter_tool<rewriter_tool<input_output_tool> > > >
{
  protected:
    typedef pbes_input_tool<pbes_output_tool<pbes_rewriter_tool<rewriter_tool<input_output_tool> > > > super;

    bool m_compute_conditions;
    bool m_remove_redundant_equations;

    void parse_options(const command_line_parser& parser)
    {
      super::parse_options(parser);
      m_compute_conditions = parser.options.count("compute-conditions") > 0;
      m_remove_redundant_equations = parser.options.count("remove-equations") > 0;
    }

    void add_options(interface_description& desc)
    {
      super::add_options(desc);
      desc.add_option("compute-conditions", "compute propagation conditions", 'c');
      desc.add_option("remove-equations", "remove redundant equations", 'e');
    }

  public:
    pbes_constelm_tool()
      : super(
        "pbesconstelm",
        "Wieger Wesselink; Simon Janssen and Tim Willemse",
        "remove constant parameters from a PBES",
        "Reads a file containing a PBES, and applies constant parameter elimination to it. If OUTFILE "
        "is not present, standard output is used. If INFILE is not present, standard input is used."
      )
    {}

    bool run()
    {
      mCRL2log(verbose) << "pbesconstelm parameters:" << std::endl;
      mCRL2log(verbose) << "  input file:         " << m_input_filename << std::endl;
      mCRL2log(verbose) << "  output file:        " << m_output_filename << std::endl;
      mCRL2log(verbose) << "  compute conditions: " << std::boolalpha << m_compute_conditions << std::endl;
      mCRL2log(verbose) << "  remove redundant equations: " << std::boolalpha << m_remove_redundant_equations << std::endl;

      pbesconstelm(input_filename(),
                   output_filename(),
                   pbes_input_format(),
                   pbes_output_format(),
                   rewrite_strategy(),
                   rewriter_type(),
                   m_compute_conditions,
                   m_remove_redundant_equations
                  );
      return true;
    }

};

int main(int argc, char* argv[])
{
  return pbes_constelm_tool().execute(argc, argv);
}
