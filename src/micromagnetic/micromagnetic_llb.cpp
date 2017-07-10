
// Vampire headers
#include "micromagnetic.hpp"

// micromagnetic module headers
#include "internal.hpp"

#include "random.hpp"
#include "errors.hpp"
#include "atoms.hpp"
#include "cells.hpp"
#include "sim.hpp"
#include "vmpi.hpp"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <fstream>

namespace mm = micromagnetic::internal;


//function declaration for the serial LLB
int LLB_serial_heun( std::vector <int> local_cell_array,
										 int num_steps,
                     int num_cells,
										 int num_local_cells,
                     double temperature,
                     std::vector<double>& x_mag_array,
                     std::vector<double>& y_mag_array,
                     std::vector<double>& z_mag_array,
                     double Hx,
                     double Hy,
                     double Hz,
                     double H,
                     double dt,
                     std::vector <double> volume_array
                  );


namespace micromagnetic_arrays{

	// Local arrays for LLG integration
	std::vector <double> x_euler_array;
	std::vector <double> y_euler_array;
	std::vector <double> z_euler_array;

	std::vector <double> x_array;
	std::vector <double> y_array;
	std::vector <double> z_array;

	std::vector <double> x_heun_array;
	std::vector <double> y_heun_array;
	std::vector <double> z_heun_array;

	std::vector <double> x_spin_storage_array;
	std::vector <double> y_spin_storage_array;
	std::vector <double> z_spin_storage_array;

	std::vector <double> x_initial_spin_array;
	std::vector <double> y_initial_spin_array;
	std::vector <double> z_initial_spin_array;

	bool LLG_set=false; ///< Flag to define state of LLG arrays (initialised/uninitialised)

}


namespace micromagnetic{

