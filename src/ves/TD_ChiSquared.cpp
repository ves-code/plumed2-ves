/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2016-2017 The ves-code team
   (see the PEOPLE-VES file at the root of this folder for a list of names)

   See http://www.ves-code.org for more information.

   This file is part of ves-code, version 1.

   ves-code is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   ves-code is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with ves-code.  If not, see <http://www.gnu.org/licenses/>.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

#include "TargetDistribution.h"

#include "core/ActionRegister.h"


namespace PLMD {
namespace ves {

//+PLUMEDOC VES_TARGETDIST TD_CHISQUARED
/*
Chi-squared distribution (static).

Employ a target distribution given by a
[chi-squared distribution](https://en.wikipedia.org/wiki/Chi-squared_distribution)
that is defined as
\f[
p(s) =
\frac
{1}
{\sigma \, 2^{\frac{k}{2}}  \,  \Gamma\left(\frac{k}{2}\right) }
\, \left(\frac{s-a}{\sigma}\right)^{\frac{k}{2}-1} \, \exp\left(- \frac{1}{2}
\left(\frac{s-a}{\sigma}\right) \right),
\f]
where \f$a\f$ is the minimum of the distribution that is defined on the interval \f$[a,\infty)\f$,
the parameter \f$k\f$ (known as the "degrees of freedom") determines how far
the peak of the distribution is from the minimum,
and the parameter \f$\sigma\f$ determines the broadness of the distribution.

The minimum \f$a\f$ is given using the MINIMUM keyword, the parameter \f$k\f$ is given
using the KAPPA keyword, and the parameter \f$\sigma\f$ is given using the SIGMA keyword.

This target distribution action is only defined for one dimension, for multiple dimensions
it should be used in combination with \ref TD_PRODUCT_DISTRIBUTION action.

\par Examples

*/
//+ENDPLUMEDOC

class TD_ChiSquared: public TargetDistribution {
  std::vector<double> minima_;
  std::vector<double> sigma_;
  std::vector<double> kappa_;
  std::vector<double> normalization_;
public:
  static void registerKeywords(Keywords&);
  explicit TD_ChiSquared(const ActionOptions& ao);
  double getValue(const std::vector<double>&) const;
};


PLUMED_REGISTER_ACTION(TD_ChiSquared,"TD_CHISQUARED")


void TD_ChiSquared::registerKeywords(Keywords& keys) {
  TargetDistribution::registerKeywords(keys);
  keys.add("compulsory","MINIMUM","The minimum of the chi-squared distribution.");
  keys.add("compulsory","SIGMA","The sigma parameter of the chi-squared distribution.");
  keys.add("compulsory","KAPPA","The kappa parameter of the chi-squared distribution.");
  keys.use("WELLTEMPERED_FACTOR");
  keys.use("SHIFT_TO_ZERO");
  keys.use("NORMALIZE");
}


TD_ChiSquared::TD_ChiSquared(const ActionOptions& ao):
  PLUMED_VES_TARGETDISTRIBUTION_INIT(ao),
  minima_(0),
  sigma_(0),
  kappa_(0),
  normalization_(0)
{
  parseVector("MINIMUM",minima_);
  parseVector("SIGMA",sigma_);
  for(unsigned int k=0; k<sigma_.size(); k++) {
    if(sigma_[k] < 0.0) {plumed_merror(getName()+": the value given in SIGMA should be postive.");}
  }

  std::vector<unsigned int> kappa_int(0);
  parseVector("KAPPA",kappa_int);
  if(kappa_int.size()==0) {plumed_merror(getName()+": some problem with KAPPA keyword, should given as postive integer larger than 1");}
  kappa_.resize(kappa_int.size());
  for(unsigned int k=0; k<kappa_int.size(); k++) {
    if(kappa_int[k] < 2) {plumed_merror(getName()+": KAPPA should be an integer 2 or higher");}
    kappa_[k] = static_cast<double>(kappa_int[k]);
  }

  setDimension(minima_.size());
  if(getDimension()>1) {plumed_merror(getName()+": only defined for one dimension");}
  if(sigma_.size()!=getDimension()) {plumed_merror(getName()+": the SIGMA keyword does not match the given dimension in MINIMUM");}
  if(kappa_.size()!=getDimension()) {plumed_merror(getName()+": the KAPPA keyword does not match the given dimension in MINIMUM");}

  normalization_.resize(getDimension());
  for(unsigned int k=0; k<getDimension(); k++) {
    normalization_[k] = 1.0/(pow(2.0,0.5*kappa_[k])*tgamma(0.5*kappa_[k])*sigma_[k]);
  }
  checkRead();
}


double TD_ChiSquared::getValue(const std::vector<double>& argument) const {
  double value = 1.0;
  for(unsigned int k=0; k<argument.size(); k++) {
    double arg=(argument[k]-minima_[k])/sigma_[k];
    if(arg<0.0) {plumed_merror(getName()+": the chi-squared istribution is not defined for values less that ones given in MINIMUM");}
    value *= normalization_[k] * pow(arg,0.5*kappa_[k]-1.0) * exp(-0.5*arg);
  }
  return value;
}


}
}