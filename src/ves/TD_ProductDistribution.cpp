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
#include "core/ActionSet.h"
#include "core/PlumedMain.h"
#include "tools/Grid.h"

namespace PLMD {
namespace ves {

//+PLUMEDOC VES_TARGETDIST TD_PRODUCT_DISTRIBUTION
/*
Target distribution given by a separable product
of one-dimensional distributions (static or dynamic).

Employ a target distribution that is a separable product
of one-dimensional distributions, defined as
\f[
p(\mathbf{s}) =
\prod_{k}^{d} p_{k}(s_{k})
\f]
where \f$d\f$ is the number of arguments used and \f$p_{k}(s_{k})\f$ is the
one-dimensional distribution corresponding to the \f$k\f$-th argument.

Note the difference between this target distribution and the one defined in
\ref TD_PRODUCT_COMBINATION. Here we have a separable distribution given as a
product of one-dimensional distribution \f$p_{k}(s_{k})\f$.

The distributions \f$p_{k}(s_{k})\f$ are given by using a separate numbered
DIST_ARG keyword for each argument. The keywords for each distribution
should be enclosed within curly brackets.

It is assumed that all the distributions given with the
DIST_ARG keywords are normalized. If that is not the case you need to
normalize the distributions by using the NORMALIZE keyword.
Here it does not matter if you normalize each distribution separately
or the overall product, it will give the same results.

The product distribution will be a dynamic target distribution if one or more
of the distributions used is a dynamic distribution. Otherwise it will be a
static distribution.

\par Examples

In the following example we employ a uniform distribution for
argument 1 and a Gaussian distribution for argument 2.
\plumedfile
TARGET_DISTRIBUTION={TD_PRODUCT_DISTRIBUTION
                     DIST_ARG1={TD_UNIFORM}
                     DIST_ARG2={TD_GAUSSIAN
                                CENTER=-2.0
                                SIGMA=0.5}}
\endplumedfile

*/
//+ENDPLUMEDOC

class TD_ProductDistribution: public TargetDistribution {
private:
  std::vector<TargetDistribution*> distribution_pntrs_;
  std::vector<Grid*> grid_pntrs_;
  unsigned int ndist_;
  void setupAdditionalGrids(const std::vector<Value*>&, const std::vector<std::string>&, const std::vector<std::string>&, const std::vector<unsigned int>&);
public:
  static void registerKeywords(Keywords&);
  explicit TD_ProductDistribution(const ActionOptions& ao);
  void updateGrid();
  double getValue(const std::vector<double>&) const;
  //
  void linkVesBias(VesBias*);
  void linkAction(Action*);
  void linkBiasGrid(Grid*);
  void linkBiasWithoutCutoffGrid(Grid*);
  void linkFesGrid(Grid*);
};


PLUMED_REGISTER_ACTION(TD_ProductDistribution,"TD_PRODUCT_DISTRIBUTION")


void TD_ProductDistribution::registerKeywords(Keywords& keys) {
  TargetDistribution::registerKeywords(keys);
  keys.add("compulsory","DISTRIBUTIONS","Labels of the one-dimensional target distributions for each argument to be used in the product distribution. Note that order of the labels is important.");
  keys.use("WELLTEMPERED_FACTOR");
  keys.use("SHIFT_TO_ZERO");
  keys.use("NORMALIZE");
}


TD_ProductDistribution::TD_ProductDistribution(const ActionOptions& ao):
  PLUMED_VES_TARGETDISTRIBUTION_INIT(ao),
  distribution_pntrs_(0),
  grid_pntrs_(0),
  ndist_(0)
{
  std::vector<std::string> targetdist_labels;
  parseVector("DISTRIBUTIONS",targetdist_labels);    
  for(unsigned int i=0; i<targetdist_labels.size(); i++) {
    TargetDistribution* dist_pntr_tmp = plumed.getActionSet().selectWithLabel<TargetDistribution*>(targetdist_labels[i]);  
    plumed_massert(dist_pntr_tmp!=NULL,"target distribution "+targetdist_labels[i]+" does not exist. NOTE: the target distribution should always be defined BEFORE the " + getName() + " action.");
    //
    if(dist_pntr_tmp->isDynamic()) {setDynamic();}
    if(dist_pntr_tmp->fesGridNeeded()) {setFesGridNeeded();}
    if(dist_pntr_tmp->biasGridNeeded()) {setBiasGridNeeded();}
    distribution_pntrs_.push_back(dist_pntr_tmp);
  }
  ndist_ = distribution_pntrs_.size();
  grid_pntrs_.assign(ndist_,NULL);
  setDimension(ndist_);

  checkRead();
}


double TD_ProductDistribution::getValue(const std::vector<double>& argument) const {
  plumed_merror("getValue not implemented for TD_ProductDistribution");
  return 0.0;
}


void TD_ProductDistribution::setupAdditionalGrids(const std::vector<Value*>& arguments, const std::vector<std::string>& min, const std::vector<std::string>& max, const std::vector<unsigned int>& nbins) {
  for(unsigned int i=0; i<ndist_; i++) {
    std::vector<Value*> arg1d(1);
    std::vector<std::string> min1d(1);
    std::vector<std::string> max1d(1);
    std::vector<unsigned int> nbins1d(1);
    arg1d[0]=arguments[i];
    min1d[0]=min[i];
    max1d[0]=max[i];
    nbins1d[0]=nbins[i];
    distribution_pntrs_[i]->setupGrids(arg1d,min1d,max1d,nbins1d);
    grid_pntrs_[i]=distribution_pntrs_[i]->getTargetDistGridPntr();
    if(distribution_pntrs_[i]->getDimension()!=1 || grid_pntrs_[i]->getDimension()!=1) {
      plumed_merror(getName() + ": all target distributions must be one dimensional");
    }
  }
}


void TD_ProductDistribution::updateGrid() {
  for(unsigned int i=0; i<ndist_; i++) {
    distribution_pntrs_[i]->updateTargetDist();
  }
  for(Grid::index_t l=0; l<targetDistGrid().getSize(); l++) {
    std::vector<unsigned int> indices = targetDistGrid().getIndices(l);
    double value = 1.0;
    for(unsigned int i=0; i<ndist_; i++) {
      value *= grid_pntrs_[i]->getValue(indices[i]);
    }
    targetDistGrid().setValue(l,value);
    logTargetDistGrid().setValue(l,-std::log(value));
  }
  logTargetDistGrid().setMinToZero();
}


void TD_ProductDistribution::linkVesBias(VesBias* vesbias_pntr_in) {
  TargetDistribution::linkVesBias(vesbias_pntr_in);
  for(unsigned int i=0; i<ndist_; i++) {
    distribution_pntrs_[i]->linkVesBias(vesbias_pntr_in);
  }
}


void TD_ProductDistribution::linkAction(Action* action_pntr_in) {
  TargetDistribution::linkAction(action_pntr_in);
  for(unsigned int i=0; i<ndist_; i++) {
    distribution_pntrs_[i]->linkAction(action_pntr_in);
  }
}


void TD_ProductDistribution::linkBiasGrid(Grid* bias_grid_pntr_in) {
  TargetDistribution::linkBiasGrid(bias_grid_pntr_in);
  for(unsigned int i=0; i<ndist_; i++) {
    distribution_pntrs_[i]->linkBiasGrid(bias_grid_pntr_in);
  }
}


void TD_ProductDistribution::linkBiasWithoutCutoffGrid(Grid* bias_withoutcutoff_grid_pntr_in) {
  TargetDistribution::linkBiasWithoutCutoffGrid(bias_withoutcutoff_grid_pntr_in);
  for(unsigned int i=0; i<ndist_; i++) {
    distribution_pntrs_[i]->linkBiasWithoutCutoffGrid(bias_withoutcutoff_grid_pntr_in);
  }
}


void TD_ProductDistribution::linkFesGrid(Grid* fes_grid_pntr_in) {
  TargetDistribution::linkFesGrid(fes_grid_pntr_in);
  for(unsigned int i=0; i<ndist_; i++) {
    distribution_pntrs_[i]->linkFesGrid(fes_grid_pntr_in);
  }
}


}
}
