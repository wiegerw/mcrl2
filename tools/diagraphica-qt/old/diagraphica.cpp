// Author(s): A.J. (Hannes) Pretorius
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file ./diagraph.cpp

#include <QApplication>
#include <QString>
#include <QFileInfo>

#include "wx.hpp" // precompiled headers

#define NAME "diagraphica"
#define AUTHOR "A. Johannes Pretorius"

#include <wx/wx.h>
#include <wx/sysopt.h>
#include <wx/clrpicker.h>
#include "mcrl2/lts/lts_io.h"
#include "mcrl2/utilities/command_line_interface.h"
#include "mcrl2/utilities/wx_tool.h"
#include "mcrl2/atermpp/aterm_init.h"
#include "mcrl2/utilities/exception.h"
#include "diagraph.h"

// windows debug libraries
#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

/// TODO: find out why this is necessary
#ifdef KeyPress
#undef KeyPress
#endif

using namespace std;

DiaGraph::DiaGraph() : super("DiaGraph",
                               "interactive visual analysis of an LTS", // what-is
                               "You are free to use images produced with DiaGraphica.\n" // gui-specific description
                               "In this case, image credits would be much appreciated.\n"
                               "\n"
                               "DiaGraphica was built with wxWidgets (www.wxwidgets.org) and \n"
                               "uses the wxWidget XML parser. \n"
                               "Color schemes were chosen with ColorBrewer (www.colorbrewer.org).",
                               "Multivariate state visualisation and simulation analysis for labelled "
                               "transition systems (LTS's) in the FSM format. If an INFILE is not supplied then "
                               "DiaGraphica is started without opening an LTS.",
                               std::vector< std::string >(1, "Hannes Pretorius")), graph(0)
{ }

bool DiaGraph::run()
{
  // windows debugging
#ifdef _MSC_VER
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  //_CrtSetBreakAlloc( 4271 );
#endif

  wxSystemOptions::SetOption(wxT("mac.listctrl.always_use_generic"), 1);

  // set mode
  mode = MODE_ANALYSIS;

  // set view
  view = VIEW_SIM;

  // init colleagues
  initColleagues();

  clustered = false;
  critSect = false;

  if (!input_filename().empty())
  {
    openFile(input_filename().c_str());
  }

  // start event loop
  return true;
}

IMPLEMENT_APP_NO_MAIN(DiaGraph_gui_tool)
IMPLEMENT_WX_THEME_SUPPORT

int main(int argc, char** argv)
{
  MCRL2_ATERMPP_INIT(argc, argv)

  QApplication app(argc, argv);

  return wxEntry(argc, argv);
}

// -- * -------------------------------------------------------------

// -- functions inherited from wxApp --------------------------------

int DiaGraph::OnExit()
{
  if (graph != 0)
  {
    // clear colleagues
    clearColleagues();
  }

  // normal exit
  return 0;
}


// -- load & save data ----------------------------------------------


void DiaGraph::openFile(const std::string& path)
{
  QFileInfo fileInfo(QString::fromStdString(path));

  Parser parser(this);
  std::string fileName = fileInfo.fileName().toStdString();
  int fileSize = fileInfo.size();

  try
  {
    critSect = true;

    // delete visualizers
    if (arcDgrm != NULL)
    {
      delete arcDgrm;
      arcDgrm = NULL;
    }
    canvasArcD->Refresh();

    if (simulator != NULL)
    {
      delete simulator;
      simulator = NULL;
    }
    if (view == VIEW_SIM)
    {
      canvasSiml->Refresh();
    }

    if (timeSeries != NULL)
    {
      delete timeSeries;
      timeSeries = NULL;
    }
    if (view == VIEW_TRACE)
    {
      canvasTrace->Refresh();
    }

    if (examiner != NULL)
    {
      delete examiner;
      examiner = NULL;
    }
    canvasExnr->Refresh();

    if (editor != NULL)
    {
      delete editor;
      editor = NULL;
    }

    // delete old graph
    if (graph != NULL)
    {
      delete graph;
      graph = NULL;
    }

    // create new graph
    graph = new Graph(this);
    graph->setFileName(fileName);

    // parse file
    initProgress(
      "Opening file",
      "Opening " + fileName,
      fileSize);
    parser.parseFile(
      path,
      graph);
    closeProgress();

    // init graph
    graph->initGraph();

    // set up frame output
    frame->clearOuput();
    frame->setTitleText(fileName);
    frame->setFileOptionsActive();
    frame->displNumNodes(graph->getSizeNodes());
    frame->displNumEdges(graph->getSizeEdges());

    // display attributes
    displAttributes();

    // init new visualizers
    arcDgrm = new ArcDiagram(
      this,
      &settings,
      graph,
      canvasArcD);
    if (mode == MODE_ANALYSIS)
    {
      canvasArcD->Refresh();
      canvasSiml->Refresh();
      canvasExnr->Refresh();
    }

    simulator = new Simulator(
      this,
      &settings,
      graph,
      canvasSiml);

    timeSeries = new TimeSeries(
      this,
      &settings,
      graph,
      canvasTrace);

    examiner = new Examiner(
      this,
      &settings,
      graph,
      canvasExnr);

    editor = new DiagramEditor(
      this,
      graph,
      canvasEdit);
    if (mode == MODE_EDIT)
    {
      canvasEdit->Refresh();
    }

    arcDgrm->setDiagram(editor->getDiagram());
    simulator->setDiagram(editor->getDiagram());
    timeSeries->setDiagram(editor->getDiagram());
    examiner->setDiagram(editor->getDiagram());

    critSect = false;
  }
  catch (const mcrl2::runtime_error& e)
  {
    delete progressDialog;
    progressDialog = NULL;

    wxLogError(wxString(e.what(), wxConvUTF8));

    critSect = false;
  }

  // clear status msg
  frame->setStatusText("");

  // enable edit mode
  frame->enableEditMode(true);
}


void DiaGraph::saveFile(const std::string& path)
{
  // init parser
  Parser parser(this);

  // do parsing
  try
  {
    parser.writeFSMFile(
      path,
      graph);
  }
  catch (const mcrl2::runtime_error& e)
  {
    wxLogError(wxString(e.what(), wxConvUTF8));
  }
}


void DiaGraph::handleLoadAttrConfig(const std::string& path)
{
  // init parser
  Parser parser(this);

  // do parsing
  try
  {
    map< size_t , size_t > attrIdxFrTo;
    map< size_t , vector< std::string > > attrCurDomains;
    map< size_t , map< size_t, size_t  > > attrOrigToCurDomains;

    parser.parseAttrConfig(
      path,
      graph,
      attrIdxFrTo,
      attrCurDomains,
      attrOrigToCurDomains);

    graph->configAttributes(
      attrIdxFrTo,
      attrCurDomains,
      attrOrigToCurDomains);
    displAttributes();

    /*
    ! also need to close any other vis windows e.g. corrlplot... !
    */
  }
  catch (const mcrl2::runtime_error& e)
  {
    wxLogError(wxString(e.what(), wxConvUTF8));
  }
}


void DiaGraph::handleSaveAttrConfig(const std::string& path)
{
  // init parser
  Parser parser(this);

  // do parsing
  try
  {
    parser.writeAttrConfig(
      path,
      graph);
  }
  catch (const mcrl2::runtime_error& e)
  {
    wxLogError(wxString(e.what(), wxConvUTF8));
  }
}


