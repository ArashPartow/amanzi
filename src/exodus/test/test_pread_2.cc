// -------------------------------------------------------------
/**
 * @file   test_pread_2.cc
 * @author William A. Perkins
 * @date Thu Apr  7 09:37:16 2011
 * 
 * @brief  
 * 
 * 
 */
// -------------------------------------------------------------
// -------------------------------------------------------------
// Created November 15, 2010 by William A. Perkins
// Last Change: Thu Apr  7 09:37:16 2011 by William A. Perkins <d3g096@PE10900.pnl.gov>
// -------------------------------------------------------------

#include <iostream>
#include <UnitTest++.h>

#include <Epetra_Map.h>
#include <Epetra_MpiComm.h>

#include "dbc.hh"
#include "../Parallel_Exodus_file.hh"
#include "../Exodus_error.hh"

extern std::string split_file_path(const std::string& fname);
extern void checkit(ExodusII::Parallel_Exodus_file & thefile);

SUITE (Exodus_2_Proc)
{
  TEST (bogus)
  {
    Epetra_MpiComm comm(MPI_COMM_WORLD);
    ExodusII::Parallel_Exodus_file *thefile = NULL;
    CHECK_THROW((thefile = new ExodusII::Parallel_Exodus_file(comm, "Some.Bogus.par")),
                ExodusII::ExodusError);
    if (thefile != NULL) delete thefile;
  }

  TEST (hex_4x4x4_ss)
  {
    std::string bname(split_file_path("hex_4x4x4_ss.par").c_str());
    
    Epetra_MpiComm comm(MPI_COMM_WORLD);

    CHECK_EQUAL(comm.NumProc(), 2);
    
    ExodusII::Parallel_Exodus_file thefile(comm, bname);
    checkit(thefile);
  }


  TEST (htc_rad_test_random)
  {
    std::string bname(split_file_path("htc_rad_test-random.par").c_str());
    
    Epetra_MpiComm comm(MPI_COMM_WORLD);

    CHECK_EQUAL(comm.NumProc(), 2);
    
    ExodusII::Parallel_Exodus_file thefile(comm, bname);
    checkit(thefile);
      
  }

  TEST (hex_11x11x11_ss)
  {
    std::string bname(split_file_path("hex_11x11x11_ss.par").c_str());
    
    Epetra_MpiComm comm(MPI_COMM_WORLD);

    CHECK_EQUAL(comm.NumProc(), 2);
    
    ExodusII::Parallel_Exodus_file thefile(comm, bname);
    checkit(thefile);
      
  }

}
