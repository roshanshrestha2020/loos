/*
  hcontacts.cpp

  Constructs a matrix representing time series for multiple for inter and/or intra-moelcular hbonds
*/

/*

  This file is part of LOOS.

  LOOS (Lightweight Object-Oriented Structure library)
  Copyright (c) 2010, Tod D. Romo
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
#include <boost/format.hpp>
#include <boost/program_options.hpp>

#include "hcore.hpp"

using namespace std;
using namespace loos;
namespace po = boost::program_options;
namespace opts = loos::OptionsFramework;


typedef vector<AtomicGroup>          vGroup;

typedef pair<SimpleAtom, SimpleAtom>   Bond;
typedef vector<Bond>                  vBond;


bool inter_bonds, intra_bonds;
double putative_threshold;

double length_low, length_high, max_angle;
bool use_periodicity;
string donor_selection, acceptor_selection;
string model_name, traj_name;

// @cond TOOLS_INTERNAL



// Force matrix elements to written out as unsigned ints...

struct FormatCharAsInteger {

  string operator()(const unsigned char& c) {
    std::ostringstream oss;
    oss << static_cast<unsigned int>(c);
    return(oss.str());
  }

};




class ToolOptions : public opts::OptionsPackage {
public:
  void addGeneric(po::options_description& o) {
    o.add_options()
      ("search", po::value<double>(&putative_threshold)->default_value(10.0), "Threshold for initial bond search")
      ("blow", po::value<double>(&length_low)->default_value(1.5), "Low cutoff for bond length")
      ("bhi", po::value<double>(&length_high)->default_value(3.0), "High cutoff for bond length")
      ("angle", po::value<double>(&max_angle)->default_value(30.0), "Max bond angle deviation from linear")
      ("periodic", po::value<bool>(&use_periodicity)->default_value(false), "Use periodic boundary")
      ("inter", po::value<bool>(&inter_bonds)->default_value(true), "Inter-molecular bonds")
      ("intra", po::value<bool>(&intra_bonds)->default_value(true), "Intra-molecular bonds");
  }

  void addHidden(po::options_description& o) {
    o.add_options()
      ("donor", po::value<string>(&donor_selection), "donor selection")
      ("acceptor", po::value<string>(&acceptor_selection), "acceptor selection");

  }

  void addPositional(po::positional_options_description& opts) {
    opts.add("donor", 1);
    opts.add("acceptor", 1);
  }

  
  bool check(po::variables_map& map) {
    if (!(inter_bonds || intra_bonds)) {
      cerr << "Error- must specify at least some kind of bond (inter/intra) to calculate.\n";
      return(true);
    }
    return(false);
  }


  string help() const {
    return("donor-selection acceptor-selection");
  }

  string print() const {
    ostringstream oss;
    oss << boost::format("search=%f,inter=%d,intra=%d,blow=%f,bhi=%f,angle=%f,periodic=%d,acceptor=\"%s\",donor=\"%s\"")
      % putative_threshold
      % inter_bonds
      % intra_bonds
      % length_low
      % length_high
      % max_angle
      % use_periodicity
      % acceptor_selection
      % donor_selection;

    return(oss.str());
  }

};


// @endcond




// Given a vector of molecules, apply selection to each return a vector of subsets

vGroup splitSelection(const vGroup& molecules, const string& selection) {
  vGroup results;
 
  for (vGroup::const_iterator i = molecules.begin(); i != molecules.end(); ++i) {
    AtomicGroup subset;
    try {
      subset = selectAtoms(*i, selection);
    }
    catch (...) {     // Ignore exceptions
      ;
    }
    if (!subset.empty())
      results.push_back(subset);
  }

  
  if (results.empty()) {
    cerr << "Error- The selection '" << selection << "' resulted in nothing being selected.\n";
    exit(-1);
  }
   
  return(results);
}





// Build up a vector of Bonds by looking for any donor/acceptor pair that's within a threshold
// distance

vBond findPotentialBonds(const AtomicGroup& donors, const AtomicGroup& acceptors, const AtomicGroup& system) {
  vBond bonds;

  for (AtomicGroup::const_iterator j = donors.begin(); j != donors.end(); ++j) {
    GCoord u = (*j)->coords();
    for (AtomicGroup::const_iterator i = acceptors.begin(); i != acceptors.end(); ++i) {
      if (u.distance((*i)->coords()) <= putative_threshold) {

        // Manually build simple atoms
        SimpleAtom new_donor(*j, system.sharedPeriodicBox(), use_periodicity);
        string name = (*j)->name();
        if (name[0] != 'H') {
          cerr << boost::format("Error- atom %s was given as a donor, but donors can only be hydrogens.\n") % name;
          exit(-10);
        }
        
        vector<int> bond_list = (*j)->getBonds();
        if (bond_list.size() != 1) {
          cerr << "Error- The following hydrogen atom has more than one bond to it...woops...\n";
          cerr << *j;
          exit(-10);
        }

        pAtom pa = system.findById(bond_list[0]);
        if (pa == 0) {
          cerr << boost::format("Error- cannot find atomid %d in system.\n") % bond_list[0];
          exit(-10);
        }
        new_donor.attach(pa);

        
        SimpleAtom new_acceptor(*i, system.sharedPeriodicBox(), use_periodicity);
        
        Bond new_bond(new_donor, new_acceptor);
        bonds.push_back(new_bond);
      }
    }
  }
  
  return(bonds);
}




ostringstream& formatBond(ostringstream& oss, const uint i, const Bond& bond) {
  pAtom a = bond.first.rawAtom();
  pAtom b = bond.second.rawAtom();

  oss << boost::format("# %d : %d-%s-%s-%d-%s => %d-%s-%s-%d-%s")
    % i
    % a->id() % a->name() % a->resname() % a->resid() % a->segid()
    % b->id() % b->name() % b->resname() % b->resid() % b->segid();

  return(oss);
}



int main(int argc, char *argv[]) {
  string hdr = invocationHeader(argc, argv);


  opts::BasicOptions* bopts = new opts::BasicOptions();
  opts::BasicTrajectory* tropts = new opts::BasicTrajectory;
  ToolOptions* topts = new ToolOptions;
  
  opts::AggregateOptions options;
  options.add(bopts).add(tropts).add(topts);
  if (! options.parse(argc, argv))
    exit(-1);


  AtomicGroup model = tropts->model;
  pTraj traj = tropts->trajectory;

  if (use_periodicity && !traj->hasPeriodicBox()) {
    cerr << "Error- trajectory has no periodic box information\n";
    exit(-1);
  }


  // Get coords if required...
  if (!model.hasCoords()) {
    traj->readFrame();
    traj->updateGroupCoords(model);
    if (tropts->skip > 0)
      traj->readFrame(tropts->skip - 1);
    else
      traj->rewind();
  }


  vGroup mols = model.splitByMolecule();

  // First, build list of pairs we will search for...
  vGroup raw_donors = splitSelection(mols, donor_selection);
  vGroup raw_acceptors = splitSelection(mols, acceptor_selection);

  vBond bond_list;

  for (uint j=0; j<raw_donors.size(); ++j) {

    if (intra_bonds) {
      vBond bonds = findPotentialBonds(raw_donors[j], raw_acceptors[j], model);
      copy(bonds.begin(), bonds.end(), back_inserter(bond_list));
    }

    if (inter_bonds) {
      for (uint i=0; i<raw_acceptors.size(); ++i) {
        if (j == i)
          continue;
        vBond bonds = findPotentialBonds(raw_donors[j], raw_acceptors[i], model);
        copy(bonds.begin(), bonds.end(), back_inserter(bond_list));
      }
    }

  }


  // Generate the metadata for the output...
  ostringstream oss;
  oss << hdr << endl;
  for (uint i=0; i<bond_list.size(); ++i) {
    formatBond(oss, i+1, bond_list[i]);
    if (i < bond_list.size()-1)
      oss << endl;
  }


  // Use unsigned chars in matrix for space savings, but necessitates using a formatter
  // on output, otherwise we'll get the ASCII char, not the numerical value...

  loos::Math::Matrix<unsigned char> M(traj->nframes() - tropts->skip, bond_list.size()+1);

  uint j = 0;
  while (traj->readFrame()) {
    traj->updateGroupCoords(model);

    M(j, 0) = j + tropts->skip;
    for (uint i=0; i<bond_list.size(); ++i)
      M(j, i+1) = bond_list[i].first.hydrogenBond(bond_list[i].second);

    ++j;
  }

  writeAsciiMatrix(cout, M, oss.str(), false, FormatCharAsInteger());
}