void DiaGraph::handleLoadDiagram(const std::string& path)
{
  // init parser
  Parser parser(this);
  // init diagrams
  Diagram* dgrmOld = editor->getDiagram();
  Diagram* dgrmNew = new Diagram(this/*, canvasEdit*/);

  // do parsing
  try
  {
    parser.parseDiagram(
      path,
      graph,
      dgrmOld,
      dgrmNew);
    editor->setDiagram(dgrmNew);

    arcDgrm->setDiagram(dgrmNew);
    arcDgrm->hideAllDiagrams();

    simulator->clearData();
    simulator->setDiagram(dgrmNew);

    timeSeries->clearData();
    timeSeries->setDiagram(dgrmNew);

    examiner->clearData();
    examiner->setDiagram(dgrmNew);

    if (mode == MODE_EDIT && canvasEdit != NULL)
    {
      canvasEdit->Refresh();
    }

    if (mode == MODE_ANALYSIS)
    {
      if (canvasArcD != NULL)
      {
        canvasArcD->Refresh();
      }
      if (canvasSiml != NULL)
      {
        canvasSiml->Refresh();
      }
      if (canvasExnr != NULL)
      {
        canvasExnr->Refresh();
      }
    }

  }
  catch (const mcrl2::runtime_error& e)
  {
    delete dgrmNew;
    dgrmNew = NULL;

    wxLogError(wxString(e.what(), wxConvUTF8));
  }

  dgrmOld = NULL;
  dgrmNew = NULL;
}


void DiaGraph::handleSaveDiagram(const std::string& path)
{
  // init parser
  Parser parser(this);

  // do parsing
  try
  {
    parser.writeDiagram(
      path,
      graph,
      editor->getDiagram());
  }
  catch (const mcrl2::runtime_error& e)
  {
    wxLogError(wxString(e.what(), wxConvUTF8));
  }
}


// -- general input & output ------------------------------------


void DiaGraph::initProgress(
  const std::string& title,
  const std::string& msg,
  const size_t& max)
{
  if (progressDialog != NULL)
  {
    delete progressDialog;
    progressDialog = NULL;
  }

  // display progress dialog
  progressDialog = new wxProgressDialog(
    wxString(title.c_str(), wxConvUTF8),
    wxString(msg.c_str(), wxConvUTF8),
    (int) max,
    frame);
  // set status message
  frame->setStatusText(msg);
}


void DiaGraph::updateProgress(const size_t& val)
{
  if (progressDialog != NULL)
  {
    progressDialog->Update((int) val);
  }
}


void DiaGraph::closeProgress()
{
  if (progressDialog != NULL)
  {
    delete progressDialog;
    progressDialog = NULL;
  }

  frame->setStatusText("");
}


void DiaGraph::setOutputText(const std::string& msg)
// Clear existing text output and display 'msg'.
{
  frame->appOutputText(msg);
}


void DiaGraph::setOutputText(const int& val)
// Clear existing text output and display 'val'.
{
  std::string msg = Utils::intToStr(val);
  frame->appOutputText(msg);
}


void DiaGraph::appOutputText(const std::string& msg)
// Append 'msg' to the current text output without clearing it first.
{
  frame->appOutputText(msg);
}


void DiaGraph::appOutputText(const int& val)
// Append 'val' to the current text output without clearing it first.
{
  std::string msg = Utils::intToStr(val);
  frame->appOutputText(msg);
}

void DiaGraph::appOutputText(const size_t& val)
// Append 'val' to the current text output without clearing it first.
{
  std::string msg = Utils::size_tToStr(val);
  frame->appOutputText(msg);
}

QColor DiaGraph::getColor(QColor col)
{
  wxColourData   data;
  wxColourDialog dialog(frame, &data);

  if (dialog.ShowModal() == wxID_OK)
  {
    data   = dialog.GetColourData();
    wxColour colTmp = data.GetColour();

    return QColor(colTmp.Red(), colTmp.Green(), colTmp.Blue());
  }
  return col;
}


void DiaGraph::handleCloseFrame(PopupFrame* f)
{
  frame->handleCloseFrame(f);
}


// -- interaction with attributes & domains -------------------------


void DiaGraph::handleAttributeSel(const size_t& idx)
{
  // display domain
  displAttrDomain(idx);
}


void DiaGraph::handleMoveAttr(
  const size_t& idxFr,
  const size_t& idxTo)
{
  if (idxFr != NON_EXISTING && idxFr < graph->getSizeAttributes() &&
      idxTo != NON_EXISTING && idxTo < graph->getSizeAttributes())
  {
    graph->moveAttribute(idxFr, idxTo);
    displAttributes();
    frame->selectAttribute(idxTo);
  }
}


void DiaGraph::handleAttributeDuplicate(const vector< size_t > &indcs)
{
  // duplicate attributes
  graph->duplAttributes(indcs);

  // display attributes
  displAttributes();
}

/*
void DiaGraph::handleAttributeDelete( const vector< int > &indcs )
{
    // reset simulator, timeSeries & examiner
    if ( simulator != NULL )
    {
        simulator->clearData();
        simulator->setDiagram( editor->getDiagram() );

        if ( mode == MODE_ANALYSIS && ( view == VIEW_SIM && canvasSiml != NULL ) )
            canvasSiml->Refresh();
    }

    if ( timeSeries != NULL )
    {
        timeSeries->clearData();
        timeSeries->setDiagram( editor->getDiagram() );

        if ( mode == MODE_ANALYSIS && ( view == VIEW_TRACE && canvasSiml != NULL ) )
            canvasTrace->Refresh();
    }

    if ( examiner != NULL )
    {
        examiner->clearData();
        examiner->setDiagram( editor->getDiagram() );

        if ( mode == MODE_ANALYSIS && canvasExnr != NULL )
            canvasExnr->Refresh();
    }

    // delete attributes
    for ( int i = 0; i < indcs.size(); ++i )
    {
        editor->clearLinkAttrDOF( indcs[i] );
        graph->deleteAttribute( indcs[i] );
    }

    // display attributes
    displAttributes();
    clearAttrDomain();
}
*/

void DiaGraph::handleAttributeDelete(const size_t& idx)
{
  // reset simulator, timeSeries & examiner
  if (simulator != NULL)
  {
    simulator->clearData();
    simulator->setDiagram(editor->getDiagram());

    if (mode == MODE_ANALYSIS && (view == VIEW_SIM && canvasSiml != NULL))
    {
      canvasSiml->Refresh();
    }
  }

  if (timeSeries != NULL)
  {
    timeSeries->clearData();
    timeSeries->setDiagram(editor->getDiagram());

    if (mode == MODE_ANALYSIS && (view == VIEW_TRACE && canvasSiml != NULL))
    {
      canvasTrace->Refresh();
    }
  }

  if (examiner != NULL)
  {
    examiner->clearData();
    examiner->setDiagram(editor->getDiagram());

    if (mode == MODE_ANALYSIS && canvasExnr != NULL)
    {
      canvasExnr->Refresh();
    }
  }

  // update cluster and time series if necessary
  Attribute* attr = graph->getAttribute(idx);
  // get attributes currently clustered on
  size_t posFoundClust = NON_EXISTING;
  vector< size_t > attrsClust;
  arcDgrm->getAttrsTree(attrsClust);
  for (size_t i = 0; i < attrsClust.size(); ++i)
  {
    if (attrsClust[i] == attr->getIndex())
    {
      posFoundClust = i;
      break;
    }
  }

  // recluster if necessary
  if (posFoundClust != NON_EXISTING)
  {
    attrsClust.erase(attrsClust.begin() + posFoundClust);
    handleAttributeCluster(attrsClust);
  }

  // get attributes currently in time series
  size_t posFoundTimeSeries = NON_EXISTING;
  vector< size_t > attrsTimeSeries;
  timeSeries->getAttrIdcs(attrsTimeSeries);
  for (size_t i = 0; i < attrsTimeSeries.size(); ++i)
  {
    if (attrsTimeSeries[i] == attr->getIndex())
    {
      posFoundTimeSeries = i;
      break;
    }
  }

  // re-initiate time series if necessary
  if (posFoundTimeSeries != NON_EXISTING)
  {
    attrsTimeSeries.erase(attrsTimeSeries.begin() + posFoundTimeSeries);
    initTimeSeries(attrsTimeSeries);
  }

  // display results
  displAttributes();
  displAttrDomain(attr->getIndex());
  attr = NULL;

  // delete attribute
  editor->clearLinkAttrDOF(idx);
  graph->deleteAttribute(idx);

  // display attributes
  displAttributes();
  clearAttrDomain();
}


