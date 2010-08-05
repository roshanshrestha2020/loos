/*
  enm-lib

  (c) 2010 Tod D. Romo, Grossfield Lab
      Department of Biochemistry
      University of Rochster School of Medicine and Dentistry


  Common code for the ENM suite

*/


/*
  This file is part of LOOS.

  LOOS (Lightweight Object-Oriented Structure library)
  Copyright (c) 2009 Tod D. Romo
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


#include "enm-lib.hpp"


using namespace std;
using namespace loos;

// Map masses from one group onto another...  Minimal error checking...
void copyMasses(AtomicGroup& target, const AtomicGroup& source) {
  if (target.size() != source.size()) {
    cerr << "ERROR- groups have different sizes in copyMasses... (maybe your PSF doesn't match the model?)\n";
    exit(-1);
  }

  for (int i=0; i<target.size(); ++i) {
    if (source[i]->name() != target[i]->name()) {
      cerr << "ERROR- atom mismatch at position " << i << endl;
      exit(-1);
    }
    target[i]->mass(source[i]->mass());
  }
}


// Copy the masses from a PSF onto a group
void massFromPSF(AtomicGroup& grp, const string& name) {
  AtomicGroup psf = createSystem(name);
  copyMasses(grp, psf);
}


// The masses are stored in the occupancy field of a PDB...
void massFromOccupancy(AtomicGroup& grp) {
  for (AtomicGroup::iterator i = grp.begin(); i != grp.end(); ++i)
      (*i)->mass((*i)->occupancy());
}


// Build the 3n x 3n diagonal mass matrix for a group
DoubleMatrix getMasses(const AtomicGroup& grp) {
  uint n = grp.size();

  DoubleMatrix M(3*n,3*n);
  for (uint i=0, k=0; i<n; ++i, k += 3) {
    M(k,k) = grp[i]->mass();
    M(k+1,k+1) = grp[i]->mass();
    M(k+2,k+2) = grp[i]->mass();
  }

  return(M);
}





DoubleMatrix submatrix(const DoubleMatrix& M, const Range& rows, const Range& cols) {
  uint m = rows.second - rows.first;
  uint n = cols.second - cols.first;

  DoubleMatrix A(m,n);
  for (uint i=0; i < n; ++i)
    for (uint j=0; j < m; ++j)
      A(j,i) = M(j+rows.first, i+cols.first);

  return(A);
}





// Matrix holds column vectors.  Each vector is normalized...
void normalizeColumns(DoubleMatrix& A) {
  for (uint i=0; i<A.cols(); ++i) {
    double sum = 0.0;
    for (uint j=0; j<A.rows(); ++j)
      sum += A(j, i) * A(j, i);

    if (sum <= 0) {
      for (uint j=0; j<A.rows(); ++j)
        A(j, i) = 0.0;
    } else {
      sum = sqrt(sum);
      for (uint j=0; j<A.rows(); ++j)
        A(j, i) /= sum;
    }
  }
}


