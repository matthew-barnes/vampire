//------------------------------------------------------------------------------
//
//   This file is part of the VAMPIRE open source package under the
//   Free BSD licence (see licence file for details).
//
//   (c) Andrea Meo, Rory Pond and Richard F L Evans 2016. All rights reserved.
//
//   Email: richard.evans@york.ac.uk
//
//------------------------------------------------------------------------------
//

// C++ standard library headers

// Vampire headers
#include "config.hpp"
#include "sim.hpp"

// config module headers
#include "internal.hpp"

namespace config
{

//------------------------------------------------------------------------------
// Function to output atomic and cell coordinates to disk
//------------------------------------------------------------------------------
void output(){ // should include variables for data to be outputted, eg spins, cells etc

   // check for data output enabled, if not no nothing
   if(config::internal::output_atoms_config == false && config::internal::output_cells_config == false) return;

   // check that config module has been initialised
   if(!config::internal::initialised) config::internal::initialize();

   //------------------------------------------------------------------------------------------
   // Calculate field ranges for output in limited applied ranges during hysteresis
   //------------------------------------------------------------------------------------------

   double minField_1;
   double maxField_1;
   double minField_2;
   double maxField_2;

   // check that minField_1>maxField_1
   if (config::internal::field_output_min_1 >= config::internal::field_output_max_1)
   {
      minField_1 = config::internal::field_output_min_1;
      maxField_1 = config::internal::field_output_max_1;
   }
   else
   {
      minField_1 = config::internal::field_output_max_1;
      maxField_1 = config::internal::field_output_min_1;
   }
   // check that maxField_2>minField_2
   if (config::internal::field_output_max_2 >= config::internal::field_output_min_2)
   {
      minField_2 = config::internal::field_output_min_2;
      maxField_2 = config::internal::field_output_max_2;
   }
   else
   {
      minField_2 = config::internal::field_output_max_2;
      maxField_2 = config::internal::field_output_min_2;
   }

   //------------------------------------------------------
   // atoms output if enabled and the time is right
   //------------------------------------------------------
   if ((config::internal::output_atoms_config == true) && (sim::output_rate_counter % config::internal::output_atoms_config_rate == 0))
   {
      // for all programs except hysteresis
      if (sim::program != 2)
      {
         if (config::internal::output_rate_counter_coords == 0) config::internal::atoms_coords();
         config::internal::atoms();
         config::internal::output_rate_counter_coords++;
      }
      // for hysteresis program
      else if (sim::program == 2)
      {
         // output config only in range [minField_1;maxField_1] for decreasing field
         if ((sim::H_applied >= maxField_1) && (sim::H_applied <= minField_1) && (sim::parity < 0))
         {
            if (config::internal::output_rate_counter_coords == 0) config::internal::atoms_coords();
            config::internal::atoms();
            config::internal::output_rate_counter_coords++;
         }
         // output config only in range [minField_2;maxField_2] for increasing field
         else if ((sim::H_applied >= minField_2) && (sim::H_applied <= maxField_2) && (sim::parity > 0))
         {
            if (config::internal::output_rate_counter_coords == 0) config::internal::atoms_coords();
            config::internal::atoms();
            config::internal::output_rate_counter_coords++;
         }
      }
   }

   //------------------------------------------------------
   // cells output if enabled and the time is right
   //------------------------------------------------------
   if ((config::internal::output_cells_config == true) && (sim::output_rate_counter % config::internal::output_cells_config_rate == 0))
   {
      // for all programs except hysteresis
      if (sim::program != 2)
      {
         if (sim::output_cells_file_counter == 0) config::internal::legacy_cells_coords();
         config::internal::legacy_cells();
      }
      // for hysteresis program
      else if (sim::program == 2)
      {
         // output config only in range [minField_1;maxField_1] for decreasing field
         if ((sim::H_applied >= maxField_1) && (sim::H_applied <= minField_1) && (sim::parity < 0))
         {
            if (sim::output_cells_file_counter == 0) config::internal::legacy_cells_coords();
            config::internal::legacy_cells();
         }
         // output config only in range [minField_2;maxField_2] for increasing field
         else if ((sim::H_applied >= minField_2) && (sim::H_applied <= maxField_2) && (sim::parity > 0))
         {
            if (sim::output_cells_file_counter == 0) config::internal::legacy_cells_coords();
            config::internal::legacy_cells();
         }
      }
   }

   // increment rate counter
   sim::output_rate_counter++;
}

}