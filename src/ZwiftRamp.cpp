//============================================================================
// Name        : ZwiftRamp.cpp
// Author      : Vincent Golle
// Version     :
// Copyright   : 
// Description :
//============================================================================

#include <iostream>
#include <fstream>

#include <stdint.h>
#include <string>
#include <assert.h>
#include <vector>
#include <list>

using namespace std;

/*
 * It will iterate through all the lines in file and
 * put them in given vector
 */
static bool getFileContent(std::string fileName, std::list<std::string> & vecOfStrs)
{

	// Open the File
	std::ifstream in(fileName.c_str());

	// Check if object is valid
	if (!in)
	{
		std::cerr << "Cannot open the File : " << fileName << std::endl;
		return false;
	}

	std::string str;
	// Read the next line from File until it reaches the end.
	while (std::getline(in, str))
	{
		// Line contains string of length > 0 then save it in vector
		if (str.size() > 0)
			vecOfStrs.push_back(str);
	}
	//Close The File
	in.close();
	return true;
}

static void createIntervals(std::list<std::string> & vecOfStrs, int sec, float powerLow, float powerHigh, int cadence) {

	int nb_inter = sec / 10;
	int dftp = (powerHigh - powerLow) / nb_inter;
	int ftp = powerLow + dftp / 2;

	char buff[512];

	do {

		if (cadence) {

			snprintf(buff, sizeof(buff), "\t\t<SteadyState Duration=\"%d\" Power=\"%d.%02d\" Cadence=\"%d\"/>",
					10,
					(int)(ftp / 100), ftp % 100,
					cadence);
		} else {

			snprintf(buff, sizeof(buff), "\t\t<SteadyState Duration=\"%d\" Power=\"%d.%02d\"/>",
					10,
					(int)(ftp / 100), ftp % 100);
		}

		ftp += dftp;

		std::string line = buff;

		vecOfStrs.push_back(line);

	} while (--nb_inter);

}

static void replaceRamps(std::list<std::string> & vecOfStrs) {

	// <Ramp Duration="300" PowerLow="0.25" PowerHigh="0.65"/>
	//

	std::list<std::string> outList;

	for (auto _line : vecOfStrs) {

		std::string line = _line;

		if (line.find("Ramp") != std::string::npos) {

			// remove leading garbage
			while (line.at(0) != '<') {

				line = line.substr(1);
			}

			std::cout << "Short ramp found : " << line << std::endl;

			int duration, cadence=0;
			float powerLow, powerHigh;

			if (line.find("Cadence") != std::string::npos) {

				// <Ramp Duration=\"%d\" PowerLow=\"%d.%02d\" PowerHigh=\"%d.%02d\" Cadence=\"%d\"/>"

				int res = sscanf(line.c_str(), "<Ramp Duration=\"%d\" PowerLow=\"%f\" PowerHigh=\"%f\" Cadence=\"%d\"/>", &duration, &powerLow, &powerHigh, &cadence);
				assert(res == 4);

			} else {

				// <Ramp Duration=\"%d\" PowerLow=\"%d.%02d\" PowerHigh=\"%d.%02d\"/>"
				int res = sscanf(line.c_str(), "<Ramp Duration=\"%d\" PowerLow=\"%f\" PowerHigh=\"%f\"/>", &duration, &powerLow, &powerHigh);
				assert(res == 3);
			}

			// ramp is too long
			if (duration > 60) {

				outList.push_back(_line);
				continue;
			}

			// ramp is short
			createIntervals(outList, duration, powerLow, powerHigh, cadence);

		} else {

			outList.push_back(_line);
		}

	}

	vecOfStrs.clear();

	vecOfStrs = outList;

}

void save(std::string wname, std::list<std::string> & _series) {

	std::ofstream fCalibrations;

	// remove surplus
	while (wname.find("\\") != std::string::npos ||
			wname.find("/") != std::string::npos) {

		wname = wname.substr(1);
	}
	while (wname.find(".") != std::string::npos) {

		wname.pop_back();
	}

	std::string new_name = "out/";
	new_name += wname;
	new_name += ".zwo";

	std::cout << "Saving to file : " << new_name << std::endl;

	fCalibrations.open(new_name);

	for (auto line : _series) {

		fCalibrations << line << std::endl;
	}

	fCalibrations.close();
}

int main(int argc, char *argv[]) {

	int tmp_argc = 1;

	assert(argc >= 2);

	do {

		std::list<std::string> vecOfStr;

		const char * fname = argv[tmp_argc];

		// Get the contents of file in a vector
		if (getFileContent(fname, vecOfStr)) {

			replaceRamps(vecOfStr);

			std::string name = fname;
			save(name, vecOfStr);

		} else {

			return -1;
		}

		tmp_argc++;

	} while (tmp_argc < argc);

	return 0;
}

