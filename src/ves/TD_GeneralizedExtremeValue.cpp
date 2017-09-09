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

//+PLUMEDOC VES_TARGETDIST TD_GENERALIZED_EXTREME_VALUE
/*
Generalized extreme value distribution (static).

Employ a target distribution given by a
[generalized extreme value distribution](https://en.wikipedia.org/wiki/Generalized_extreme_value_distribution)
that is defined as
\f[
p(s) =
\frac{1}{\sigma} \, t(s)^{\xi+1} \, e^{-t(s)},
\f]
where
\f[
t(s) =
\begin{cases}
\left( 1 + \xi \left( \frac{s-\mu}{\sigma} \right) \right)^{-1/\xi} & \mathrm{if\ }\xi \neq 0 \\
\exp\left(- \frac{s-\mu}{\sigma} \right) & \mathrm{if\ } \xi = 0
\end{cases},
\f]
and \f$\mu\f$ is the location parameter, \f$\sigma\f$ is the scale parameter,
and \f$\xi\f$ is the shape parameter.

The location parameter \f$\mu\f$ is given using the LOCATION keyword, the scale parameter \f$\sigma\f$
using the SCALE keyword, and the shape parameter \f$\xi\f$ using the SHAPE
keyword.

This target distribution action is only defined for one dimension, for multiple dimensions
it should be used in combination with \ref TD_PRODUCT_DISTRIBUTION action.

\par Examples

*/
//+ENDPLUMEDOC

class TD_GeneralizedExtremeValue: public TargetDistribution {
  std::vector<double> center_;
  std::vector<double> scale_;
  std::vector<double> shape_;
  std::vector<double> normalization_;
  double GEVdiagonal(const std::vector<double>&, const std::vector<double>&, const std::vector<double>&, const std::vector<double>&, const std::vector<double>&) const;
public:
  static void registerKeywords(Keywords&);
  explicit TD_GeneralizedExtremeValue(const ActionOptions& ao);
  double getValue(const std::vector<double>&) const;
};


PLUMED_REGISTER_ACTION(TD_GeneralizedExtremeValue,"TD_GENERALIZED_EXTREME_VALUE")


void TD_GeneralizedExtremeValue::registerKeywords(Keywords& keys) {
  TargetDistribution::registerKeywords(keys);
  keys.add("compulsory","LOCATION","The location parameter of the generalized extreme value distribution.");
  keys.add("compulsory","SCALE","The scale parameter for the generalized extreme value distribution.");
  keys.add("compulsory","SHAPE","The shape parameter for the generalized extreme value distribution.");
  keys.use("WELLTEMPERED_FACTOR");
  keys.use("SHIFT_TO_ZERO");
  keys.use("NORMALIZE");
}


TD_GeneralizedExtremeValue::TD_GeneralizedExtremeValue(const ActionOptions& ao):
  PLUMED_VES_TARGETDISTRIBUTION_INIT(ao),
  center_(0),
  scale_(0),
  shape_(0),
  normalization_(0)
{
  parseVector("LOCATION",center_);
  parseVector("SCALE",scale_);
  parseVector("SHAPE",shape_);

  setDimension(center_.size());
  if(getDimension()>1) {plumed_merror(getName()+": only defined for one dimension");}
  if(scale_.size()!=getDimension()) {plumed_merror(getName()+": the SCALE keyword does not match the given dimension in MINIMA");}
  if(shape_.size()!=getDimension()) {plumed_merror(getName()+": the SHAPE keyword does not match the given dimension in MINIMA");}

  normalization_.resize(getDimension());
  for(unsigned int k=0; k<getDimension(); k++) {
    if(scale_[k]<0.0) {plumed_merror(getName()+": the value given for the scale parameter in SCALE should be larger than 0.0");}
    normalization_[k] = 1.0/scale_[k];
  }
  checkRead();
}


double TD_GeneralizedExtremeValue::getValue(const std::vector<double>& argument) const {
  return GEVdiagonal(argument,center_,scale_,shape_,normalization_);
}


double TD_GeneralizedExtremeValue::GEVdiagonal(const std::vector<double>& argument, const std::vector<double>& center, const std::vector<double>& scale, const std::vector<double>& shape, const std::vector<double>& normalization) const {
  double value = 1.0;
  for(unsigned int k=0; k<argument.size(); k++) {
    double arg=(argument[k]-center[k])/scale[k];
    double tx;
    if(shape_[k]!=0.0) {
      if( shape_[k]>0 && argument[k] <= (center[k]-scale[k]/shape[k]) ) {return 0.0;}
      if( shape_[k]<0 && argument[k] > (center[k]-scale[k]/shape[k]) ) {return 0.0;}
      tx = pow( (1.0+arg*shape[k]), -1.0/shape[k] );
    }
    else {
      tx = exp(-arg);
    }
    value *= normalization[k] * pow(tx,shape[k]+1.0) * exp(-tx);
  }
  return value;
}



}
}