  int micromagnetic_init(int num_cells){

  	// check calling of routine if error checking is activated
  	if(err::check==true) std::cout << "LLB_init has been called" << std::endl;
		////std::cout << "called" <<std::endl;
  	using namespace micromagnetic_arrays;
		x_spin_storage_array.resize(num_cells,0.0);
  	y_spin_storage_array.resize(num_cells,0.0);
  	z_spin_storage_array.resize(num_cells,0.0);

		x_array.resize(num_cells,0.0);
  	y_array.resize(num_cells,0.0);
  	z_array.resize(num_cells,0.0);

  	x_initial_spin_array.resize(num_cells,0.0);
  	y_initial_spin_array.resize(num_cells,0.0);
  	z_initial_spin_array.resize(num_cells,0.0);

  	x_euler_array.resize(num_cells,0.0);
  	y_euler_array.resize(num_cells,0.0);
  	z_euler_array.resize(num_cells,0.0);

  	x_heun_array.resize(num_cells,0.0);
  	y_heun_array.resize(num_cells,0.0);
  	z_heun_array.resize(num_cells,0.0);

  	LLG_set=true;

    	return EXIT_SUCCESS;
  }

int LLB( std::vector <int> local_cell_array,
							int num_steps,
                     int num_cells,
							int num_local_cells,
                     double temperature,
                     std::vector<double>& x_mag_array,
                     std::vector<double>& y_mag_array,
                     std::vector<double>& z_mag_array,
                     double Hx,
                     double Hy,
                     double Hz,
                     double H,
                     double dt,
                     std::vector <double> volume_array
                  ){

	// check calling of routine if error checking is activated
	if(err::check==true){std::cout << "micromagnetic::LLG_Heun has been called" << std::endl;}

  using namespace micromagnetic_arrays;

	// Check for initialisation of LLG integration arrays
	if(LLG_set== false) micromagnetic::micromagnetic_init(num_cells);
	// Local variables for system integration

	//save this new m as the initial value, so it can be saved and used in the final equation.
	for (int lc = 0; lc < num_local_cells; lc++){
		int cell = local_cell_array[lc];
		x_array[cell] = x_mag_array[cell]/mm::ms[cell];
		y_array[cell] = y_mag_array[cell]/mm::ms[cell];
		z_array[cell] = z_mag_array[cell]/mm::ms[cell];
		x_initial_spin_array[cell] = x_array[cell];
		y_initial_spin_array[cell] = y_array[cell];
		z_initial_spin_array[cell] = z_array[cell];
	}
	//std::cout << "bc" <<std::endl;

	const double kB = 1.3806503e-23;

	std::vector<double> m(3,0.0);
	std::vector<double> spin_field(3,0.0);

	//The external fields equal the length of the field times the applied field vector.
	//This is saved to an array.
	mm::ext_field[0] = H*Hx;
	mm::ext_field[1] = H*Hy;
	mm::ext_field[2] = H*Hz;


//std::cout << "bd" <<std::endl;

	//calculte chi(T).
	mm::one_o_chi_para =  mm::calculate_chi_para(num_local_cells,local_cell_array, num_cells, temperature);
	mm::one_o_chi_perp =  mm::calculate_chi_perp(num_local_cells,local_cell_array, num_cells, temperature);

	//6 arrays of gaussian random numbers to store the stochastic noise terms for x,y,z parallel and perperdicular
	std::vector <double> GW1x(num_cells,0.0);
	std::vector <double> GW1y(num_cells,0.0);
	std::vector <double> GW1z(num_cells,0.0);
	std::vector <double> GW2x(num_cells,0.0);
	std::vector <double> GW2y(num_cells,0.0);
	std::vector <double> GW2z(num_cells,0.0);

//std::cout << "be" <<std::endl;

	//fill the noise terms
	generate (GW1x.begin(),GW1x.end(), mtrandom::gaussian);
	generate (GW1y.begin(),GW1y.end(), mtrandom::gaussian);
	generate (GW1z.begin(),GW1z.end(), mtrandom::gaussian);
	generate (GW2x.begin(),GW2x.end(), mtrandom::gaussian);
	generate (GW2y.begin(),GW2y.end(), mtrandom::gaussian);
	generate (GW2z.begin(),GW2z.end(), mtrandom::gaussian);

	//if (micromagnetic::stochastic == true) a = 1.0;
	//else if (micromagnetic::stochastic == false) a = 0.0;
	//loops over all the cells to calculate the spin terms per cell - only filled cells where MS>0
	for (int lc = 0; lc < number_of_micromagnetic_cells; lc++){
		int cell = list_of_micromagnetic_cells[lc];
		m[0] = x_array[cell];
		m[1] = y_array[cell];
		m[2] = z_array[cell];

		spin_field = mm::calculate_llb_fields(m, temperature, num_cells, cell, x_array,y_array,z_array);
		//if (cell == 0 ) std::cout << m[0] << "\t" << spin_field[0] <<std::endl;
		//calculates the stochatic parallel and perpendicular terms
		////std::cout << dt << "\t" << temperature << '\t' << kB << '\t' << mm::alpha_para[cell] << '\t' << mm::alpha_perp[cell] << '\t' << mm::ms[cell] << '\t' << sigma_para << '\t' << sigma_perp << std::endl;
		double sigma_para = sqrt(2*kB*temperature*mm::alpha_para[cell]/(mm::ms[cell]*dt)); //why 1e-27
		double sigma_perp = sqrt(2*kB*temperature*(mm::alpha_perp[cell]-mm::alpha_para[cell])/(dt*mm::ms[cell]*mm::alpha_perp[cell]*mm::alpha_perp[cell]));
		const double H[3] = {spin_field[0], spin_field[1], spin_field[2]};

		//saves the noise terms to an array
		const double GW2t[3] = {GW2x[cell],GW2y[cell],GW2z[cell]};
		const double one_o_m_squared = 1.0/(m[0]*m[0]+m[1]*m[1]+m[2]*m[2]);
		const double SdotH = m[0]*H[0] + m[1]*H[1] + m[2]*H[2];


		double xyz[3] = {0.0,0.0,0.0};
		//calculates the LLB equation
	//if (cell==0)	std:: cout << m[0] << '\t' << m[1] << '\t' << m[2] << "\t" << H[0] << '\t' << H[1] << '\t' << H[2] << "\t" << m[1]*H[2] << '\t'  << m[2]*H[1] << std::endl;
 		xyz[0]=  - (m[1]*H[2]-m[2]*H[1])
						 + mm::alpha_para[cell]*m[0]*SdotH*one_o_m_squared
						 - mm::alpha_perp[cell]*(m[1]*(m[0]*H[1]-m[1]*H[0])-m[2]*(m[2]*H[0]-m[0]*H[2]))*one_o_m_squared
						 + GW1x[cell]*sigma_para
						 - mm::alpha_perp[cell]*(m[1]*(m[0]*GW2t[1]-m[1]*GW2t[0])-m[2]*(m[2]*GW2t[0]-m[0]*GW2t[2]))*one_o_m_squared*sigma_perp;

		xyz[1]=  - (m[2]*H[0]-m[0]*H[2])
						 + mm::alpha_para[cell]*m[1]*SdotH*one_o_m_squared
						 - mm::alpha_perp[cell]*(m[2]*(m[1]*H[2]-m[2]*H[1])-m[0]*(m[0]*H[1]-m[1]*H[0]))*one_o_m_squared
						 + GW1y[cell]*sigma_para
						 - mm::alpha_perp[cell]*(m[2]*(m[1]*GW2t[2]-m[2]*GW2t[1])-m[0]*(m[0]*GW2t[1]-m[1]*GW2t[0]))*one_o_m_squared*sigma_perp;

		xyz[2]=	 - (m[0]*H[1]-m[1]*H[0])
						 + mm::alpha_para[cell]*m[2]*SdotH*one_o_m_squared
						 - mm::alpha_perp[cell]*(m[0]*(m[2]*H[0]-m[0]*H[2])-m[1]*(m[1]*H[2]-m[2]*H[1]))*one_o_m_squared
						 + GW1z[cell]*sigma_para
						 - mm::alpha_perp[cell]*(m[0]*(m[2]*GW2t[0]-m[0]*GW2t[2])-m[1]*(m[1]*GW2t[2]-m[2]*GW2t[1]))*one_o_m_squared*sigma_perp;

		x_euler_array[cell] = xyz[0];
		y_euler_array[cell] = xyz[1];
		z_euler_array[cell] = xyz[2];
		//if(cell == 0) std::cout << (m[1]*H[2]-m[2]*H[1]) << '\t' <<mm::alpha_para[cell]*m[0]*SdotH*one_o_m_squared <<"\t" << mm::alpha_perp[cell]*(m[1]*(m[0]*H[1]-m[1]*H[0])-m[2]*(m[2]*H[0]-m[0]*H[2]))*one_o_m_squared << "\t" << x_euler_array[cell] << std::endl;
	}

	//these new x postiion are stored in an array (store)
	//x = x+step*dt
	for (int lc = 0; lc < number_of_micromagnetic_cells; lc++){
		int cell = list_of_micromagnetic_cells[lc];
		x_spin_storage_array[cell] = x_array[cell] + x_euler_array[cell]*dt;
		y_spin_storage_array[cell] = y_array[cell] + y_euler_array[cell]*dt;
		z_spin_storage_array[cell] = z_array[cell] + z_euler_array[cell]*dt;
	}

	//loops over all the cells to calculate the spin terms per cell - only filled cells where MS>0
	for (int lc = 0; lc < number_of_micromagnetic_cells; lc++){
		int cell = list_of_micromagnetic_cells[lc];

		m[0] = x_spin_storage_array[cell];
		m[1] = y_spin_storage_array[cell];
		m[2] = z_spin_storage_array[cell];

		spin_field = mm::calculate_llb_fields(m, temperature, num_cells, cell, x_array,y_array,z_array);

		//fill the noise terms
		generate (GW1x.begin(),GW1x.end(), mtrandom::gaussian);
		generate (GW1y.begin(),GW1y.end(), mtrandom::gaussian);
		generate (GW1z.begin(),GW1z.end(), mtrandom::gaussian);
		generate (GW2x.begin(),GW2x.end(), mtrandom::gaussian);
		generate (GW2y.begin(),GW2y.end(), mtrandom::gaussian);
		generate (GW2z.begin(),GW2z.end(), mtrandom::gaussian);

		//calculates the stochatic parallel and perpendicular terms
		double sigma_para = sqrt(2*kB*temperature*mm::alpha_para[cell]/(mm::ms[cell]*dt)); //why 1e-27
		double sigma_perp = sqrt(2*kB*temperature*(mm::alpha_perp[cell]-mm::alpha_para[cell])/(dt*mm::ms[cell]*mm::alpha_perp[cell]*mm::alpha_perp[cell]));

		const double H[3] = {spin_field[0], spin_field[1], spin_field[2]};

		//saves the noise terms to an array
		const double GW2t[3] = {GW2x[cell],GW2y[cell],GW2z[cell]};
		const double one_o_m_squared = 1.0/(m[0]*m[0]+m[1]*m[1]+m[2]*m[2]);
		const double SdotH = m[0]*H[0] + m[1]*H[1] + m[2]*H[2];


		double xyz[3] = {0.0,0.0,0.0};
		//calculates the LLB equation
		xyz[0]=  - (m[1]*H[2]-m[2]*H[1])
						 + mm::alpha_para[cell]*m[0]*SdotH*one_o_m_squared
						 - mm::alpha_perp[cell]*(m[1]*(m[0]*H[1]-m[1]*H[0])-m[2]*(m[2]*H[0]-m[0]*H[2]))*one_o_m_squared
						 + GW1x[cell]*sigma_para
						 - mm::alpha_perp[cell]*(m[1]*(m[0]*GW2t[1]-m[1]*GW2t[0])-m[2]*(m[2]*GW2t[0]-m[0]*GW2t[2]))*one_o_m_squared*sigma_perp;

		xyz[1]=  - (m[2]*H[0]-m[0]*H[2])
						 + mm::alpha_para[cell]*m[1]*SdotH*one_o_m_squared
						 - mm::alpha_perp[cell]*(m[2]*(m[1]*H[2]-m[2]*H[1])-m[0]*(m[0]*H[1]-m[1]*H[0]))*one_o_m_squared
						 + GW1y[cell]*sigma_para
						 - mm::alpha_perp[cell]*(m[2]*(m[1]*GW2t[2]-m[2]*GW2t[1])-m[0]*(m[0]*GW2t[1]-m[1]*GW2t[0]))*one_o_m_squared*sigma_perp;

		xyz[2]=	 - (m[0]*H[1]-m[1]*H[0])
						 + mm::alpha_para[cell]*m[2]*SdotH*one_o_m_squared
						 - mm::alpha_perp[cell]*(m[0]*(m[2]*H[0]-m[0]*H[2])-m[1]*(m[1]*H[2]-m[2]*H[1]))*one_o_m_squared
						 + GW1z[cell]*sigma_para
						 - mm::alpha_perp[cell]*(m[0]*(m[2]*GW2t[0]-m[0]*GW2t[2])-m[1]*(m[1]*GW2t[2]-m[2]*GW2t[1]))*one_o_m_squared*sigma_perp;


		x_heun_array[cell] = xyz[0];
		y_heun_array[cell] = xyz[1];
		z_heun_array[cell] = xyz[2];
	}
	//calcualtes the final position as x = xinital + 1/2(euler+heun)*dt
	for (int lc = 0; lc < number_of_micromagnetic_cells; lc++){
		int cell = list_of_micromagnetic_cells[lc];

		 x_array[cell] = x_initial_spin_array[cell] + 0.5*dt*(x_euler_array[cell] + x_heun_array[cell]);
		 y_array[cell] = y_initial_spin_array[cell] + 0.5*dt*(y_euler_array[cell] + y_heun_array[cell]);
		 z_array[cell] = z_initial_spin_array[cell] + 0.5*dt*(z_euler_array[cell] + z_heun_array[cell]);
//if (cell == 0) std::cout << x_array[cell] << '\t' << dt << "\t" <<x_initial_spin_array[cell]  << '\t' << x_euler_array[cell] << '\t' << x_heun_array[cell] <<std::endl;
		cells::mag_array_x[cell] = x_array[cell]*mm::ms[cell];
		cells::mag_array_y[cell] = y_array[cell]*mm::ms[cell];
		cells::mag_array_z[cell] = z_array[cell]*mm::ms[cell];

	}
	for(int atom_list=0;atom_list<number_of_none_atomistic_atoms;atom_list++){
		 int atom = list_of_none_atomistic_atoms[atom_list];
		 int cell = cells::atom_cell_id_array[atom];
		 atoms::x_spin_array[atom] = x_array[cell];
		 atoms::y_spin_array[atom] = y_array[cell];
		 atoms::z_spin_array[atom] = z_array[cell];

	}
//std::cout << "bh" <<std::endl;
	return 0;

}


}
