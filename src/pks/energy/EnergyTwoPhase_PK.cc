/*
  This is the Energy component of the Amanzi code.
   
  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Authors: Ethan Coon
           Konstantin Lipnikov (lipnikov@lanl.gov)

  Process kernel for thermal Richards' flow.
*/

// Amanzi
#include "eos_evaluator.hh"
#include "OperatorDiffusionFactory.hh"

// Energy
#include "EnergyTwoPhase_PK.hh"
#include "EnthalpyEvaluator.hh"
#include "IEMEvaluator.hh"
#include "TCMEvaluator_TwoPhase.hh"
#include "TwoPhaseEnergyEvaluator.hh"

namespace Amanzi {
namespace Energy {

/* ******************************************************************
* Default constructor for Thermal Richrads PK.
****************************************************************** */
EnergyTwoPhase_PK::EnergyTwoPhase_PK(
                   Teuchos::ParameterList& pk_tree,
                   const Teuchos::RCP<Teuchos::ParameterList>& glist,
                   const Teuchos::RCP<State>& S,
                   const Teuchos::RCP<TreeVector>& soln) :
    Energy_PK(glist, S),
    soln_(soln)
    
{
  Teuchos::RCP<Teuchos::ParameterList> pk_list = Teuchos::sublist(glist, "PKs", true);
  ep_list_ = Teuchos::sublist(pk_list, "Energy", true);

  // We also need miscaleneous sublists
  preconditioner_list_ = Teuchos::sublist(glist, "Preconditioners", true);
  ti_list_ = Teuchos::sublist(ep_list_, "time integrator");
};


/* ******************************************************************
* Create the physical evaluators for energy, enthalpy, thermal
* conductivity, and any sources.
****************************************************************** */
void EnergyTwoPhase_PK::Setup()
{
  // basic class setup
  Energy_PK::Setup();

  // Get data and evaluators needed by the PK
  // -- energy, the conserved quantity
  S_->RequireField(energy_key_)->SetMesh(mesh_)->SetGhosted()
    ->AddComponent("cell", AmanziMesh::CELL, 1);

  Teuchos::ParameterList ee_list = glist_->sublist("PKs").sublist("Energy").sublist("energy evaluator");
  ee_list.set("energy key", energy_key_);
  Teuchos::RCP<TwoPhaseEnergyEvaluator> ee = Teuchos::rcp(new TwoPhaseEnergyEvaluator(ee_list));
  S_->SetFieldEvaluator(energy_key_, ee);

  // -- advection of enthalpy
  S_->RequireField(enthalpy_key_)->SetMesh(mesh_)
    ->SetGhosted()->AddComponent("cell", AmanziMesh::CELL, 1);

  Teuchos::ParameterList enth_plist = glist_->sublist("PKs").sublist("Energy").sublist("enthalpy evaluator");
  enth_plist.set("enthalpy key", enthalpy_key_);
  Teuchos::RCP<EnthalpyEvaluator> enth = Teuchos::rcp(new EnthalpyEvaluator(enth_plist));
  S_->SetFieldEvaluator(enthalpy_key_, enth);

  // -- thermal conductivity
  S_->RequireField(conductivity_key_)->SetMesh(mesh_)
    ->SetGhosted()->AddComponent("cell", AmanziMesh::CELL, 1);
  Teuchos::ParameterList tcm_plist = glist_->sublist("PKs")
                                            .sublist("Energy")
                                            .sublist("thermal conductivity evaluator");
  Teuchos::RCP<TCMEvaluator_TwoPhase> tcm = Teuchos::rcp(new TCMEvaluator_TwoPhase(tcm_plist));
  S_->SetFieldEvaluator(conductivity_key_, tcm);
}


/* ******************************************************************
* Initialize the needed models to plug in enthalpy.
****************************************************************** */
void EnergyTwoPhase_PK::Initialize()
{
  // create verbosity object
  Teuchos::ParameterList vlist;
  vlist.sublist("VerboseObject") = ep_list_->sublist("VerboseObject");
  vo_ = new VerboseObject("EnergyPK::2Phase", vlist); 

  // Create a scalar tensor so far
  K.resize(ncells_owned);
  for (int c = 0; c < ncells_owned; c++) {
    K[c].Init(dim, 1);
    K[c](0, 0) = 1.0;
  }

  // Call the base class initialize.
  Energy_PK::Initialize();

  // Create pointers to the primary flow field pressure.
  solution = S_->GetFieldData("temperature", passwd_);
  soln_->SetData(solution); 

  // Create local evaluators. Initialize local fields.
  InitializeFields_();

  // Create specific evaluators (not used yet)
  /*
  Teuchos::RCP<FieldEvaluator> eos_fe = S_->GetFieldEvaluator("molar_density_liquid");
  eos_fe->HasFieldChanged(S_.ptr(), "molar_density_liquid");
  Teuchos::RCP<Relations::EOSEvaluator> eos_eval = Teuchos::rcp_dynamic_cast<Relations::EOSEvaluator>(eos_fe);
  ASSERT(eos_eval != Teuchos::null);
  eos_liquid_ = eos_eval->get_EOS();

  Teuchos::RCP<FieldEvaluator> iem_fe = S_->GetFieldEvaluator("internal_energy_liquid");
  Teuchos::RCP<IEMEvaluator> iem_eval = Teuchos::rcp_dynamic_cast<IEMEvaluator>(iem_fe);
  ASSERT(iem_eval != Teuchos::null);
  iem_liquid_ = iem_eval->get_IEM();
  */

  // initialize independent operators: diffusion and advection 
  Teuchos::ParameterList tmp_list = ep_list_->sublist("operators").sublist("diffusion operator");
  Teuchos::ParameterList oplist_matrix = tmp_list.sublist("matrix");
  Teuchos::ParameterList oplist_pc = tmp_list.sublist("preconditioner");

  AmanziGeometry::Point g(dim);

  Operators::OperatorDiffusionFactory opfactory;
  op_matrix_diff_ = opfactory.Create(mesh_, op_bc_, oplist_matrix, g, 0);
  op_matrix_diff_->SetBCs(op_bc_);
  op_matrix_ = op_matrix_diff_->global_operator();
  op_matrix_->Init();
  Teuchos::RCP<std::vector<WhetStone::Tensor> > Kptr = Teuchos::rcpFromRef(K);
  op_matrix_diff_->Setup(Kptr, Teuchos::null, Teuchos::null);

  Teuchos::ParameterList oplist_adv = ep_list_->sublist("operators").sublist("advection operator");
  op_matrix_advection_ = Teuchos::rcp(new Operators::OperatorAdvection(oplist_adv, mesh_));

  const CompositeVector& flux = *S_->GetFieldData("darcy_flux");
  op_matrix_advection_->Setup(flux);
  op_advection_ = op_matrix_advection_->global_operator();

  // initialize copuled operators: diffusion + advection + accumulation
  op_preconditioner_diff_ = opfactory.Create(mesh_, op_bc_, oplist_pc, g, 0);
  op_preconditioner_diff_->SetBCs(op_bc_);
  op_preconditioner_ = op_preconditioner_diff_->global_operator();
  op_preconditioner_->Init();
  op_preconditioner_diff_->Setup(Kptr, Teuchos::null, Teuchos::null);

  op_acc_ = Teuchos::rcp(new Operators::OperatorAccumulation(AmanziMesh::CELL, op_preconditioner_));
  op_preconditioner_advection_ = Teuchos::rcp(new Operators::OperatorAdvection(oplist_adv, op_preconditioner_));

  op_preconditioner_->SymbolicAssembleMatrix();

  // preconditioner and optional linear solver
  ASSERT(ti_list_->isParameter("preconditioner"));
  preconditioner_name_ = ti_list_->get<std::string>("preconditioner");

  // initialize time integrator
  std::string ti_method_name = ti_list_->get<std::string>("time integration method", "none");
  if (ti_method_name == "BDF1") {
    Teuchos::ParameterList& bdf1_list = ti_list_->sublist("BDF1");

    if (! bdf1_list.isSublist("VerboseObject"))
        bdf1_list.sublist("VerboseObject") = ep_list_->sublist("VerboseObject");

    bdf1_dae_ = Teuchos::rcp(new BDF1_TI<TreeVector, TreeVectorSpace>(*this, bdf1_list, soln_));
  }

  // output of initialization header
  if (vo_->getVerbLevel() >= Teuchos::VERB_MEDIUM) {
    Teuchos::OSTab tab = vo_->getOSTab();
    *vo_->os() << std::endl << vo_->color("green")
               << "Initalization of TI period is complete." << vo_->reset() << std::endl;
  }
}


/* ****************************************************************
* This completes initialization of missed fields in the state.
**************************************************************** */
void EnergyTwoPhase_PK::InitializeFields_()
{
  Teuchos::OSTab tab = vo_->getOSTab();

  if (S_->HasField(prev_energy_key_)) {
    if (!S_->GetField(prev_energy_key_, passwd_)->initialized()) {
      temperature_eval_->SetFieldAsChanged(S_.ptr());
      S_->GetFieldEvaluator(energy_key_)->HasFieldChanged(S_.ptr(), passwd_);

      const CompositeVector& e1 = *S_->GetFieldData(energy_key_);
      CompositeVector& e0 = *S_->GetFieldData(prev_energy_key_, passwd_);
      e0 = e1;

      S_->GetField(prev_energy_key_, passwd_)->set_initialized();

      if (vo_->getVerbLevel() >= Teuchos::VERB_MEDIUM)
          *vo_->os() << "initilized prev_energy to previous energy" << std::endl;  
    }
  }
}


/* ******************************************************************* 
* Performs one time step of size dt_ either for steady-state or 
* transient sumulation.
******************************************************************* */
bool EnergyTwoPhase_PK::AdvanceStep(double t_old, double t_new)
{
  dt_ = t_new - t_old;

  // save a copy of pressure
  CompositeVector temperature_copy(*S_->GetFieldData("temperature", passwd_));

  // swap conserved field (i.e., energy) and save
  S_->GetFieldEvaluator(energy_key_)->HasFieldChanged(S_.ptr(), passwd_);
  const CompositeVector& e = *S_->GetFieldData(energy_key_);
  CompositeVector& e_prev = *S_->GetFieldData(prev_energy_key_, passwd_);

  CompositeVector e_prev_copy(e_prev);
  e_prev = e;

  // initialization
  if (num_itrs_ == 0) {
    Teuchos::RCP<TreeVector> udot = Teuchos::rcp(new TreeVector(*soln_));
    udot->PutScalar(0.0);
    bdf1_dae_->SetInitialState(t_old, soln_, udot);

    UpdatePreconditioner(t_old, soln_, dt_);
    num_itrs_++;
  }

  // trying to make a step
  bool failed(false);
  failed = bdf1_dae_->TimeStep(dt_, dt_next_, soln_);
  if (failed) {
    dt_ = dt_next_;

    // restore the original primary solution, temperature
    *S_->GetFieldData("temperature", passwd_) = temperature_copy;
    temperature_eval_->SetFieldAsChanged(S_.ptr());

    // restore the original fields
    *S_->GetFieldData(prev_energy_key_, passwd_) = e_prev_copy;

    Teuchos::OSTab tab = vo_->getOSTab();
    *vo_->os() << "Step failed. Restored temperature, prev_energy." << std::endl;

    return failed;
  }

  // commit solution (should we do it here ?)
  bdf1_dae_->CommitSolution(dt_, soln_);
  temperature_eval_->SetFieldAsChanged(S_.ptr());

  num_itrs_++;
  dt_ = dt_next_;
  
  return failed;
}


/* ******************************************************************
* TBW 
****************************************************************** */
void EnergyTwoPhase_PK::CommitStep(double t_old, double t_new)
{
  dt_ = dt_next_;
}

}  // namespace Energy
}  // namespace Amanzi
