// Vampire headers
#include "environment.hpp"

// micromagnetic module headers
#include "internal.hpp"
#include "sim.hpp"
#include <math.h>
namespace environment{

   namespace internal{

     int output(){

       double mx = 0;
     	 double my = 0;
     	 double mz = 0;
     	 double ml = 0;

       for (int cell = 0; cell < num_cells; cell++){

   			   mx =  mx + x_mag_array[cell];
   			   my =  my + y_mag_array[cell];
   			   mz =  mz + z_mag_array[cell];
   			   ml =  ml + Ms;
   		}

   		double msat = ml;
   		double magm = sqrt(mx*mx + my*my + mz*mz);
   		mx = mx/magm;
   		my = my/magm;
   		mz = mz/magm;

   		o_file <<sim::time << '\t' << sim::temperature << "\t" << mx << '\t' << my<< '\t' << mz << '\t' <<  magm/msat << std::endl;
         std::cout <<sim::time << '\t' << sim::temperature << "\t" << mx << '\t' << my<< '\t' << mz << '\t' <<  magm/msat << std::endl;

         // std::ofstream pfile;
  	// 		pfile.open("env_cell_config");
         //
  	// 		for (int cell = 0; cell < num_cells; cell++){
  	// 			//pfile2 << cell << '\t' << cell_coords_array_x[cell] << '\t' << cell_coords_array_y[cell] << '\t' << cell_coords_array_z[cell] << '\t' <<mag_array_x[cell] <<
  	// 			pfile << cell_coords_array_x[cell] << '\t' << cell_coords_array_y[cell] << '\t' << cell_coords_array_z[cell] << '\t' <<x_mag_array[cell] << '\t' << y_mag_array[cell] << '\t' << z_mag_array[cell] << '\t' << std::endl;
  	// 		}


       return 0;
     }
   }
}