void DiaGraph::handleAttributeRename(
  const size_t& idx,
  const std::string& name)
{
  if (idx != NON_EXISTING && idx < graph->getSizeAttributes())
  {
    graph->getAttribute(idx)->setName(name);
    displAttributes();
  }
}


void DiaGraph::handleAttributeCluster(const vector< size_t > &indcs)
{
  bool zeroCard = false;
  clustered = true;

  for (size_t i = 0; i < indcs.size() && zeroCard == false; ++i)
  {
    if (graph->getAttribute(indcs[i])->getSizeCurValues() == 0)
    {
      zeroCard = true;
    }
  }

  if (indcs.size() > 0)
  {
    if (zeroCard == true)
    {
      wxLogError(
        wxString(wxT("Error clustering.\nAt least one attribute has no domain defined.")));
    }
    else
    {
      critSect = true;

      graph->clustNodesOnAttr(indcs);
      arcDgrm->setAttrsTree(indcs);

      arcDgrm->setDataChanged(true);
      handleMarkFrameClust(timeSeries);

      critSect = false;

      if (canvasArcD != NULL && mode == MODE_ANALYSIS)
      {
        canvasArcD->Refresh();
      }
    }
  }
  else
  {
    vector< size_t > coord;
    coord.push_back(0);

    critSect = true;

    graph->clearSubClusters(coord);
    vector< size_t > empty;
    arcDgrm->setAttrsTree(empty);
    arcDgrm->setDataChanged(true);

    critSect = false;

    if (canvasArcD != NULL && mode == MODE_ANALYSIS)
    {
      canvasArcD->Refresh();
    }
  }
}


void DiaGraph::handleMoveDomVal(
  const size_t& idxAttr,
  const size_t& idxFr,
  const size_t& idxTo)
{
  if (idxAttr != NON_EXISTING && idxAttr < graph->getSizeAttributes())
  {
    Attribute* attr = graph->getAttribute(idxAttr);

    if (idxFr != NON_EXISTING && idxFr < attr->getSizeCurValues() &&
        idxTo != NON_EXISTING && idxTo < attr->getSizeCurValues())
    {
      attr->moveValue(idxFr, idxTo);
      displAttrDomain(idxAttr);
      frame->selectDomainVal(idxTo);
    }

    attr = NULL;
  }
}


void DiaGraph::handleDomainGroup(
  const size_t& attrIdx,
  const vector< int > domIndcs,
  const std::string& newValue)
{
  Attribute* attr;

  if (attrIdx != NON_EXISTING && attrIdx < graph->getSizeAttributes())
  {
    attr = graph->getAttribute(attrIdx);
    attr->clusterValues(domIndcs, newValue);

    displAttributes();
    frame->selectAttribute(attrIdx);
    displAttrDomain(attrIdx);
  }

  attr = NULL;
}


void DiaGraph::handleDomainUngroup(const size_t& attrIdx)
{
  Attribute* attr;

  if (attrIdx != NON_EXISTING && attrIdx < graph->getSizeAttributes())
  {
    attr = graph->getAttribute(attrIdx);
    attr->clearClusters();

    displAttributes();
    frame->selectAttribute(attrIdx);
    displAttrDomain(attrIdx);
  }

  attr = NULL;
}


// -- attribute plots -----------------------------------------------


void DiaGraph::handleAttributePlot(const size_t& idx)
{
  if (canvasDistr == NULL)
  {
    canvasDistr = frame->getCanvasDistr();
    distrPlot = new DistrPlot(
      this,
      graph,
      canvasDistr);
    distrPlot->setDiagram(editor->getDiagram());
  }

  vector< size_t > number;

  // display domain
  displAttrDomain(idx);

  // visualize distribution of domain
  graph->calcAttrDistr(idx, number);
  distrPlot->setValues(
    idx,
    number);
  canvasDistr->Refresh();
}


void DiaGraph::handleAttributePlot(
  const size_t& idx1,
  const size_t& idx2)
{
  if (canvasCorrl == NULL)
  {
    canvasCorrl = frame->getCanvasCorrl();
    corrlPlot = new CorrlPlot(
      this,
      graph,
      canvasCorrl);
    corrlPlot->setDiagram(editor->getDiagram());
  }

  vector< int > indices;
  vector< std::string > vals1;
  vector< vector< size_t > > corrlMap;
  vector< vector< int > > number;

  // display correlation between 2 attr's
  graph->calcAttrCorrl(
    idx1,
    idx2,
    corrlMap,
    number);
  corrlPlot->setValues(
    idx1,
    idx2,
    corrlMap,
    number);
  canvasCorrl->Refresh();
}


void DiaGraph::handleAttributePlot(const vector< size_t > &indcs)
{
  if (canvasCombn == NULL)
  {
    canvasCombn = frame->getCanvasCombn();
    combnPlot = new CombnPlot(
      this,
      graph,
      canvasCombn);
    combnPlot->setDiagram(editor->getDiagram());
  }
  /*
  int combinations     = 1;
  Attribute* attribute = NULL;
  int cardinality      = 0;

  for ( int i = 0; i < indcs.size(); ++i )
  {
      attribute   = graph->getAttribute( indcs[i] );
      cardinality = attribute->getSizeCurValues();
      if ( cardinality > 0 )
          combinations *= cardinality;
  }
  attribute = NULL;
  */
  /*
  if ( combinations < 2048 )
  {
  */
  vector< vector< size_t > > combs;
  vector< size_t > number;

  if (indcs.size() > 0)
  {
    clearAttrDomain();

    graph->calcAttrCombn(
      indcs,
      combs,
      number);

    combnPlot->setValues(
      indcs,
      combs,
      number);
    canvasCombn->Refresh();
  }
  /*
  }
  else
  {
  std::string msg;
      msg.append( "More than 2048 combinations. " );
      msg.append( "Please select fewer attributes " );
      msg.append( "or group their domains." );
      wxLogError( wxString( msg->c_str(), wxConvUTF8 ) );
  }
  */
}


void DiaGraph::handlePlotFrameDestroy()
{
  if (distrPlot != NULL)
  {
    delete distrPlot;
    distrPlot = NULL;
  }
  canvasDistr = NULL;

  if (corrlPlot != NULL)
  {
    delete corrlPlot;
    corrlPlot = NULL;
  }
  canvasCorrl = NULL;

  if (combnPlot != NULL)
  {
    delete combnPlot;
    combnPlot = NULL;
  }
  canvasCombn = NULL;
}


void DiaGraph::handleEditClust(Cluster* c)
{
  frame->displClustMenu();
  tempClust = c;
}


