#ifndef inpFileIO_h
#define inpFileIO_h

#include <string>
#include <vector>
#include <set>
#include <map>
#include <sstream>
#include <iostream>
#include <fstream>

#include "cmbNucPartDefinition.h"

class cmbNucAssembly;
class cmbNucCore;
class cmbNucDefaults;

class inpFileReader
{
public:
  inpFileReader();
  enum FileType{ERROR_TYPE, UNKNOWN_TYPE, ASSEMBLY_TYPE, CORE_TYPE};
  FileType open(std::string fname);
  void close();
  bool read(cmbNucAssembly & assembly);
  bool read(cmbNucCore & core, bool read_assemblies = true);
  bool read_defaults(cmbNucDefaults & defaults);
  bool read_defaults(cmbNucAssembly & assembly);
protected:
  std::string FileName;
  FileType Type;
  std::string CleanFile;
private:
};

class inpFileWriter
{
public:
  static bool write(std::string fname, cmbNucAssembly & assembly, bool updateFname = true, bool limited = false);
  static bool write(std::string fname, cmbNucCore & core, bool updateFname = true);
  static bool writeGSH(std::string fname, cmbNucCore & core, std::string assyName);
private:
};


#endif
