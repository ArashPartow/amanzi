# plots tracer concentration along x at last timestep 
# benchmark: compares to pflotran simulation results
# author: S.Molins - Sept. 2013

import os
import sys
import h5py
import numpy as np
import matplotlib
# matplotlib.use('Agg')
from matplotlib import pyplot as plt

try:
    sys.path.append('../../../../tools/amanzi_xml')
except:
    pass
    
try:
    sys.path.append('../../../../tools/testing')
except:
    pass

import run_amanzi_standard
from compare_field_results import GetXY_AmanziU_1D
from compare_field_results import GetXY_AmanziS_1D
from compare_field_results import GetXY_PFloTran_1D
from compare_field_results import GetXY_CrunchFlow_1D


if __name__ == "__main__":

    import os,sys
    import numpy as np

    try:
        sys.path.append('../../../../MY_TPL_BUILD/ccse/ccse-1.3.4-source/Tools/Py_util')
    except:
        pass
    
    # root name for problem
    root = "tracer"

    # pflotran
    path_to_pflotran = "pflotran"
    root_pflo = "1d-"+root  

     # hardwired for 1d-calcite: time and comp
    time = 'Time:  5.00000E+01 y'
    comp = 'Total_'+root.title()+' [M]'

    x_pflotran, c_pflotran = GetXY_PFloTran_1D(path_to_pflotran,root_pflo,time,comp)    
    
    # CrunchFlow: hardwired for calcite_1d_CF.in: time and comp
    times_CF = 'totcon5.out'
    comp = 0
    ignore = 4

    # crunchflow GIMRT
    path_to_crunchflow = "crunchflow/gimrt"
    x_crunchflow, c_crunchflow = GetXY_CrunchFlow_1D(path_to_crunchflow,root,times_CF,comp,ignore)

    # crunchflow OS3D
    path_to_crunchflow = "crunchflow/os3d"
    x_crunchOS3D, c_crunchOS3D = GetXY_CrunchFlow_1D(path_to_crunchflow,root,times_CF,comp,ignore)
    
    CWD = os.getcwd()
    local_path = "" 

    # amanziU

    try:
        comp = 'total_component_concentration.cell.tracer conc'
        input_file = os.path.join("amanzi-u-1d-"+root+"-alq-pflo.xml")
        path_to_amanzi = "output-u-alq-pflo"
        run_amanzi_standard.run_amanzi(input_file, 1, ["1d-"+root+".in",root+".dat",input_file], path_to_amanzi)
        x_amanzi_alquimia, c_amanzi_alquimia = GetXY_AmanziU_1D(path_to_amanzi,root,comp,1)
        alq = len(x_amanzi_alquimia)

    except:
        alq = 0

    try:
        comp = 'total_component_concentration.cell.tracer conc'
        input_file = os.path.join("amanzi-u-1d-"+root+"-alq-crunch.xml")
        path_to_amanzi = "output-u-alq-crunch"
        run_amanzi_standard.run_amanzi(input_file, 1, ["1d-"+root+"-crunch.in",root+".dbs",input_file], path_to_amanzi)
        x_amanzi_alquimia_crunch, c_amanzi_alquimia_crunch = GetXY_AmanziU_1D(path_to_amanzi,root,comp,1)
        alq_crunch = len(x_amanzi_alquimia_crunch)

    except:
        alq_crunch = 0

    # amanziS
    
    try:
        input_file = os.path.join("amanzi-s-1d-"+root+"-alq-pflo.xml")
        path_to_amanziS = "output-s-alq-pflo"
        run_amanzi_standard.run_amanzi(input_file, 1, ["1d-"+root+".in",root+".dat",input_file], path_to_amanziS)
        root_amanziS = "plt"
        compS = "tracer_water_Concentration"
        x_amanziS, c_amanziS = GetXY_AmanziS_1D(path_to_amanziS,root_amanziS,compS,1)
        struct = len(x_amanziS)
    except:
        struct = 0

    try:
        input_file = os.path.join("amanzi-s-1d-"+root+"-alq-crunch.xml")
        path_to_amanziS = "output-s-alq-crunch"
        run_amanzi_standard.run_amanzi(input_file, 1, ["1d-"+root+"-crunch.in",root+".dbs",input_file], path_to_amanziS)
        root_amanziS = "plt"
        compS = "tracer_water_Concentration"
        x_amanziS_crunch, c_amanziS_crunch = GetXY_AmanziS_1D(path_to_amanziS,root_amanziS,compS,1)
        struct_crunch = len(x_amanziS_crunch)
    except:
        struct_crunch = 0

# plotting --------------------------------------------------------

# subplots
    fig, ax = plt.subplots()

# pflotran
    ax.plot(x_pflotran, c_pflotran,'m-',label='PFloTran',linewidth=2)

# crunchflow
    ax.plot(x_crunchflow, c_crunchflow,'m--',label='CrunchFlow GIMRT',linewidth=2)
    ax.plot(x_crunchOS3D, c_crunchOS3D,'m*',label='CrunchFlow OS3D',linewidth=2) 

# unstruct amanzi alquimia + pflotran
    if alq>0:
        ax.plot(x_amanzi_alquimia, c_amanzi_alquimia,'r-',label='AmanziU+Alq(PFT)',linewidth=2)

    if alq_crunch>0:
        ax.plot(x_amanzi_alquimia_crunch, c_amanzi_alquimia_crunch,'r*',label='AmanziU+Alq(CF)',linewidth=2)

# struct amanzi alquimia + pflotran
    if (struct>0):
        sam = ax.plot(x_amanziS, c_amanziS,'g-',label='AmanziS+Alq(PFT)',linewidth=2)     

    if (struct_crunch>0):
        sam = ax.plot(x_amanziS_crunch, c_amanziS_crunch,'g*',label='AmanziS+Alq(CF)',linewidth=2)     

# figure look
    # axes
    ax.set_xlabel("Distance (m)",fontsize=20)
    ax.set_ylabel("Total "+root.title()+" concentration [mol/L]",fontsize=20)

    # plot adjustments
    plt.subplots_adjust(left=0.20,bottom=0.15,right=0.95,top=0.90)
    plt.legend(loc='upper right',fontsize=13)
    plt.suptitle("Amanzi 1D "+root.title()+" Benchmark at 50 years",x=0.57,fontsize=20)
    plt.tick_params(axis='both', which='major', labelsize=20)

    plt.savefig(root+"_1d.png",format="png")
#   plt.show()
#   plt.close()