void DiaGraph::handleClustFrameDisplay()
{
  vector< size_t >    attrIdcs;
  vector< std::string > attrNames;

  for (size_t i = 0; i < graph->getSizeAttributes(); ++i)
  {
    attrIdcs.push_back(graph->getAttribute(i)->getIndex());
    attrNames.push_back(graph->getAttribute(i)->getName());
  }

  frame->displAttrInfoClust(
    attrIdcs,
    attrNames);
}


void DiaGraph::handleClustPlotFrameDisplay(const size_t& /*idx*/)
{
  if (canvasDistr == NULL)
  {
    canvasDistr = frame->getCanvasDistr();
    distrPlot = new DistrPlot(
      this,
      graph,
      canvasDistr);
  }

  // visualize distribution of domain
  /*
  vector< int > distr;
  graph->calcAttrDistr( tempClust, idx, distr );
  distrPlot->setValues(
      idx,
      distr );
  canvasDistr->Refresh();
  */
}


void DiaGraph::handleClustPlotFrameDisplay(
  const size_t& idx1,
  const size_t& idx2)
{
  if (canvasCorrl == NULL)
  {
    canvasCorrl = frame->getCanvasCorrl();
    corrlPlot = new CorrlPlot(
      this,
      graph,
      canvasCorrl);
  }

  vector< int > indices;
  vector< std::string > vals1;
  vector< vector< size_t > > corrlMap;
  vector< vector< int > > number;

  // display correlation between 2 attr's
  graph->calcAttrCorrl(
    tempClust,
    idx1,
    idx2,
    corrlMap,
    number);
  corrlPlot->setValues(
    idx1,
    idx2,
    corrlMap,
    number);
  canvasCorrl->Refresh();

}


void DiaGraph::handleClustPlotFrameDisplay(const vector< size_t > &indcs)
{
  if (canvasCombn == NULL)
  {
    canvasCombn = frame->getCanvasCombn();
    combnPlot = new CombnPlot(
      this,
      graph,
      canvasCombn);
  }

  vector< vector< size_t > > combs;
  vector< size_t > number;

  if (indcs.size() > 0)
  {
    graph->calcAttrCombn(
      tempClust,
      indcs,
      combs,
      number);

    combnPlot->setValues(
      indcs,
      combs,
      number);
    canvasCombn->Refresh();
  }
}


void DiaGraph::setClustMode(const int& m)
{
  clustMode = m;
}


size_t DiaGraph::getClustMode()
{
  return clustMode;
}


// -- global mode changes -------------------------------------------


void DiaGraph::handleSetModeAnalysis()
{
  mode = MODE_ANALYSIS;
  if (editor != NULL)
  {
    editor->setEditModeSelect();
    editor->deselectAll();
  }

  frame->clearDOFInfo();
  canvasColChooser = NULL;
  if (colChooser != NULL)
  {
    delete colChooser;
    colChooser = NULL;
  }
  canvasOpaChooser = NULL;
  if (opaChooser != NULL)
  {
    delete opaChooser;
    opaChooser = NULL;
  }
  frame->setEditModeSelect();

  if (arcDgrm != NULL)
  {
    arcDgrm->updateDiagramData();
  }

  if (canvasExnr != NULL)
  {
    canvasExnr->Refresh();
  }
}


void DiaGraph::handleSetModeEdit()
{
  mode = MODE_EDIT;
  editor->reGenText();

  if (canvasExnr != NULL)
  {
    canvasExnr->Refresh();
  }
}


int DiaGraph::getMode()
{
  return mode;
}


void DiaGraph::handleSetViewSim()
{
  view = VIEW_SIM;
  if (arcDgrm != NULL)
  {
    arcDgrm->unmarkLeaves();
  }
  canvasArcD->Refresh();
}


void DiaGraph::handleSetViewTrace()
{
  view = VIEW_TRACE;
  handleMarkFrameClust(timeSeries);
  canvasArcD->Refresh();
}


int DiaGraph::getView()
{
  return view;
}


bool DiaGraph::getClustered()
{
  return clustered;
}



// -- diagram editor ------------------------------------------------


void* DiaGraph::getGraph()
{
  return graph;
}


void DiaGraph::handleNote(const size_t& shapeId, const std::string& msg)
{
  frame->handleNote(shapeId, msg);
}


void DiaGraph::handleEditModeSelect()
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->setEditModeSelect();
  }

  if (colChooser != NULL)
  {
    delete colChooser;
    colChooser = NULL;
  }
  canvasColChooser = NULL;

  if (opaChooser != NULL)
  {
    delete opaChooser;
    opaChooser = NULL;
  }
  canvasOpaChooser = NULL;
}


void DiaGraph::handleEditModeNote()
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->setEditModeNote();
  }
}


void DiaGraph::handleEditModeDOF(Colleague* c)
{
  if (c == frame)
  {
    if (mode == MODE_EDIT && editor != NULL)
    {
      editor->setEditModeDOF();
    }
  }
  else if (c == editor)
  {
    frame->setEditModeDOF();
  }
}


void DiaGraph::handleEditModeRect()
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->setEditModeRect();
  }
}


void DiaGraph::handleEditModeEllipse()
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->setEditModeEllipse();
  }
}


void DiaGraph::handleEditModeLine()
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->setEditModeLine();
  }
}


void DiaGraph::handleEditModeArrow()
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->setEditModeArrow();
  }
}


void DiaGraph::handleEditModeDArrow()
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->setEditModeDArrow();
  }
}


void DiaGraph::handleEditModeFillCol()
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->setFillCol();
  }
}


void DiaGraph::handleEditModeLineCol()
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->setLineCol();
  }
}


void DiaGraph::handleEditShowGrid(const bool& flag)
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->setShowGrid(flag);
  }
}


void DiaGraph::handleEditSnapGrid(const bool& flag)
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->setSnapGrid(flag);
  }
}


void DiaGraph::handleEditShape(
  const bool& cut,
  const bool& copy,
  const bool& paste,
  const bool& clear,
  const bool& bringToFront,
  const bool& sendToBack,
  const bool& bringForward,
  const bool& sendBackward,
  const bool& editDOF,
  const int&  checkedItem)
{
  frame->displShapeMenu(
    cut,
    copy,
    paste,
    clear,
    bringToFront,
    sendToBack,
    bringForward,
    sendBackward,
    editDOF,
    checkedItem);
}


void DiaGraph::handleShowVariable(
  const std::string& variable,
  const int&    variableId)
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleShowVariable(variable, variableId);
  }
}


void DiaGraph::handleShowNote(const std::string& variable, const size_t& shapeId)
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleShowNote(variable, shapeId);
  }
}


void DiaGraph::handleAddText(std::string& variable, size_t& shapeId)
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    return editor->handleAddText(variable, shapeId);
  }
}


void DiaGraph::handleTextSize(size_t& textSize, size_t& shapeId)
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    return editor->handleTextSize(textSize, shapeId);
  }
}


void DiaGraph::handleSetTextSize(size_t& textSize, size_t& shapeId)
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleSetTextSize(textSize, shapeId);
  }
}


void DiaGraph::handleCutShape()
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleCut();
  }
}


void DiaGraph::handleCopyShape()
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleCopy();
  }
}


void DiaGraph::handlePasteShape()
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handlePaste();
  }
}


void DiaGraph::handleDeleteShape()
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleDelete();
  }
}


void DiaGraph::handleBringToFrontShape()
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleBringToFront();
  }
}


void DiaGraph::handleSendToBackShape()
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleSendToBack();
  }
}


void DiaGraph::handleBringForwardShape()
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleBringForward();
  }
}


void DiaGraph::handleSendBackwardShape()
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleSendBackward();
  }
}


