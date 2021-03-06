#include "model.h"
#include "atom.h"
#include "controller.h"
#include "misc.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream> // stringstream
#include <cassert>
#include <exception>
#include <iterator>
#include <algorithm>

///////////////////////////////
// AUX FUNCTION DECLARATIONS //
///////////////////////////////

bool isAtomLine(const std::vector<std::string>& substrings);
std::string strToValidSymbol(std::string str);
static inline std::vector<std::string> splitLine(std::string& line);

/////////////////
// FILE IMPORT //
/////////////////

// reads radii from a file specified by the filepath and
// stores them in an unordered map that is an attribute of
// the Model object
void Model::readRadiiAndAtomNumFromFile(std::string& filepath){
  // clear unordered_maps to avoid keeping data from previous runs
  raw_radius_map.clear();
  elem_Z.clear();

  std::string line;
  std::ifstream inp_file(filepath);

  while(getline(inp_file,line)){
    std::vector<std::string> substrings = splitLine(line);
    if(substrings.size() == 3){
      // TODO: make sure substrings[1] is converted to valid symbol
      raw_radius_map[substrings[1]] = std::stod(substrings[2]);
      elem_Z[substrings[1]] = std::stoi(substrings[0]);
    }
  }
  // Notify the user if no radius is defined
  // the program can continue running because the user can manually define radii
  if (raw_radius_map.size() == 0) {
    Ctrl::getInstance()->notifyUser("Invalid radii definition file!");
    Ctrl::getInstance()->notifyUser("Please select a valid file or set radii manually.");
  }
}

bool Model::readAtomsFromFile(const std::string& filepath, bool include_hetatm){

  atom_amounts.clear();
  raw_atom_coordinates.clear();

  if (fileExtension(filepath) == "xyz"){
    readFileXYZ(filepath);
  }
  else if (fileExtension(filepath) == "pdb"){
    readFilePDB(filepath, include_hetatm);
  }
  else { // The browser does not allow other file formats but a user could manually write the path to an invalid file
    Ctrl::getInstance()->notifyUser("Invalid structure file format!");
    return false;
  }
  if (raw_atom_coordinates.size() == 0){ // If no atom is detected in the input file, the file is deemed invalid
    Ctrl::getInstance()->notifyUser("Invalid structure file!");
    return false;
  }
  return true;
}

void Model::readFileXYZ(const std::string& filepath){

//if (inp_file.is_open()){  //TODO consider adding an exception, for when file in not valid
  std::string line;
  std::ifstream inp_file(filepath);

  // iterate through lines
  while(getline(inp_file,line)){
    // divide line into "words"
    std::vector<std::string> substrings = splitLine(line);
    // create new atom and add to storage vector if line format corresponds to Element_Symbol x y z
    if (isAtomLine(substrings)) {

      std::string valid_symbol = strToValidSymbol(substrings[0]);
      atom_amounts[valid_symbol]++; // adds one to counter for this symbol

      // if a key leads to multiple z-values, set z-value to 0 (?)
      if (elem_Z.count(valid_symbol) > 0){
        elem_Z[valid_symbol] = 0;
      }
      // Stores the full list of atom coordinates from the input file
      raw_atom_coordinates.emplace_back(valid_symbol,
                                        std::stod(substrings[1]),
                                        std::stod(substrings[2]),
                                        std::stod(substrings[3]));
    }
  }
  // file has been read
  inp_file.close();
}

void Model::readFilePDB(const std::string& filepath, bool include_hetatm){

//if (inp_file.is_open()){  //TODO consider adding an exception, for when file in not valid
  std::string line;
  std::ifstream inp_file(filepath);

  // iterate through lines
  while(getline(inp_file,line)){
    if (line.substr(0,6) == "ATOM  " || (include_hetatm == true && line.substr(0,6) == "HETATM")){
      // Element symbol is located at characters 77 and 78, right-justified in the official pdb format
      std::string symbol = line.substr(76,2);
      // Some software generate pdb files with symbol left-justified instead of right-justified
      // Therefore, it is better to check both characters and erase any white space
      symbol.erase(std::remove(symbol.begin(), symbol.end(), ' '), symbol.end());
      symbol = strToValidSymbol(symbol);
      atom_amounts[symbol]++; // adds one to counter for this symbol

      // if a key leads to multiple z-values, set z-value to 0 (?)
      if (elem_Z.count(symbol) > 0){
        elem_Z[symbol] = 0;
      }
      // Stores the full list of atom coordinates from the input file
      raw_atom_coordinates.emplace_back(symbol,
                                        std::stod(line.substr(30,8)),
                                        std::stod(line.substr(38,8)),
                                        std::stod(line.substr(46,8)));
    }
  }
  // file has been read
  inp_file.close();
}

////////////////////////
// METHOD DEFINITIONS //
////////////////////////

// returns the radius of an atom with a given symbol
inline double Model::findRadiusOfAtom(const std::string& symbol){
  //TODO add exception handling for when no radius was found:
  //if(radius_map[symbol == 0]){
  //  throw ...;
  //}
  //else{ return...;}
  return radius_map[symbol];
}

inline double Model::findRadiusOfAtom(const Atom& at){
  return findRadiusOfAtom(at.symbol);
}

///////////////////
// AUX FUNCTIONS //
///////////////////

// split line into substrings when separated by whitespaces
static inline std::vector<std::string> splitLine(std::string& line){
  std::istringstream iss(line);
  std::vector<std::string> substrings((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());
  return substrings;
}

bool isAtomLine(const std::vector<std::string>& substrings) {
  if (substrings.size() == 4) {
// TODO: decide whether element symbol substring should be checked as starting with a letter
//  if (isalpha(substrings[0][0])){
    for (char i=1; i<4; i++){
      // using a try-block feels hacky, but the alternative using strtod does as well
      try {
        size_t str_pos = 0; // will contain the last position in the string that was successfully evaluated by std::stod()
        std::stod(substrings[i], &str_pos);
        if (substrings[i].size() != str_pos) {
          return false;
        }
      }
      catch (const std::invalid_argument& ia) {
        return false;
      }
    }
/* TODO bis
  }
  else {
  	return false;
  }*/
    return true;
  }
  return false;
}

// reads a string and converts it to valid atom symbol: first character uppercase followed by lowercase characters
std::string strToValidSymbol(std::string str){
  // iterate over all characters in string
  for (int i = 0; i<str.size(); i++) {
// TODO: decide whether non-alphabetic characters should be erased or not
//    if (isalpha(str[i])){
      // only for first character in sequence, convert to uppercase
      if (i==0) {
        str[i] = toupper(str[i]);
      }
      // for all other characters, convert to lowercase
      else {
        str[i] = tolower(str[i]);
      }
// TODO bis
//    }
//    else { // Remove number or charges from atoms so that "Pd2+" becomes "Pd"
//      str.erase(i, str.size());
//    }
  }
  return str;
}
