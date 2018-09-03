/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2011-2018 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   See http://www.plumed.org for more information.

   This file is part of plumed, version 2.

   plumed is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   plumed is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with plumed.  If not, see <http://www.gnu.org/licenses/>.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#include "SetupReferenceBase.h"
#include "core/PlumedMain.h"

namespace PLMD {
namespace setup {

void SetupReferenceBase::registerKeywords( Keywords& keys ) {
  Action::registerKeywords( keys ); ActionAtomistic::registerKeywords( keys );
}

SetupReferenceBase::SetupReferenceBase(const ActionOptions&ao):
Action(ao),
ActionSetup(ao),
ActionAtomistic(ao),
ActionWithValue(ao),
hasatoms(false)
{
}

SetupReferenceBase::~SetupReferenceBase() {
  if( hasatoms ) { atoms.removeVirtualAtom( this ); atoms.removeGroup( getLabel() ); }
}

void SetupReferenceBase::getNatomsAndNargs( unsigned& natoms, unsigned& nargs ) const {
  // Get the number of atoms
  natoms=0;
  for(unsigned i=0;i<myindices.size();++i) { 
      if( myindices[i].serial()>natoms ) natoms = myindices[i].serial();
  }
  // Get the number of arguments
  nargs=0; if( getNumberOfComponents()>0 ) nargs = getPntrToOutput(0)->getNumberOfValues( getLabel() );
}

void SetupReferenceBase::transferDataToPlumed( const unsigned& npos, std::vector<double>& masses, std::vector<double>& charges, 
                                               std::vector<Vector>& positions, const std::string& argname, PlumedMain& plmd ) const {
  for(unsigned i=0;i<myindices.size();++i) {
      masses[npos + myindices[i].index()] = atoms.getVatomMass(mygroup[i]);
      charges[npos + myindices[i].index()] = atoms.getVatomCharge(mygroup[i]);
      positions[npos + myindices[i].index()] = atoms.getVatomPosition(mygroup[i]);
  }
  if( getNumberOfComponents()>0 ) {
      unsigned nvals = getPntrToOutput(0)->getSize(); 
      std::vector<double> valdata( nvals );
      for(unsigned i=0;i<nvals;++i) valdata[i] = getPntrToOutput(0)->get(i);
      plmd.cmd("setValue " + argname, &valdata[0] );
  }
}

}
}
