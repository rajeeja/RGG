#ifndef cmbNucCore_H
#define cmbNucCore_H

#include <string>
#include <vector>
#include <QColor>

#include "vtkMultiBlockDataSet.h"
#include "cmbNucPartDefinition.h"
#include "vtkSmartPointer.h"

class cmbNucAssembly;

// Represents the core which is composed of multiple assemblies
// (cmbNucAssembly). Assemblies are layed out on a lattice.
class cmbNucCore : public AssyPartObj
{
public:

  // Creates an empty Core.
  cmbNucCore();

  // Destroys the Core.
  ~cmbNucCore();

  virtual enumNucPartsType GetType() {return CMBNUC_CORE;}

  // Adds a new Assembly to the core. After adding the Assembly it
  // can be placed in the Core with the SetAssembly() method.
  void AddAssembly(cmbNucAssembly *assembly);

  // Remove the Assembly with label from the Core.
  void RemoveAssembly(const std::string &label);

  // Returns the Assembly with label or index.
  // Returns 0 if no Assembly with label or index exists.
  cmbNucAssembly* GetAssembly(const std::string &label);
  cmbNucAssembly* GetAssembly(int idx);

  // Return the number of assemblies in the core
  int GetNumberOfAssemblies();

  // Sets the dimensions of the Assembly Core.
  void SetDimensions(int i, int j);
  // Returns the dimensions of the Assembly Core.
  std::pair<int, int> GetDimensions() const;

  // Sets the contents of the Assembly (i, j) to name.
  void SetAssemblyLabel(int i, int j, const std::string &name, const QColor& color);
  // Returns the contents of the Assembly (i, j).
  LatticeCell GetAssemblyLabel(int i, int j) const;
  // Clears the contents of the Assembly (i, j). This is equivalent
  // to calling SetAssembly(i, j, "xx", Qt::white).
  void ClearAssemblyLabel(int i, int j);

  // Rebuild the grid (which for now just updates the colors at each cell)
  void RebuildGrid();

  // Returns a multi-block data set containing the geometry for
  // the core with assemblies. This is used to render the core in 3D.
  vtkSmartPointer<vtkMultiBlockDataSet> GetData();

  // Get/Set Assembly pitch
  double AssyemblyPitchX;
  double AssyemblyPitchY;
  std::vector<std::vector<LatticeCell> > Grid;

private:
  vtkSmartPointer<vtkMultiBlockDataSet> Data;
  std::vector<cmbNucAssembly*> Assemblies;

};

#endif // cmbNucCore_H
