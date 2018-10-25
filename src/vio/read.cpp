//------------------------------------------------------------------------------
//
//   This file is part of the VAMPIRE open source package under the
//   Free BSD licence (see licence file for details).
//
//   (c) Richard F L Evans and Rory Pond 2016. All rights reserved.
//
//   Email: richard.evans@york.ac.uk and rory.pond@york.ac.uk
//
//------------------------------------------------------------------------------
//

// C++ standard library headers
#include <algorithm>
#include <sstream>
#include <cstring>
#include <string>
#include <iostream>
// Vampire headers
#include "vio.hpp"
#include "errors.hpp"

// vio module headers
#include "internal.hpp"

namespace vin{
	// Function to extract all variables from a string and return a vector
	std::vector<double> doubles_from_string(std::string value){

		// array for storing variables
		std::vector<double> array(0);

		// set source for ss
		std::istringstream source(value);

		// double variable to store values
		double temp = 0.0;

		// string to store text
		std::string field;

		// loop over all comma separated values
		while(getline(source,field,',')){

			// convert string to ss
			std::stringstream fs(field);

			// read in variable
			fs >> temp;

			// push data value back to array
			array.push_back(temp);

		}

		// return values to calling function
		return array;

	}

	/// @brief Function to read in variables from a file.
	///
	/// @section License
	/// Use of this code, either in source or compiled form, is subject to license from the authors.
	/// Copyright \htmlonly &copy \endhtmlonly Richard Evans, 2009-2010. All Rights Reserved.
	///
	/// @section Information
	/// @author  Richard Evans, rfle500@york.ac.uk
	/// @version 1.1
	/// @date    18/01/2010
	///
	/// @param[in] filename Name of file to be opened
	/// @return EXIT_SUCCESS
	///
	/// @internal
	///	Created:		14/01/2010
	///	Revision:	  ---
	///=====================================================================================
	///
	int read(string const filename){
		// Print informative message to zlog file
		zlog << zTs() << "Opening main input file \"" << filename << "\"." << std::endl;

		std::stringstream inputfile;
		inputfile.str( vin::get_string(filename.c_str(), "input", -1) );

		// Print informative message to zlog file
		zlog << zTs() << "Parsing system parameters from main input file." << std::endl;

		int line_counter=0;
		// Loop over all lines and pass keyword to matching function
                char com = '#';
                char delim[] = ":=!";
                char cstr[512];
		while (! inputfile.eof() ){
			line_counter++;
			// read in whole line
			std::string line;
			getline(inputfile,line);

			// Clear whitespace and tabs
			line.erase(remove(line.begin(), line.end(), '\t'), line.end());
			line.erase(remove(line.begin(), line.end(), ' '), line.end());

			// clear carriage return for dos formatted files
			line.erase(remove(line.begin(), line.end(), '\r'), line.end());
                        

			// strip key,word,unit,value
			std::string key="";
			std::string word="";
			std::string value="";
			std::string unit="";

			// get size of string
			int linelength = line.length();
                        // max size of char array cstr is 512, error if greater than this. // Strictly, could do after stripping comments. 
                        if (linelength > 510){std::cout << "Input line: " << line_counter << " is too long."<<std::endl;};

                        // remove everything after comment character
                        line = line.substr(0,line.find('#')) ;

                        // convert to c-string style, for tokenisation
                        //char *cstr = new char[line.length() + 1]; // Don't use new pointers. 

                        strcpy(cstr, line.c_str());

                        // tokenise the string, using delimiters from above
                        char *token = strtok(cstr,delim);  // If this is still too much pointing, 
                                                           // an alternative would be to use strcspn 
                                                           //or std::string find and stripping the beginning based on this. 
                        for (int count = 0; count < 4 && token !=NULL; count++){
                            if (count==0){key=token;}       // Format is always the same
                            else if(count==1){word=token;}  // but breaks if EOL found
                            else if(count==2){value=token;}
                            else if(count==3){unit=token;}
                            token = strtok(NULL,delim);
                            };

                        // tidy up
			string empty="";
			if(key!=empty){
			//std::cout << "\t" << "key:  " << key << std::endl;
			//std::cout << "\t" << "word: " << word << std::endl;
			//std::cout << "\t" << "value:" << value << std::endl;
			//std::cout << "\t" << "unit: " << unit << std::endl;
			int matchcheck = match(key, word, value, unit, line_counter);
			if(matchcheck==EXIT_FAILURE){
				err::vexit();
			}
			}
		}

		return EXIT_SUCCESS;
	}


}