void DiaGraph::handleEditDOFShape()
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleEditDOF();
  }
}


void DiaGraph::handleSetDOF(const size_t& attrIdx)
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleSetDOF(attrIdx);
  }
}


void DiaGraph::handleCheckedVariable(const size_t& idDOF, const int& variableId)
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleCheckedVariable(idDOF, variableId);
  }
}

void DiaGraph::handleEditDOF(
  const vector< size_t > &degsOfFrdmIds,
  const vector< std::string > &degsOfFrdm,
  const vector< size_t > &attrIndcs,
  const size_t& selIdx)
{
  // init attrIndcs
  vector< std::string > attributes;
  {
    for (size_t i = 0; i < attrIndcs.size(); ++i)
    {
      if (attrIndcs[i] == NON_EXISTING)
      {
        attributes.push_back("");
      }
      else
        attributes.push_back(
          graph->getAttribute(
            attrIndcs[i])->getName());
    }
  }

  frame->displDOFInfo(
    degsOfFrdmIds,
    degsOfFrdm,
    attributes,
    selIdx);

  // color chooser
  if (colChooser != NULL)
  {
    delete colChooser;
    colChooser = NULL;
  }
  canvasColChooser = frame->getCanvasColDOF();
  colChooser = new ColorChooser(
    this,
    graph,
    canvasColChooser);
  canvasColChooser->Refresh();

  // opacity chooser
  if (opaChooser != NULL)
  {
    delete opaChooser;
    opaChooser = NULL;
  }
  canvasOpaChooser = frame->getCanvasOpaDOF();
  opaChooser = new OpacityChooser(
    this,
    graph,
    canvasOpaChooser);
  canvasOpaChooser->Refresh();
}

void DiaGraph::setDOFColorSelected()
// Select the Color Item in the Edit DOF Menu
{
  frame->setDOFColorSelected();
}

void DiaGraph::setDOFOpacitySelected()
// Select the Opacity Item in the Edit DOF Menu
{
  frame->setDOFOpacitySelected();
}


void DiaGraph::handleDOFSel(const size_t& DOFIdx)
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleDOFSel(DOFIdx);
  }
}


void DiaGraph::handleSetDOFTextStatus(
  const size_t& DOFIdx,
  const int& status)
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleDOFSetTextStatus(DOFIdx, status);
  }
}


size_t DiaGraph::handleGetDOFTextStatus(const size_t& DOFIdx)
{
  size_t result = NON_EXISTING;
  if (mode == MODE_EDIT && editor != NULL)
  {
    result = editor->handleDOFGetTextStatus(DOFIdx);
  }
  return result;
}


void DiaGraph::handleDOFColActivate()
{
  if (colChooser != NULL)
  {
    colChooser->setActive(true);

    if (canvasColChooser != NULL)
    {
      canvasColChooser->Refresh();
    }
  }
}


void DiaGraph::handleDOFColDeactivate()
{
  if (colChooser != NULL)
  {
    colChooser->setActive(false);

    if (canvasColChooser != NULL)
    {
      canvasColChooser->Refresh();
    }
  }
}


void DiaGraph::handleDOFColAdd(
  const double& hue,
  const double& y)
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleDOFColAdd(hue, y);
  }
}


void DiaGraph::handleDOFColUpdate(
  const size_t& idx,
  const double& hue,
  const double& y)
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleDOFColUpdate(idx, hue, y);
  }
}


void DiaGraph::handleDOFColClear(
  const size_t& idx)
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleDOFColClear(idx);
  }
}


void DiaGraph::handleDOFColSetValuesEdt(
  const vector< double > &hue,
  const vector< double > &y)
{
  if (colChooser != NULL)
  {
    colChooser->setPoints(hue, y);
  }
}


void DiaGraph::handleDOFOpaActivate()
{
  if (opaChooser != NULL)
  {
    opaChooser->setActive(true);

    if (canvasOpaChooser != NULL)
    {
      canvasOpaChooser->Refresh();
    }
  }
}


void DiaGraph::handleDOFOpaDeactivate()
{
  if (opaChooser != NULL)
  {
    opaChooser->setActive(false);

    if (canvasOpaChooser != NULL)
    {
      canvasOpaChooser->Refresh();
    }
  }
}


void DiaGraph::handleDOFOpaAdd(
  const double& opa,
  const double& y)
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleDOFOpaAdd(opa, y);
  }
}


void DiaGraph::handleDOFOpaUpdate(
  const size_t& idx,
  const double& opa,
  const double& y)
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleDOFOpaUpdate(idx, opa, y);
  }
}


void DiaGraph::handleDOFOpaClear(
  const size_t& idx)
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->handleDOFOpaClear(idx);
  }
}


void DiaGraph::handleDOFOpaSetValuesEdt(
  const vector< double > &opa,
  const vector< double > &y)
{
  if (opaChooser != NULL)
  {
    opaChooser->setPoints(opa, y);
  }
}


void DiaGraph::handleLinkDOFAttr(
  const size_t DOFIdx,
  const size_t attrIdx)
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    /*
    if ( graph->getAttribute( attrIdx )->getSizeCurValues() == 0 )
        wxLogError(
            wxString( "Error linking DOF.\nAt least one attribute has an undefined or real-valued domain." ) );
    else
    */
    editor->setLinkDOFAttr(DOFIdx, attrIdx);
  }
}


void DiaGraph::handleUnlinkDOFAttr(const size_t DOFIdx)
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->clearLinkDOFAttr(DOFIdx);
  }
}


void DiaGraph::handleDOFFrameDestroy()
{
  if (mode == MODE_EDIT && editor != NULL)
  {
    editor->setEditModeSelect();
    editor->deselectAll();
    frame->setEditModeSelect();
  }

  // color chooser
  canvasColChooser = NULL;
  delete colChooser;
  colChooser = NULL;

  // opacity chooser
  canvasOpaChooser = NULL;
  delete opaChooser;
  opaChooser = NULL;
}


void DiaGraph::handleDOFDeselect()
{
  frame->clearDOFInfo();
}


// -- simulator, time series & examiner -----------------------------

/*
void DiaGraph::handleSetSimCurrFrame(
    Cluster* frame,
    const vector< Attribute* > &attrs )
{
    if ( simulator != NULL )
    {
        simulator->setFrameCurr( frame, attrs );

        if ( mode == MODE_ANALYSIS && canvasPrev != NULL )
            canvasPrev->Refresh();
        if ( mode == MODE_ANALYSIS && canvasCurr != NULL )
            canvasCurr->Refresh();
        if ( mode == MODE_ANALYSIS && canvasNext != NULL )
            canvasNext->Refresh();
        if ( mode == MODE_ANALYSIS && canvasHist != NULL )
            canvasHist->Refresh();
    }
}
*/

void DiaGraph::initSimulator(
  Cluster* currFrame,
  const vector< Attribute* > &attrs)
{
  if (simulator != NULL)
  {
    simulator->initFrameCurr(currFrame, attrs);

    if (mode == MODE_ANALYSIS && canvasSiml != NULL)
    {
      canvasSiml->Refresh();
    }
  }
}


void DiaGraph::initTimeSeries(const vector< size_t > attrIdcs)
{
  if (timeSeries != NULL)
  {
    timeSeries->initAttributes(attrIdcs);
  }

  if (view == VIEW_TRACE && (mode == MODE_ANALYSIS && canvasTrace != NULL))
  {
    canvasTrace->Refresh();
  }
}


