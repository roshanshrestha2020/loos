/*
  Lipid lifetime
  
  (c) 2009 Tod D. Romo, Grossfield Lab
  Department of Biochemistry
  University of Rochster School of Medicine and Dentistry

  Given a lipid in contact with a protein at time t, what is the prob
  that the lipid will be in contact at time t+dt?

*/



/*

  This file is part of LOOS.

  LOOS (Lightweight Object-Oriented Structure library)
  Copyright (c) 2008-2009, Tod D. Romo
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
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <numeric>

using namespace std;
using namespace loos;
namespace po = boost::program_options;

typedef vector<AtomicGroup> vGroup;
typedef vector<vector <AtomicGroup> > list_vGroup; 

string protein_selection;
string model_name, traj_name;
string lipid_selection;
int skip;

void fullHelp(void) {
    cout << "Sorry... can't help you";

}

void parseOptions(int argc, char *argv[]) {
  try {

    po::options_description generic("Allowed options");
    generic.add_options()
      ("help", "Produce this help message")
      ("fullhelp", "Even more help")
      ("probe,p", po::value<string>(&protein_selection)->default_value("segname =~ 'Rhod'"), "Main selection")
      ("skip,s", po::value<int>(&skip)->default_value(0), "Frames to skip");

    po::options_description hidden("Hidden options");
    hidden.add_options()
      ("model", po::value<string>(&model_name), "Model filename")
      ("traj", po::value<string>(&traj_name), "Trajectory filename")
      ("target", po::value<string>(&lipid_selection), "Target selection");
    
    po::options_description command_line;
    command_line.add(generic).add(hidden);
    
    po::positional_options_description p;
    p.add("model", 1);
    p.add("traj", 1);
    p.add("target", -1);

    
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(command_line).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help") || vm.count("fullhelp") || !(vm.count("model") && vm.count("traj") && !lipid_selection.empty())) {
      cerr << "Usage- " << argv[0] << " [options] model-name trajectory-name lipid-selection\n";
      cerr << generic;
      if (vm.count("fullhelp"))
        fullHelp();
      exit(-1);
    }

  }
  catch(exception& e) {
    cerr << "Error - " << e.what() << endl;
    exit(-1);
  }
}


int main(int argc, char *argv[]) {
  string hdr = invocationHeader(argc, argv);
  parseOptions(argc, argv);

  AtomicGroup model = createSystem(model_name);
  pTraj traj = createTrajectory(traj_name, model);

  AtomicGroup protein = selectAtoms(model, protein_selection);
  //vGroup probe_residues = probe.splitByResidue();

  cout << "# " << hdr << endl;
  // selections for targets
  AtomicGroup lipid = selectAtoms(model, lipid_selection);
  vGroup lipids = lipid.splitByMolecule();


  vector < vector <double> > contacts;
  // initialize vector of timeseries to store 
  for (unsigned int i = 0; i < lipids.size(); i++)
      {

        vector <double> tmp (traj->nframes(), 0.0);
        contacts.push_back(tmp);

      }

int frame_count = 0;
  while (traj->readFrame()) 
      {
        traj->updateGroupCoords(model);

        // Loop over lipids
        for (unsigned int j = 0; j < contacts.size(); j++)
            {

            // Find distance less than cutoff
            double cutoff = 6.0;
            
            // Loop over atoms in lipid
            for (unsigned int k = 0; k < lipids[j].size(); k++)
                {

                    // Loop over atoms in protein
                    for (unsigned int l = 0; l < protein.size(); l++)
                        {

                        // Compare to cutoff... If matches, set contact to 1 and set k and l
                        // so that the loops end...
                        if (((lipids[j][k]->coords()).distance(protein[l]->coords())) < cutoff)
                            {

                            k = lipids[j].size();
                            l = protein.size();
                            contacts[j][frame_count] = 1.0;

                            }

                        }

                }


            }

        frame_count++;

      }

/* Probability Calculations
 */

// loop over dt
//
cout << "0\t1.00" << endl;
for (unsigned int t = 1; t < 1000; t++)
    {

    double bound_tmp = 0.0;
    double total_tmp = 0.0;
    // loop over lipids
    for (unsigned int i = 0; i < contacts.size(); i++)
        {

        // loop through values for that lipid's contacts
        for (unsigned int j = 0; j < (contacts[i].size() - t); j++)
            {
            
            // if current value is 1
            if (contacts[i][j] == 1)
                {
                    if ((contacts[i][j+t]) == 1)
                        {

                        bound_tmp++;

                        }

                        total_tmp++;
                }

            }

        }

    double prob_tmp = bound_tmp/total_tmp;

    cout << t << "\t" << prob_tmp << endl;
    }

  /* TIME AUTOCORRELATION CODE...

// loop over lipids and calculate correlation info and store in correl matrix
vector <TimeSeries <double> > correl;

for (unsigned int i = 0; i < contacts.size(); i++)
    {
    
    TimeSeries<double> t_set;
    t_set = TimeSeries<double>(contacts[i]);
    if (t_set.average() > 0.01)
        {
        correl.push_back(t_set.correl(1000));
        }
    }

for (unsigned int k = 0; k < correl[0].size(); k++)
    {

    vector<double> tmp_time;

    for (unsigned int j = 0; j < correl.size(); j++)
        {
        
        tmp_time.push_back(correl[j][k]);

        }

    TimeSeries<double> t = TimeSeries<double>(tmp_time);
    cout << k << "\t" << t.average() << "\t" << t.stdev() << "\t" << t.sterr() << endl;
    }

    */

// end main
} 
