/*
  Operators 

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Konstantin Lipnikov (lipnikov@lanl.gov)
*/

#ifndef AMANZI_UPWIND_STANDARD_HH_
#define AMANZI_UPWIND_STANDARD_HH_

#include <string>
#include <vector>

#include "Epetra_IntVector.h"
#include "Teuchos_RCP.hpp"
#include "Teuchos_ParameterList.hpp"

#include "CompositeVector.hh"
#include "Mesh.hh"
#include "mfd3d_diffusion.hh"
#include "VerboseObject.hh"

#include "Upwind.hh"

namespace Amanzi {
namespace Operators {

template<class Model>
class UpwindStandard : public Upwind<Model> {
 public:
  UpwindStandard(Teuchos::RCP<const AmanziMesh::Mesh> mesh,
                 Teuchos::RCP<const Model> model)
      : Upwind<Model>(mesh, model) {};
  ~UpwindStandard() {};

  // main methods
  void Init(Teuchos::ParameterList& plist);

  void Compute(const CompositeVector& flux, const CompositeVector& solution,
               const std::vector<int>& bc_model, const std::vector<double>& bc_value,
               const CompositeVector& field, CompositeVector& field_upwind,
               double (Model::*Value)(int, double) const);

 private:
  using Upwind<Model>::vo_;
  using Upwind<Model>::mesh_;
  using Upwind<Model>::model_;

 private:
  int method_, order_;
  double tolerance_;
};


/* ******************************************************************
* Public init method. It is not yet used.
****************************************************************** */
template<class Model>
void UpwindStandard<Model>::Init(Teuchos::ParameterList& plist)
{
  vo_ = Teuchos::rcp(new VerboseObject("UpwindStandard", plist));

  method_ = Operators::OPERATOR_UPWIND_FLUX;
  tolerance_ = plist.get<double>("tolerance", OPERATOR_UPWIND_RELATIVE_TOLERANCE);

  order_ = plist.get<int>("order", 1);
}


/* ******************************************************************
* Flux-based upwind.
****************************************************************** */
template<class Model>
void UpwindStandard<Model>::Compute(
    const CompositeVector& flux, const CompositeVector& solution,
    const std::vector<int>& bc_model, const std::vector<double>& bc_value,
    const CompositeVector& field, CompositeVector& field_upwind,
    double (Model::*Value)(int, double) const)
{
  ASSERT(field.HasComponent("cell"));
  ASSERT(field_upwind.HasComponent("face"));

  Teuchos::OSTab tab = vo_->getOSTab();

  field.ScatterMasterToGhosted("cell");
  flux.ScatterMasterToGhosted("face");

  const Epetra_MultiVector& flx_face = *flux.ViewComponent("face", true);
  const Epetra_MultiVector& fld_cell = *field.ViewComponent("cell", true);
  // const Epetra_MultiVector& sol_face = *solution.ViewComponent("face", true);

  Epetra_MultiVector& upw_face = *field_upwind.ViewComponent("face", true);
  upw_face.PutScalar(0.0);

  double flxmin, flxmax;
  flx_face.MinValue(&flxmin);
  flx_face.MaxValue(&flxmax);
  double tol = tolerance_ * std::max(fabs(flxmin), fabs(flxmax));

  int ncells_wghost = mesh_->num_entities(AmanziMesh::CELL, AmanziMesh::USED);
  std::vector<int> dirs;
  AmanziMesh::Entity_ID_List faces;
  WhetStone::MFD3D_Diffusion mfd(mesh_);

  for (int c = 0; c < ncells_wghost; c++) {
    mesh_->cell_get_faces_and_dirs(c, &faces, &dirs);
    int nfaces = faces.size();
    double kc(fld_cell[0][c]);

    for (int n = 0; n < nfaces; n++) {
      int f = faces[n];
      bool flag = (flx_face[0][f] * dirs[n] <= -tol);  // upwind flag
      
      // Internal faces. We average field on almost vertical faces. 
      if (bc_model[f] == OPERATOR_BC_NONE && fabs(flx_face[0][f]) <= tol) { 
        double tmp(0.5);
        int c2 = mfd.cell_get_face_adj_cell(c, f);
        if (c2 >= 0) { 
          double v1 = mesh_->cell_volume(c);
          double v2 = mesh_->cell_volume(c2);
          tmp = v2 / (v1 + v2);
        }
        upw_face[0][f] += kc * tmp; 
      // Boundary faces. We upwind only on inflow dirichlet faces.
      } else if (bc_model[f] == OPERATOR_BC_DIRICHLET && flag) {
        upw_face[0][f] = ((*model_).*Value)(c, bc_value[f]);
      } else if (bc_model[f] == OPERATOR_BC_NEUMANN && flag) {
        // upw_face[0][f] = ((*model_).*Value)(c, sol_face[0][f]);
        upw_face[0][f] = kc;
      } else if (bc_model[f] == OPERATOR_BC_MIXED && flag) {
        upw_face[0][f] = kc;
      // Internal and boundary faces. 
      } else if (!flag) {
        upw_face[0][f] = kc;
      }
    }
  }
}

}  // namespace Operators
}  // namespace Amanzi

#endif