void DiaGraph::markTimeSeries(
  Colleague* /*sender*/,
  Cluster* currFrame)
{
  if (timeSeries != NULL)
  {
    timeSeries->markItems(currFrame);
    handleMarkFrameClust(timeSeries);

    if (mode == MODE_ANALYSIS && (view == VIEW_TRACE && canvasTrace != NULL))
    {
      canvasArcD->Refresh();
      canvasTrace->Refresh();
    }
  }
}


void DiaGraph::markTimeSeries(
  Colleague* /*sender*/,
  const vector< Cluster* > frames)
{
  if (timeSeries != NULL)
  {
    timeSeries->markItems(frames);
    handleMarkFrameClust(timeSeries);

    if (mode == MODE_ANALYSIS && (view == VIEW_TRACE && canvasTrace != NULL))
    {
      canvasTrace->Refresh();
    }
  }
}


void DiaGraph::addToExaminer(
  Cluster* currFrame,
  const vector< Attribute* > &attrs)
{
  if (examiner != NULL)
  {
    examiner->addFrameHist(currFrame, attrs);

    if (mode == MODE_ANALYSIS && canvasExnr != NULL)
    {
      canvasExnr->Refresh();
    }
  }
}


void DiaGraph::addToExaminer(
  const vector< Cluster* > frames,
  const vector< Attribute* > &attrs)
{
  if (examiner != NULL)
  {
    for (size_t i = 0; i < frames.size(); ++i)
    {
      examiner->addFrameHist(frames[i], attrs);
    }

    if (mode == MODE_ANALYSIS && canvasExnr != NULL)
    {
      canvasExnr->Refresh();
    }
  }
}


void DiaGraph::handleShowClusterMenu()
{
  frame->displClusterMenu();
}


void DiaGraph::handleSendDgrm(
  Colleague* sender,
  const bool& sendSglToSiml,
  const bool& sendSglToTrace,
  const bool& sendSetToTrace,
  const bool& sendSglToExnr,
  const bool& sendSetToExnr)
{
  dgrmSender = sender;

  frame->displDgrmMenu(
    sendSglToSiml,
    sendSglToTrace,
    sendSetToTrace,
    sendSglToExnr,
    sendSetToExnr);
}


void DiaGraph::handleSendDgrmSglToSiml()
{
  if (dgrmSender == arcDgrm)
  {
    arcDgrm->handleSendDgrmSglToSiml();
  }
  else if (dgrmSender == simulator)
  {
    *this << "Simulator sending single to siml\n";
  }
  else if (dgrmSender == examiner)
  {
    examiner->handleSendDgrmSglToSiml();
  }
}


void DiaGraph::handleSendDgrmSglToTrace()
{
  if (dgrmSender == arcDgrm)
  {
    arcDgrm->handleSendDgrmSglToTrace();
  }
  else if (dgrmSender == examiner)
  {
    examiner->handleSendDgrmSglToTrace();
  }
}


void DiaGraph::handleSendDgrmSetToTrace()
{
  if (dgrmSender == arcDgrm)
  {
    arcDgrm->handleSendDgrmSetToTrace();
  }
  else if (dgrmSender == examiner)
  {
    examiner->handleSendDgrmSetToTrace();
  }
}


void DiaGraph::handleSendDgrmSglToExnr()
{
  if (dgrmSender == arcDgrm)
  {
    arcDgrm->handleSendDgrmSglToExnr();
  }
  else if (dgrmSender == simulator)
  {
    simulator->handleSendDgrmSglToExnr();
  }
  else if (dgrmSender == timeSeries)
  {
    timeSeries->handleSendDgrmSglToExnr();
  }
}


void DiaGraph::handleSendDgrmSetToExnr()
{
  if (dgrmSender == arcDgrm)
  {
    arcDgrm->handleSendDgrmSetToExnr();
  }
}


void DiaGraph::handleClearSim(Colleague* sender)
{
  if (sender == simulator)
  {
    frame->displSimClearDlg();
  }
  else if (sender == frame)
  {
    if (arcDgrm != NULL)
    {
      if (mode == MODE_ANALYSIS && canvasArcD != NULL)
      {
        canvasArcD->Refresh();
      }
    }

    if (simulator != NULL)
    {
      simulator->clearData();

      if (mode == MODE_ANALYSIS && canvasSiml != NULL)
      {
        canvasSiml->Refresh();
      }
    }
  }
}


void DiaGraph::handleClearExnr(Colleague* sender)
{
  if (sender == examiner)
  {
    frame->displExnrClearDlg();
  }
  else if (sender == frame)
  {
    if (arcDgrm != NULL)
    {
      if (mode == MODE_ANALYSIS && canvasArcD != NULL)
      {
        canvasArcD->Refresh();
      }
    }

    if (examiner != NULL)
    {
      examiner->clrFrameHist();

      if (mode == MODE_ANALYSIS && canvasExnr != NULL)
      {
        canvasExnr->Refresh();
      }
    }
  }
}


void DiaGraph::handleClearExnrCur(Colleague* sender)
{
  if (sender == examiner)
  {
    frame->displExnrFrameMenu(true);
  }
  else if (sender == frame)
  {
    if (examiner != NULL)
    {
      examiner->clrFrameHistCur();

      if (mode == MODE_ANALYSIS && canvasExnr != NULL)
      {
        canvasExnr->Refresh();
      }
    }
  }
}

/*
void DiaGraph::handleAnimFrameBundl( Colleague* sender )
{
    if ( arcDgrm != NULL && canvasArcD != NULL )
    {
        arcDgrm->unmarkLeaves();
        if ( sender == timeSeries )
        {
            int idx;
            set< int > idcs;
            QColor col;

            timeSeries->getAnimIdxDgrm( idxLeaf, idxBndl, col );

//            arcDgrm->unmarkLeaves();
//            arcDgrm->unmarkBundles();
//            arcDgrm->markBundle( idxBndl );
//            canvasArcD->Refresh();
        }
    }
}
*/

void DiaGraph::handleAnimFrameClust(Colleague* sender)
{
  if (arcDgrm != NULL && canvasArcD != NULL)
  {
    arcDgrm->unmarkLeaves();
    if (sender == timeSeries)
    {
      size_t idx;
      set< size_t > idcs;
      QColor col;

      timeSeries->getAnimIdxDgrm(idx, idcs, col);

      arcDgrm->unmarkBundles();
      arcDgrm->unmarkLeaves();
      for (set< size_t >::iterator it = idcs.begin(); it != idcs.end(); ++it)
      {
        arcDgrm->markBundle(*it);
      }
      arcDgrm->markLeaf(idx, col);

      canvasArcD->Refresh();
    }
  }
}


