/*
  Simple molecular-order parameters for comparing CG to AA MD
*/

/*
  This file is part of LOOS.

  LOOS (Lightweight Object-Oriented Structure library)
  Copyright (c) 2011, Tod D. Romo and Alan Grossfield
  Department of Biochemistry and Biophysics
  School of Medicine & Dentistry, University of Rochester

  This package (LOOS) is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation under version 3 of the License.

  This package is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/


#include <loos.hpp>

using namespace std;
using namespace loos;

namespace opts = loos::OptionsFramework;
namespace po = loos::OptionsFramework::po;


typedef vector<string>      vString;
typedef vector<double>      vecDbl;
typedef vector<AtomicGroup> vGroup;
typedef vector<vGroup>      vvGroup;


const double minp = 1e-30;
ulong nplanar = 0;
ulong ntotal = 0;

string progname("unknown");

// @cond TOOLS_INTERNAL


string fullHelpMessage(void) {
  string msg =
    "\n"
    "SYNOPSIS\n"
    "Mops calculates a molecular-order parameter for a selection\n"
    "\n"
    "DESCRIPTION\n"
    "\tMops is used to calculate an order parameter using a whole molecule.\n"
    "This is used in cases such as coarse-grained simulations where there are\n"
    "no hydrogens.  For each molecule in the selection, the principal axes are\n"
    "found.  The 2nd and 3rd axes are treated as faux-hydrogens and their angle\n"
    "with the z-axis is used to calculate an order parameter (as in order_params).\n"
    "\n"
    "The order parameters can be written out as a time-series.  If multiple trajectories\n"
    "are given, then there will be extra spaces between each trajectory in the output.\n"
    "Individual trajectories can be plotted with gnuplot by using the 'index' keyword.\n"
    "\n"
    "Use the --residues=1 option to force splitting the selection by residue rather than\n"
    "connectivity or unique segid.\n"
    "\n"
    "EXAMPLES\n"
    "\tmops 'resname == \"POPC\"' model.gro simulation.xtc >order.asc\n"
    "This computes a molecular order parameter for all POPC residues.\n"
    "\n"
    "\tmops --skip=50 'segid == \"LIPID\"' namd.psf sim1.dcd sim2.dcd sim3.dcd sim4.dcd >order.asc\n"
    "This computes a molecular order parameter for all molecules with a LIPID segid,\n"
    "averaging over all 4 trajectories.  The first 50 frames of each trajectory are\n"
    "skipped.\n"
    "\n"
    "NOTES\n"
    "\tThe entire selected molecule is used for the order parameter.  You will\n"
    "probably want to restrict it to a single chain for comparison with order_params,\n"
    "e.g. select only a palmitoyl or oleoyl chain...\n"
    "\n"
    "SEE ALSO\n"
    "\torder_params\n";
  

  return(msg);
}



struct ToolOptions : public opts::OptionsPackage 
{
 
  void addGeneric(po::options_description& o) 
  {
    o.add_options()
      ("skip", po::value<uint>(&skip)->default_value(0), "Skip these frames at the start of each trajectory")
      ("residue", po::value<bool>(&residue_split)->default_value(false), "Force split by residue")
      ("timeseries", po::value<bool>(&timeseries)->default_value(false), "Write out time-series of MOPS");

  }

  void addHidden(po::options_description& o) 
  {
    o.add_options()
      ("selection", po::value<string>(&selection), "Atoms to use")
      ("model", po::value<string>(&model_name), "Model filename")
      ("traj", po::value< vector<string> >(&traj_names), "Trajectory filenames");
    
  }

  void addPositional(po::positional_options_description& o) 
  {
    o.add("selection", 1);
    o.add("model", 1);
    o.add("traj", -1);
  }
  

  bool check(po::variables_map& vm) 
  {
    return( selection.empty() || model_name.empty() || traj_names.empty() );
  }

  string help() const 
  {
    return("selection model trajectory [trajectory ...]");
  }
  

  string print() const 
  {
    ostringstream oss;
    oss << boost::format("skip=%d, selection='%s', residue=%d, timeseries=%d, model='%s', traj='%s'")
      % skip
      % selection
      % residue_split
      % timeseries
      % model_name
      % vectorAsStringWithCommas(traj_names);
    

    return(oss.str());
  }

  uint skip;
  bool residue_split;
  bool timeseries;
  string selection;
  string model_name;
  vector<string> traj_names;
};


// @endcond



double rowStats(const RealMatrix& M, const uint row, double *std) {
  double avg = 0.0;
  for (uint i=0; i<M.cols(); ++i)
    avg += M(row, i);
  avg /= M.cols();
  
  double var = 0.0;
  for (uint i=0; i<M.cols(); ++i) {
    double d = M(row, i) - avg;
    var += d*d;
  }
  var /= (M.cols() - 1);

  *std = sqrt(var/M.cols());
  return(avg);
}


ulong calculateSize(AtomicGroup& model, const vString& names) {
  ulong n = 0;
  for (vString::const_iterator i = names.begin(); i != names.end(); ++i) {
    pTraj traj = createTrajectory(*i, model);
    n += traj->nframes();
  }
  return(n);
}


// Note: hist is really an average binned on distance
void principalComponentsOrder(dTimeSeries& order_parameters,
                              const vGroup& residues) {

  for (vGroup::const_iterator i = residues.begin(); i != residues.end(); ++i) {
    AtomicGroup residue = i->copy();
    residue.centerAtOrigin();
    residue.mergeImage();
    vector<GCoord> axes = residue.principalAxes();
    bool planar = false;

    if (abs(axes[3].z()) < minp) {
      if (nplanar == 0) {
        PDB pdb = PDB::fromAtomicGroup(residue);
        cerr << progname << ": Warning- PCA magnitudes out of bounds " << axes[3] << endl;
        cerr << pdb;
      }
      planar = true;
      ++nplanar;
    }

    double order1 = 0.5 - 1.5 * axes[1].z() * axes[1].z();
    double order2 = 0.5 - 1.5 * axes[2].z() * axes[2].z();

    order_parameters.push_back(order1);
    ++ntotal;
    if (!planar) {
      order_parameters.push_back(order2);
      ++ntotal;
    }
  }
}



vGroup extractSelections(const AtomicGroup& model, const string& selection, const bool force_residues) {
  AtomicGroup subset = selectAtoms(model, selection);
  
  if (subset.empty()) {
    cerr << "Error- no atoms were selected.\n";
    exit(EXIT_FAILURE);
  }

  vGroup residues;
  if (force_residues) {
    cerr << progname << ": Forcing split by residue\n";
    residues = subset.splitByResidue();
  } else {
    if (subset.hasBonds()) {
      cerr << progname << ": Model has connectivity.  Using this to split selection.\n";
      residues = subset.splitByMolecule();
    } else {
      residues = subset.splitByUniqueSegid();
    }
  }

  if (!force_residues && residues[0].size() == subset.size()) {
    cerr << progname << ": GROMACS model suspected... Splitting by residue.\n";
    residues = subset.splitByResidue();
  }

  cerr << boost::format("%s: Extracted %d molecules from selection.\n") % progname % residues.size();
  return(residues);
}



int main(int argc, char *argv[]) {
  

  progname = string(argv[0]);

  string hdr = invocationHeader(argc, argv);
  opts::BasicOptions* bopts = new opts::BasicOptions(fullHelpMessage());
  ToolOptions* topts = new ToolOptions;

  opts::AggregateOptions options;
  options.add(bopts).add(topts);
  if (!options.parse(argc, argv))
    exit(-1);


  uint skip = topts->skip;
  string selection = topts->selection;
  AtomicGroup model = createSystem(topts->model_name);
  vGroup subset = extractSelections(model, selection, topts->residue_split);
  vString traj_names = topts->traj_names;

  cout << "# " << hdr << endl;



  // Calculate size of matrix to use...
  ulong n = calculateSize(model, traj_names);
  n -= traj_names.size() * skip;


  dTimeSeries order;

  uint file = 0;

  for (vString::const_iterator i = traj_names.begin(); i != traj_names.end(); ++i) {
    dTimeSeries suborder;

    pTraj traj = createTrajectory(*i, model);
    if (skip != 0)
      traj->readFrame(skip-1);

    uint t = skip;
    while (traj->readFrame()) {
      dTimeSeries frameorder;
      traj->updateGroupCoords(model);
      principalComponentsOrder(frameorder, subset);
      copy(frameorder.begin(), frameorder.end(), back_inserter(suborder));
      if (topts->timeseries)
        cout << file << '\t' << t++ << '\t' << frameorder.average() << '\t' << '\t' << frameorder.stdev() << endl;
    }
    if (topts->timeseries)
      cout << endl << endl;
    order.push_back(suborder.average());
    ++file;
  }

  
  if (topts->timeseries) {
    cerr << boost::format("%s: Average MOP = %f\n") % progname % order.average();
    cout << "# Avg = " << order.average() << endl;
    cout << "# Std = " << order.stdev() << endl;
    cerr << boost::format("%s: OB Data = %d out of %d (%.2f%%)\n") % progname % nplanar % ntotal % (nplanar * 100.0 / ntotal);
  } else {
    cout << "Avg = " << order.average() << endl;
    cout << "Std = " << order.stdev() << endl;
    cout << boost::format("OB Data = %d out of %d (%.2f%%)\n") % nplanar % ntotal % (nplanar * 100.0 / ntotal);
  }
  exit(EXIT_SUCCESS);
}