void DiaGraph::handleMarkFrameClust(Colleague* sender)
{
  if (arcDgrm != NULL)
  {
    arcDgrm->unmarkLeaves();
    if (sender == simulator)
    {
      QColor col = simulator->SelectColor();
      size_t      idx = simulator->SelectedClusterIndex();

      arcDgrm->markLeaf(idx, col);
    }
    else if (sender == timeSeries)
    {
      QColor col;
      set< size_t > idcs;
      size_t idx;

      // clear bundles
      arcDgrm->unmarkBundles();

      // marked nodes
      timeSeries->getIdcsClstMarked(idcs, col);
      for (set< size_t >::iterator it = idcs.begin(); it != idcs.end(); ++it)
      {
        arcDgrm->markLeaf(*it, col);
      }

      // mouse over
      timeSeries->getIdxMseOver(idx, idcs, col);
      if (idx != NON_EXISTING)
      {
        arcDgrm->markLeaf(idx, col);
        for (set< size_t >::iterator it = idcs.begin(); it != idcs.end(); ++it)
        {
          arcDgrm->markBundle(*it);
        }
      }

      // current diagram
      timeSeries->getCurrIdxDgrm(idx, idcs, col);
      if (idx != NON_EXISTING)
      {
        arcDgrm->markLeaf(idx, col);
        for (set< size_t >::iterator it = idcs.begin(); it != idcs.end(); ++it)
        {
          arcDgrm->markBundle(*it);
        }
      }

      // examiner view
      idx = examiner->getIdxClstSel();
      if (idx != NON_EXISTING)
      {
        col = examiner->getColorSel();
        arcDgrm->markLeaf(idx, col);
      }

    }
    else if (sender == examiner)
    {
      QColor col;
      size_t      idx;

      // trace view
      if (view == VIEW_TRACE)
      {
        set< size_t > idcs;
        timeSeries->getIdcsClstMarked(idcs, col);
        for (set< size_t >::iterator it = idcs.begin(); it != idcs.end(); ++it)
        {
          arcDgrm->markLeaf(*it, col);
        }
      }

      // examiner view
      col = examiner->getColorSel();
      idx = examiner->getIdxClstSel();
      arcDgrm->markLeaf(idx, col);
    }
  }
}


void DiaGraph::handleUnmarkFrameClusts(Colleague* sender)
{
  if (arcDgrm != NULL)
  {
    arcDgrm->unmarkLeaves();
    if (sender == simulator)
    {
      QColor col = examiner->getColorSel();
      size_t      idx = examiner->getIdxClstSel();

      arcDgrm->markLeaf(idx, col);
    }
    else if (sender == examiner)
    {
      // trace view
      if (view == VIEW_TRACE)
      {
        QColor col;
        set< size_t > idcs;
        timeSeries->getIdcsClstMarked(idcs, col);

        for (set< size_t >::iterator it = idcs.begin(); it != idcs.end(); ++it)
        {
          arcDgrm->markLeaf(*it, col);
        }
      }
    }

    if (mode == MODE_ANALYSIS && canvasArcD != NULL)
    {
      canvasArcD->Refresh();
    }
  }
}

void DiaGraph::handleShowFrame(
  Cluster* frame,
  const vector< Attribute* > &attrs,
  QColor col)
{
  if (examiner != NULL)
  {
    examiner->setFrame(frame, attrs, col);
    if (mode == MODE_ANALYSIS && canvasExnr != NULL)
    {
      canvasExnr->Refresh();
    }
  }
}


void DiaGraph::handleUnshowFrame()
{
  if (examiner != NULL)
  {
    examiner->clrFrame();
    if (mode == MODE_ANALYSIS && canvasExnr != NULL)
    {
      canvasExnr->Refresh();
    }
  }
}


// -- visualization settings ----------------------------------------


void DiaGraph::getGridCoordinates(double& xLeft, double& xRight, double& yTop, double& yBottom)
{
  if (editor != NULL)
  {
    editor->getDiagram()->getGridCoordinates(xLeft, xRight, yTop, yBottom);
  }
}


// -- visualization -------------------------------------------------


void DiaGraph::handlePaintEvent(GLCanvas* c)
{
  if (critSect != true)
  {
    if (mode == MODE_EDIT)
    {
      // draw in render mode
      if (c == canvasEdit && editor != NULL)
      {
        editor->updateGL();
      }
      else if (c == canvasColChooser && colChooser != NULL)
      {
        colChooser->updateGL();
      }
      else if (c == canvasOpaChooser && opaChooser != NULL)
      {
        opaChooser->updateGL();
      }
      else if (c != NULL)
      {
        c->clear();
      }
    }
    else if (mode == MODE_ANALYSIS)
    {
      // draw in render mode
      if (c == canvasArcD && arcDgrm != NULL)
      {
        arcDgrm->updateGL();
      }
      else if (view == VIEW_SIM && (c == canvasSiml && simulator != NULL))
      {
        simulator->updateGL();
      }
      else if (view == VIEW_TRACE && (c == canvasTrace && timeSeries != NULL))
      {
        timeSeries->updateGL();
      }
      else if (c == canvasExnr && examiner != NULL)
      {
        examiner->updateGL();
      }
      else if (c == canvasDistr && distrPlot != NULL)
      {
        distrPlot->updateGL();
      }
      else if (c == canvasCorrl && corrlPlot != NULL)
      {
        corrlPlot->updateGL();
      }
      else if (c == canvasCombn && combnPlot != NULL)
      {
        combnPlot->updateGL();
      }
      else if (c!= NULL)
      {
        c->clear();
      }
    }
  }
}


void DiaGraph::handleSizeEvent(GLCanvas* c)
{
  if (mode == MODE_EDIT)
  {
    // draw in render mode
    if (c == canvasEdit && editor != NULL)
    {
      editor->handleSizeEvent();
    }
  }
  else if (mode == MODE_ANALYSIS)
  {
    // draw in render mode
    if (c == canvasArcD && arcDgrm != NULL)
    {
      arcDgrm->handleSizeEvent();
    }
    else if (view == VIEW_SIM && (c == canvasSiml && simulator != NULL))
    {
      simulator->handleSizeEvent();
    }
    else if (view == VIEW_TRACE && (c == canvasTrace && timeSeries != NULL))
    {
      timeSeries->handleSizeEvent();
    }
    else if (c == canvasExnr && examiner != NULL)
    {
      examiner->handleSizeEvent();
    }
    else if (c == canvasDistr && distrPlot != NULL)
    {
      distrPlot->handleSizeEvent();
    }
    else if (c == canvasCorrl && corrlPlot != NULL)
    {
      corrlPlot->handleSizeEvent();
    }
    else if (c == canvasCombn && combnPlot != NULL)
    {
      combnPlot->handleSizeEvent();
    }
  }
}


void DiaGraph::updateDependancies(GLCanvas* c)
{
  if (mode == MODE_ANALYSIS)
  {
    if (c == canvasSiml)
    {
      canvasArcD->Refresh();
    }
  }
}


// -- input event handlers --------------------------------------


void DiaGraph::handleDragDrop(
  const int& srcWindowId,
  const int& tgtWindowId,
  const int& tgtX,
  const int& tgtY,
  const vector< int > &data)
{
  frame->handleDragDrop(
    srcWindowId,
    tgtWindowId,
    tgtX,
    tgtY,
    data);
}

Visualizer* DiaGraph::currentVisualizer(GLCanvas* c)
{

  if (mode == MODE_EDIT)
  {
    if (c == canvasEdit && editor != NULL)
    {
      return editor;
    }
    else if (c == canvasColChooser && colChooser != NULL)
    {
      return colChooser;
    }
    else if (c == canvasOpaChooser && opaChooser != NULL)
    {
      return opaChooser;
    }
  }
  else if (mode == MODE_ANALYSIS)
  {
    if (c == canvasArcD && arcDgrm != NULL)
    {
      return arcDgrm;
    }
    else if (view == VIEW_SIM && (c == canvasSiml && simulator != NULL))
    {
      return simulator;
    }
    else if (view == VIEW_TRACE && (c == canvasTrace && timeSeries != NULL))
    {
      return timeSeries;
    }
    else if (c == canvasExnr && examiner != NULL)
    {
      return examiner;
    }
  }

  if (c == canvasDistr && distrPlot != NULL)
  {
    return distrPlot;
  }
  else if (c == canvasCorrl && corrlPlot != NULL)
  {
    return corrlPlot;
  }
  else if (c == canvasCombn && combnPlot != NULL)
  {
    return combnPlot;
  }

  return NULL;
}


void DiaGraph::handleMouseEvent(GLCanvas* c, QMouseEvent* e)
{
  Visualizer *current = currentVisualizer(c);
  if (current != NULL)
  {
    current->handleMouseEvent(e);

    if ((e->type() == QEvent::MouseMove || e->button() == Qt::LeftButton) &&
        (current == simulator || current == timeSeries || current == examiner))
    {
      canvasArcD->Refresh();
    }

    if ((e->type() == QEvent::MouseMove || e->button() == Qt::LeftButton) &&
        (current == arcDgrm || current == simulator || current == timeSeries))
    {
      canvasExnr->Refresh();
    }

    if (e->type() == QEvent::MouseButtonRelease && e->button() == Qt::LeftButton &&
        current == editor && editor->getEditMode() != DiagramEditor::EDIT_MODE_DOF)
    {
      editor->setEditModeSelect();
      frame->setEditModeSelect();
    }
  }
}

void DiaGraph::handleWheelEvent(GLCanvas* c, QWheelEvent* e)
{
  Visualizer *current = currentVisualizer(c);
  if (current != NULL)
  {
    current->handleWheelEvent(e);
  }
}

void DiaGraph::handleMouseEnterEvent(GLCanvas* c)
{
  Visualizer *current = currentVisualizer(c);
  if (current != NULL)
  {
    current->handleMouseEnterEvent();
  }
}

void DiaGraph::handleMouseLeaveEvent(GLCanvas* c)
{
  Visualizer *current = currentVisualizer(c);
  if (current != NULL)
  {
    current->handleMouseLeaveEvent();
  }
}

void DiaGraph::handleKeyEvent(GLCanvas* c, QKeyEvent* e)
{
  Visualizer *current = currentVisualizer(c);
  if (current != NULL)
  {
    current->handleKeyEvent(e);

    if (e->type() == QEvent::KeyPress && (current == simulator || current == timeSeries || current == examiner))
    {
      canvasArcD->Refresh();
    }

    if (e->type() == QEvent::KeyPress && (current == arcDgrm || current == simulator || current == timeSeries))
    {
      canvasExnr->Refresh();
    }
  }
}


// -- overloaded operators --------------------------------------


void DiaGraph::operator<<(const std::string& msg)
{
  this->appOutputText(msg);
}


void DiaGraph::operator<<(const int& msg)
{
  this->appOutputText(msg);
}

void DiaGraph::operator<<(const size_t& msg)
{
  this->appOutputText(msg);
}

// -- protected functions inhereted from Mediator -------------------


void DiaGraph::initColleagues()
{
  // init graph
  graph = NULL;

  // init frame
  frame = new Frame(
    this,
    &settings,
    wxT("DiaGraphica"));
  // show frame
  frame->Show(TRUE);
  this->SetTopWindow(frame);

  *frame << "Welcome to DiaGraphica.\n";

  // init progress dialog
  progressDialog = NULL;

  // init visualizations
  canvasArcD  = frame->getCanvasArcD();
  arcDgrm     = NULL;

  canvasSiml  = frame->getCanvasSiml();
  simulator   = NULL;

  canvasTrace = frame->getCanvasTrace();
  timeSeries  = NULL;

  canvasExnr  = frame->getCanvasExnr();
  examiner    = NULL;

  canvasEdit  = frame->getCanvasEdit();
  editor      = NULL;

  canvasDistr = NULL;
  distrPlot   = NULL;
  canvasCorrl = NULL;
  corrlPlot   = NULL;
  canvasCombn = NULL;
  combnPlot   = NULL;

  canvasColChooser = NULL;
  colChooser       = NULL;
  canvasOpaChooser = NULL;
  opaChooser       = NULL;

  dgrmSender = NULL;

  tempAttr = NULL;
}


void DiaGraph::clearColleagues()
{
  // composition
  if (graph != NULL)
  {
    delete graph;
    graph = NULL;
  }

  // composition
  if (progressDialog != NULL)
  {
    delete progressDialog;
    progressDialog = NULL;
  }

  // association
  canvasArcD = NULL;
  // composition
  if (arcDgrm != NULL)
  {
    delete arcDgrm;
    arcDgrm = NULL;
  }

  // association
  canvasSiml = NULL;
  // composition
  if (simulator != NULL)
  {
    delete simulator;
    simulator = NULL;
  }

  // association
  canvasTrace = NULL;
  // composition
  if (timeSeries != NULL)
  {
    delete timeSeries;
    timeSeries = NULL;
  }

  // associatioin
  canvasExnr = NULL;
  // composition
  if (examiner != NULL)
  {
    delete examiner;
    examiner = NULL;
  }

  // association
  canvasEdit = NULL;
  // composition
  if (editor != NULL)
  {
    delete editor;
    editor = NULL;
  }

  // association
  canvasDistr = NULL;
  // composition
  if (distrPlot != NULL)
  {
    delete distrPlot;
    distrPlot = NULL;
  }

  // association
  canvasCorrl = NULL;
  // composition
  if (corrlPlot != NULL)
  {
    delete corrlPlot;
    corrlPlot = NULL;
  }

  // association
  canvasCombn = NULL;
  // composition
  if (combnPlot != NULL)
  {
    delete combnPlot;
    combnPlot = NULL;
  }
}


void DiaGraph::displAttributes()
{
  Attribute*       attr;
  vector< size_t >    indcs;
  vector< std::string > names;
  vector< std::string > types;
  vector< size_t >    cards;
  vector< std::string > range;
  for (size_t i = 0; i < graph->getSizeAttributes(); ++i)
  {
    attr = graph->getAttribute(i);

    indcs.push_back(attr->getIndex());
    names.push_back(attr->getName());
    types.push_back(attr->getType());
    cards.push_back(attr->getSizeCurValues());
    range.push_back("");
  }
  frame->displAttrInfo(indcs, names, types, cards, range);

  attr = NULL;
}


void DiaGraph::displAttributes(const size_t& selAttrIdx)
{
  Attribute*       attr;
  vector< size_t >    indcs;
  vector< std::string > names;
  vector< std::string > types;
  vector< size_t >    cards;
  vector< std::string > range;
  for (size_t i = 0; i < graph->getSizeAttributes(); ++i)
  {
    attr = graph->getAttribute(i);

    indcs.push_back(attr->getIndex());
    names.push_back(attr->getName());
    types.push_back(attr->getType());
    cards.push_back(attr->getSizeCurValues());
    range.push_back("");
  }
  frame->displAttrInfo(selAttrIdx, indcs, names, types, cards, range);

  attr = NULL;
}


void DiaGraph::displAttrDomain(const size_t& attrIdx)
{
  Attribute* attribute;
  size_t        numValues;
  size_t        numNodes;
  vector< size_t >    indices;
  vector< std::string > values;
  vector< size_t >    number;
  vector< double>  perc;

  attribute = graph->getAttribute(attrIdx);
  numValues = attribute->getSizeCurValues();
  numNodes  = graph->getSizeNodes();

  // update indices and values
  for (size_t i = 0; i < numValues; ++i)
  {
    indices.push_back(attribute->getCurValue(i)->getIndex());
    values.push_back(attribute->getCurValue(i)->getValue());
  }

  // get number of nodes
  graph->calcAttrDistr(
    attrIdx,
    number);

  // calc perc
  {
    for (size_t i = 0; i < numValues; ++i)
    {
      perc.push_back(Utils::perc((double) number[i], (double) numNodes));
    }
  }

  // display domain
  frame->displDomainInfo(indices, values, number, perc);

  // reset ptr
  attribute = NULL;
}


void DiaGraph::clearAttrDomain()
{}


// -- end -----------------------------------------------------------