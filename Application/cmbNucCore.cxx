
#include "cmbNucCore.h"
#include "inpFileIO.h"

#include <iostream>
#include <algorithm>
#include <fstream>
#include <limits>
#include <set>

#include "cmbNucAssembly.h"

#include "vtkTransform.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkTransformFilter.h"
#include "vtkMath.h"
#include "cmbNucDefaults.h"
#include "cmbNucMaterialColors.h"

#include "vtkCmbLayeredConeSource.h"

#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QDebug>

void cmbNucCoreConnection::dataChanged()
{
  v->setAndTestDiffFromFiles(true);
  v->clearOldGeometry();
  emit dataChangedSig();
}

void cmbNucCoreConnection::clearData()
{
  v->clearOldGeometry();
}

void cmbNucCoreConnection::assemblyChanged()
{
  v->checkUsedAssembliesForGen();
  emit dataChangedSig();
}

const double cmbNucCore::CosSinAngles[6][2] =
{ { cos( 2*(vtkMath::Pi() / 6.0) * (0 + 3) ),
    sin( 2*(vtkMath::Pi() / 6.0) * (0 + 3) ) },
  { cos( 2*(vtkMath::Pi() / 6.0) * (1 + 3) ),
    sin( 2*(vtkMath::Pi() / 6.0) * (1 + 3) ) },
  { cos( 2*(vtkMath::Pi() / 6.0) * (2 + 3) ),
    sin( 2*(vtkMath::Pi() / 6.0) * (2 + 3) ) },
  { cos( 2*(vtkMath::Pi() / 6.0) * (3 + 3) ),
    sin( 2*(vtkMath::Pi() / 6.0) * (3 + 3) ) },
  { cos( 2*(vtkMath::Pi() / 6.0) * (4 + 3) ),
    sin( 2*(vtkMath::Pi() / 6.0) * (4 + 3) ) },
  { cos( 2*(vtkMath::Pi() / 6.0) * (5 + 3) ),
    sin( 2*(vtkMath::Pi() / 6.0) * (5 + 3) ) } };

cmbNucCore::cmbNucCore(bool needSaved)
{
  clearOldGeometry();
  this->AssyemblyPitchX = this->AssyemblyPitchY = 23.5;
  this->HexSymmetry = 1;
  DifferentFromFile = needSaved;
  DifferentFromH5M = true;
  this->Connection = new cmbNucCoreConnection();
  this->Connection->v = this;
  hasCylinder = false;
  cylinderRadius = 0;
  cylinderOuterSpacing = 0;
}

cmbNucCore::~cmbNucCore()
{
  for(std::vector<cmbNucAssembly*>::iterator fit=this->Assemblies.begin();
    fit!=this->Assemblies.end(); ++fit)
    {
    if(*fit)
      {
      delete *fit;
      }
    }
  this->Assemblies.clear();
  delete this->Defaults;
  delete this->Connection;
}

void cmbNucCore::clearOldGeometry()
{
  this->Data = NULL;
}

void cmbNucCore::SetDimensions(int i, int j)
{
  this->lattice.SetDimensions(i, j);
}

void cmbNucCore::clearExceptAssembliesAndGeom()
{
  clearOldGeometry();
  this->lattice.SetDimensions(1, 1, true);
  this->setAndTestDiffFromFiles(true);
  FileName = "";
  h5mFile = "";
  Params.clear();
}

void cmbNucCore::AddAssembly(cmbNucAssembly *assembly)
{
    if(this->Assemblies.size()==0)
    {
    if(assembly)
      {
      this->lattice.SetGeometryType(
        assembly->getLattice().GetGeometryType());
      }
    this->SetDimensions(1, 1);
    }
  this->Assemblies.push_back(assembly);
  QObject::connect(assembly->GetConnection(), SIGNAL(dataChangedSig()),
                   this->Connection, SIGNAL(dataChangedSig()));
  QObject::connect(assembly->GetConnection(), SIGNAL(colorChanged()),
                   this->Connection, SIGNAL(colorChanged()));
  QObject::connect(assembly->GetConnection(), SIGNAL(dataChangedSig()),
                   this->Connection, SLOT(assemblyChanged()));
  QObject::connect(this->Connection, SIGNAL(dataChangedSig()),
                   this->Connection, SLOT(clearData()));
  if(this->Assemblies.size() == 1)
    {
    this->SetAssemblyLabel(0, 0, assembly->label, assembly->GetLegendColor());
    }
  // the new assembly need to be in the grid
}

void cmbNucCore::RemoveAssembly(const std::string &label)
{
  for(size_t i = 0; i < this->Assemblies.size(); i++)
    {
    if(this->Assemblies[i]->label == label)
      {
      delete this->Assemblies[i];
      this->Assemblies.erase(this->Assemblies.begin() + i);
      break;
      }
    }
  // update the Grid
  if(this->lattice.ClearCell(label))
  {
    this->setAndTestDiffFromFiles(true);
    clearOldGeometry();
  }
}

cmbNucAssembly* cmbNucCore::GetAssembly(const std::string &label)
{
  for(size_t i = 0; i < this->Assemblies.size(); i++)
    {
    if(this->Assemblies[i]->label == label)
      {
      return this->Assemblies[i];
      }
    }

  return NULL;
}

cmbNucAssembly* cmbNucCore::GetAssembly(int idx)
{
  return idx<this->Assemblies.size() ? this->Assemblies[idx] : NULL;
}

std::vector< cmbNucAssembly* > cmbNucCore::GetUsedAssemblies()
{
  std::set<std::string> usedDict;
  for(size_t i = 0; i < this->lattice.Grid.size(); i++)
    {
    for(size_t j = 0; j < this->lattice.Grid[i].size(); j++)
      {
      usedDict.insert(this->lattice.GetCell(i, j).label);
      }
    }
  std::vector< cmbNucAssembly* > result;
  for (unsigned int i = 0; i < this->Assemblies.size(); ++i)
    {
    if(this->Assemblies[i]!=NULL &&
       usedDict.find(this->Assemblies[i]->label) != usedDict.end())
      {
      result.push_back(Assemblies[i]);
      }
    }
  return result;
}

vtkSmartPointer<vtkMultiBlockDataSet> cmbNucCore::GetData()
{
  if(this->Data == NULL)
  {
    generateData();
  }
  return this->Data;
}

void cmbNucCore::generateData()
{
  if(this->Assemblies.size()==0 || this->lattice.Grid.size()==0
    || this->lattice.Grid[0].size()==0)
    {
    clearOldGeometry(); return;
    }
  // we need at least one duct
  if(Assemblies[0]->AssyDuct.numberOfDucts()==0)
    {
    clearOldGeometry(); return;
    }
  this->Data = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  int subType = this->lattice.GetGeometrySubType();

  double startX = this->Assemblies[0]->AssyDuct.getDuct(0)->x;
  double startY = this->Assemblies[0]->AssyDuct.getDuct(0)->y;
  double outerDuctWidth = this->Assemblies[0]->AssyDuct.getDuct(0)->thickness[1];
  double outerDuctHeight = this->Assemblies[0]->AssyDuct.getDuct(0)->thickness[0];
  double extraXTrans = 0, extraYTrans = 0;

  // Is this Hex type?
  bool isHex = Assemblies[0]->getLattice().GetGeometryType() == HEXAGONAL;


  // setup data
  size_t numBlocks = isHex ?
    (1 + 3*(int)this->lattice.Grid.size()*((int)this->lattice.Grid.size() - 1)) :
    this->lattice.Grid.size()*this->lattice.Grid[0].size();

  this->Data->SetNumberOfBlocks(numBlocks);

  if( isHex && (subType & ANGLE_360) && this->lattice.Grid.size()>=1 )
  {
    double odc = outerDuctHeight*this->lattice.Grid.size();
    double tmp = odc - outerDuctHeight;
    double t2 = tmp*0.5;
    double ty = std::sqrt(tmp*tmp-t2*t2);
    extraYTrans = -ty;
    extraXTrans = odc;
  }

  for(size_t i = 0; i < this->lattice.Grid.size(); i++)
    {
    size_t startBlock = isHex ?
      (i==0 ? 0 : (1 + 3*i*(i-1))) : (i*this->lattice.Grid[0].size());

    const std::vector<Lattice::LatticeCell> &row = this->lattice.Grid[i];

    for(size_t j = 0; j < row.size(); j++)
      {
      const std::string &type = row[j].label;

      if(!type.empty() && type != "xx" && type != "XX")
        {
        cmbNucAssembly* assembly = this->GetAssembly(type);
        if(!assembly)
          {
          continue;
          }
        vtkSmartPointer<vtkMultiBlockDataSet> assemblyData;
        if( isHex && subType & VERTEX)
        {
          vtkSmartPointer<vtkTransform> transform2 = vtkSmartPointer<vtkTransform>::New();
          assemblyData = vtkSmartPointer<vtkMultiBlockDataSet>::New();
          transform2->RotateZ(30);
          transformData(assembly->GetData(), assemblyData, transform2);
        }
        else
        {
          assemblyData = assembly->GetData();
        }
        vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();

        if(isHex)
          {
          Duct *hexDuct = assembly->AssyDuct.getDuct(0);
          double layerCorners[6][2], hexDiameter, layerRadius;
          hexDiameter = hexDuct->thickness[0];

          // For hex geometry type, figure out the six corners first
          if(i>0)
            {
            layerRadius = hexDiameter * i;
            for(int c = 0; c < 6; c++)
              {
              // Needs a little more thinking on math here?
              layerCorners[c][0] = layerRadius * CosSinAngles[c][0];
              layerCorners[c][1] = layerRadius * CosSinAngles[c][1];
              }
            }

          double tX=startX, tY=startY;
          int cornerIdx;
          if(i == 1)
            {
            cornerIdx = j%6;
            tX += layerCorners[cornerIdx][0];
            tY += layerCorners[cornerIdx][1];
            }
          else if( i > 1)
            {
            cornerIdx = j / i;
            int idxOnEdge = j%i;
            if(idxOnEdge == 0) // one of the corners
              {
              tX += layerCorners[cornerIdx][0];
              tY += layerCorners[cornerIdx][1];
              }
            else
              {
              // for each layer, we should have (numLayers-2) middle hexes
              // between the corners
              double deltx, delty, numSegs = i, centerPos[2];
              int idxNext = cornerIdx==5 ? 0 : cornerIdx+1;
              deltx = (layerCorners[idxNext][0] - layerCorners[cornerIdx][0]) / numSegs;
              delty = (layerCorners[idxNext][1] - layerCorners[cornerIdx][1]) / numSegs;
              centerPos[0] = layerCorners[cornerIdx][0] + deltx * (idxOnEdge);
              centerPos[1] = layerCorners[cornerIdx][1] + delty * (idxOnEdge);
              tX += centerPos[0];
              tY += centerPos[1];
              }
            }
          if((subType & ANGLE_60) && (subType & VERTEX))
            {
            transform->RotateZ(-90);
            }
          else
            {
            transform->RotateZ(-60);
            }
          transform->Translate(tX, -tY, 0.0);
          }
        else
          {
          double tX = startX + i * (outerDuctWidth);
          double tY = startY + j * (outerDuctHeight);
          transform->Translate(tY, tX-outerDuctWidth*(this->lattice.Grid.size()-1), 0);
          }
        // transform block by block --- got to have better ways
        vtkSmartPointer<vtkMultiBlockDataSet> blockData = vtkSmartPointer<vtkMultiBlockDataSet>::New();
        blockData->SetNumberOfBlocks(assemblyData->GetNumberOfBlocks());
        // move the assembly to the correct position
        transformData(assemblyData, blockData, transform);
        if(extraXTrans != 0 || extraYTrans != 0)
          {
          vtkSmartPointer<vtkTransform> transform2 = vtkSmartPointer<vtkTransform>::New();
          vtkSmartPointer<vtkMultiBlockDataSet> blockData2 = vtkSmartPointer<vtkMultiBlockDataSet>::New();
          blockData2->SetNumberOfBlocks(blockData->GetNumberOfBlocks());
          transform2->Translate(extraXTrans, extraYTrans, 0.0);
          transformData(blockData, blockData2, transform2);
          this->Data->SetBlock(startBlock+j, blockData2.GetPointer());
          }
        else
          {
          this->Data->SetBlock(startBlock+j, blockData.GetPointer());
          }


        vtkInformation* info = this->Data->GetMetaData(startBlock+j);
        info->Set(vtkCompositeDataSet::NAME(), assembly->label.c_str());
        }
      else
        {
        this->Data->SetBlock(startBlock+j, NULL);
        }
      }
    }
  this->Data->SetBlock(this->Data->GetNumberOfBlocks()+1, NULL);
  this->drawCylinder();
}

void cmbNucCore::transformData(vtkMultiBlockDataSet * input,
                               vtkMultiBlockDataSet * output,
                               vtkTransform * xmform)
{
  if(input == NULL) return;
  output->SetNumberOfBlocks(input->GetNumberOfBlocks());
  // move the assembly to the correct position
  for(int idx=0; idx<input->GetNumberOfBlocks(); idx++)
  {
    // Brutal. I wish the SetDefaultExecutivePrototype had workd :(
    if(vtkDataObject* objBlock = input->GetBlock(idx))
    {
      if(vtkMultiBlockDataSet* assyPartBlock =
         vtkMultiBlockDataSet::SafeDownCast(objBlock))
      {
        vtkSmartPointer<vtkMultiBlockDataSet> assyPartObjs = vtkSmartPointer<vtkMultiBlockDataSet>::New();
        assyPartObjs->SetNumberOfBlocks(assyPartBlock->GetNumberOfBlocks());
        transformData(assyPartBlock, assyPartObjs, xmform);
        output->SetBlock(idx, assyPartObjs.GetPointer());
      }
      else
      {
        vtkNew<vtkTransformFilter> filter;
        filter->SetTransform(xmform);
        filter->SetInputDataObject(input->GetBlock(idx));
        filter->Update();
        output->SetBlock(idx, filter->GetOutput());
      }
    }
    else
    {
      output->SetBlock(idx, NULL);
    }
  }
}

void cmbNucCore::setGeometryLabel(std::string geomType)
{
  enumGeometryType type = lattice.GetGeometryType();
  int subType = lattice.GetGeometrySubType() & JUST_ANGLE;
  std::transform(geomType.begin(), geomType.end(), geomType.begin(), ::tolower);
  type = HEXAGONAL;
  if( geomType == "hexflat" )
  {
    subType |= FLAT;
  }
  else if(geomType == "hexvertex")
  {
    subType |= VERTEX;
  }
  else
  {
    type = RECTILINEAR;
    subType = ANGLE_360;
  }
  lattice.SetGeometryType(type);
  lattice.SetGeometrySubType(subType);
}

void cmbNucCore::setHexSymmetry(int sym)
{
  int type = lattice.GetGeometrySubType() & ~JUST_ANGLE; // clear angle
  if(sym == 6)
  {
    type |= ANGLE_60;
  }
  else if(sym == 12)
  {
    type |= ANGLE_30;
  }
  else //all others treated as 360
  {
    type |= ANGLE_360;
  }
  lattice.SetGeometrySubType(type);
}

bool cmbNucCore::IsHexType()
{
  return HEXAGONAL == lattice.GetGeometryType();
}

void cmbNucCore::SetLegendColorToAssemblies(int numDefaultColors, int defaultColors[][3])
{
  for(unsigned int i = 0; i < this->Assemblies.size(); ++i)
    {
      cmbNucAssembly * subAssembly = this->Assemblies[i];
      if (subAssembly)
        {
        int acolorIndex = i  % numDefaultColors;
        QColor acolor(defaultColors[acolorIndex][0],
                      defaultColors[acolorIndex][1],
                      defaultColors[acolorIndex][2]);
        subAssembly->SetLegendColor(acolor);
        }
    }
  this->RebuildGrid();
}

cmbNucAssembly* cmbNucCore::loadAssemblyFromFile(
  const std::string &fileName, const std::string &assyLabel)
{
  // read file and create new assembly
  cmbNucAssembly* assembly = new cmbNucAssembly;
  assembly->label = assyLabel;
  assembly->ReadFile(fileName);
  this->AddAssembly(assembly);
  return assembly;
}

void cmbNucCore::RebuildGrid()
{
  for(size_t i = 0; i < this->lattice.Grid.size(); i++)
    {
    for(size_t j = 0; j < this->lattice.Grid[i].size(); j++)
      {
      std::string type = this->lattice.Grid[i][j].label;
      cmbNucAssembly* assembly = NULL;
      if(!(type.empty() || type == "xx" || type == "XX" ||
          (assembly = this->GetAssembly(type)) == NULL))
        {
        this->lattice.Grid[i][j].color = assembly->GetLegendColor();
        }
      else
        {
        this->lattice.Grid[i][j].color = Qt::white;
        }
      }
    }
}

void cmbNucCore::computePitch()
{
  std::vector< cmbNucAssembly* > assemblies = this->GetUsedAssemblies();
  AssyemblyPitchX = 0;
  AssyemblyPitchY = 0;
  double tmp[2];
  for (unsigned int i = 0; i < assemblies.size(); ++i)
  {
    if(assemblies[i]==NULL) continue;
    assemblies[i]->GetDuctWidthHeight(tmp);
    if(tmp[0]>AssyemblyPitchX) AssyemblyPitchX = tmp[0];
    if(tmp[1]>AssyemblyPitchY) AssyemblyPitchY = tmp[1];
  }
}

int cmbNucCore::GetNumberOfAssemblies() const
{
  return static_cast<int>(this->Assemblies.size());
}

void cmbNucCore::setAndTestDiffFromFiles(bool diffFromFile)
{
  if(diffFromFile)
  {
    this->DifferentFromFile = true;
    this->DifferentFromH5M = true;
    return;
  }
  //make sure file exits
  //check to see if a h5m file has been generate and is older than this file
  QFileInfo inpInfo(this->FileName.c_str());
  if(!inpInfo.exists())
  {
    this->DifferentFromFile = true;
    this->DifferentFromH5M = true;
    return;
  }
  this->DifferentFromFile = false;
  //QFileInfo h5mFI();
  QDateTime inpLM = inpInfo.lastModified();
  QFileInfo h5mInfo(inpInfo.dir(), h5mFile.c_str());
  if(!h5mInfo.exists())
  {
    this->DifferentFromH5M = true;
    return;
  }
  QDateTime h5mLM = h5mInfo.lastModified();
  this->DifferentFromH5M = h5mLM < inpLM;
  this->checkUsedAssembliesForGen();
}

void cmbNucCore::checkUsedAssembliesForGen()
{
  if(this->DifferentFromH5M) return;
  QFileInfo h5mInfo(QFileInfo(this->FileName.c_str()).dir(), this->h5mFile.c_str());
  std::vector< cmbNucAssembly* > assy = this->GetUsedAssemblies();
  for(unsigned int i = 0; i < assy.size() && !this->DifferentFromH5M; ++i)
  {
    this->DifferentFromH5M |= assy[i]->changeSinceLastGenerate();
    QFileInfo inpInfo(assy[i]->FileName.c_str());
    QFileInfo cubInfo(inpInfo.dir(), inpInfo.baseName() + ".cub");
    this->DifferentFromH5M |= !cubInfo.exists() || h5mInfo.lastModified() < cubInfo.lastModified();
  }
}

bool cmbNucCore::changeSinceLastSave() const
{
  return this->DifferentFromFile;
}

bool cmbNucCore::changeSinceLastGenerate() const
{
  return this->DifferentFromH5M;
}

QPointer<cmbNucDefaults> cmbNucCore::GetDefaults()
{
  return this->Defaults;
}

bool cmbNucCore::HasDefaults() const
{
  return this->Defaults != NULL;
}

void cmbNucCore::initDefaults()
{
  double DuctThickX = 10;
  double DuctThickY = 10;
  double length = 10;
  delete this->Defaults;
  this->Defaults = new cmbNucDefaults();
  this->Defaults->setHeight(length);

  this->Defaults->setDuctThickness(DuctThickX, DuctThickY);
}

void cmbNucCore::calculateDefaults()
{
  delete this->Defaults;
  this->Defaults = new cmbNucDefaults();
  std::vector< cmbNucAssembly* > assys = this->GetUsedAssemblies();

  double RadialMeshSize = 1e23;
  double AxialMeshSize = 1e23;
  int    EdgeInterval = 2147483647;

  double DuctThickX = -1;
  double DuctThickY = -1;
  double length = -1;
  QString MeshType;

  for(unsigned int i = 0; i < assys.size(); ++i)
  {
    cmbNucAssembly * assy = assys[i];
    cmbAssyParameters* params =  assy->GetParameters();
    if(params->isValueSet(params->AxialMeshSize) &&
       params->AxialMeshSize < AxialMeshSize)
      AxialMeshSize = params->AxialMeshSize;
    if(params->isValueSet(params->EdgeInterval) &&
       params->EdgeInterval < EdgeInterval)
      EdgeInterval = params->EdgeInterval;
    if(MeshType.isEmpty() && params->isValueSet(params->MeshType))
    {
      MeshType = params->MeshType.c_str();
    }
    if(length < assy->AssyDuct.getLength()) length = assy->AssyDuct.getLength();
    double r[2];
    assy->GetDuctWidthHeight(r);
    if( r[0] > DuctThickX ) DuctThickX = r[0];
    if( r[1] > DuctThickY ) DuctThickY = r[1];
  }
  if(AxialMeshSize != 1e23)
    this->Defaults->setAxialMeshSize(AxialMeshSize);
  if(EdgeInterval != 2147483647)
    this->Defaults->setEdgeInterval(EdgeInterval);
  if(!MeshType.isEmpty()) this->Defaults->setMeshType(MeshType);
  if(length != -1) this->Defaults->setHeight(length);

  this->Defaults->setDuctThickness(DuctThickX, DuctThickY);
  this->sendDefaults();
}

void cmbNucCore::sendDefaults()
{
  std::vector< cmbNucAssembly* > assys = this->Assemblies;
  for(unsigned int i = 0; i < assys.size(); ++i)
  {
    assys[i]->setFromDefaults(this->Defaults);
  }
}

bool cmbNucCore::label_unique(std::string & n)
{
  std::vector< cmbNucAssembly* > assys = this->Assemblies;
  for(unsigned int i = 0; i < assys.size(); ++i)
  {
    if(assys[i]->getLabel() == n) return false;
  }
  return true;
}

void cmbNucCore::fillList(QStringList & l)
{
  for(int i = 0; i < this->GetNumberOfAssemblies(); i++)
  {
    cmbNucAssembly *t = this->GetAssembly(i);
    l.append(t->label.c_str());
  }
}

AssyPartObj * cmbNucCore::getFromLabel(const std::string & s)
{
  return this->GetAssembly(s);
}

int start = 0;

void cmbNucCore::drawCylinder()
{
  if(cylinderRadius <= 0 || cylinderOuterSpacing <= 0)
  {
    return;
  }
  if(!hasCylinder) return;

  vtkSmartPointer<vtkCmbLayeredConeSource> cylinder = vtkSmartPointer<vtkCmbLayeredConeSource>::New();
  //cylinder->SetGenerateEnds(0);

  double outerDuctWidth = this->Assemblies[0]->AssyDuct.getDuct(0)->thickness[1];
  double outerDuctHeight = this->Assemblies[0]->AssyDuct.getDuct(0)->thickness[0];
  double extraXTrans = 0, extraYTrans = 0;

  // setup data
  if(IsHexType())
  {
    double odc = outerDuctHeight*this->lattice.Grid.size();
    double tmp = odc - outerDuctHeight;
    double t2 = tmp*0.5;
    double ty = std::sqrt(tmp*tmp-t2*t2);
    extraYTrans = -ty;
    extraXTrans = odc;
  }
  else
  {
    double tx = outerDuctWidth*(this->lattice.Grid.size()-1)*0.5;
    double ty = outerDuctHeight*(this->lattice.Grid[0].size()-1)*0.5;
    extraXTrans = ty,
    extraYTrans = tx-outerDuctWidth*(this->lattice.Grid.size()-1);
  }

  double h;
  Defaults->getHeight(h);
  cylinder->SetHeight(h);
  cylinder->SetNumberOfLayers(1);
  cylinder->SetTopRadius(0, cylinderRadius);
  cylinder->SetBaseRadius(0, cylinderRadius);
  cylinder->SetResolution(0, cylinderOuterSpacing);
  double odc = outerDuctHeight*(this->lattice.Grid.size()-1);
  double tmp = (outerDuctHeight*0.5) / (cos(30.0 * vtkMath::Pi() / 180.0));
  double pt1[] = {odc * CosSinAngles[0][0], odc * CosSinAngles[0][1]};
  double pt2[2];
  if(IsHexType())
  {
    for(int i = 0; i < 6; ++i)
    {
      pt2[0] = odc * CosSinAngles[(i+1)%6][0];
      pt2[1] = odc * CosSinAngles[(i+1)%6][1];
      int sp1 = (i + 0)%6;
      int sp2 = (i + 1)%6;
      if(this->lattice.Grid.size() == 1)
      {
        cylinder->addInnerPoint(pt1[0]+tmp * cmbNucAssembly::CosSinAngles[sp1][0],
                                pt1[1]+tmp * cmbNucAssembly::CosSinAngles[sp1][1]);
      }
      else for(int j = 0; j < this->lattice.Grid.size(); ++j)
      {
        double s = j/(this->lattice.Grid.size()-1.0);
        double pt[] = {pt1[0]*(1.0-s)+pt2[0]*(s), pt1[1]*(1.0-s)+pt2[1]*(s)};

        {
          cylinder->addInnerPoint(pt[0]+tmp * cmbNucAssembly::CosSinAngles[sp1][0],
                                  pt[1]+tmp * cmbNucAssembly::CosSinAngles[sp1][1]);
        }
        if(j != this->lattice.Grid.size()-1 )
        {
          cylinder->addInnerPoint(pt[0]+tmp * cmbNucAssembly::CosSinAngles[sp2][0],
                                pt[1]+tmp * cmbNucAssembly::CosSinAngles[sp2][1]);
        }
      }

      pt1[0] = pt2[0];
      pt1[1] = pt2[1];
    }
  }
  else
  {
    double height = outerDuctWidth*(this->lattice.Grid.size())*0.5;
    double width = outerDuctHeight*(this->lattice.Grid[0].size())*0.5;

    cylinder->addInnerPoint( width,-height);
    cylinder->addInnerPoint( width, height);
    cylinder->addInnerPoint(-width, height);
    cylinder->addInnerPoint(-width,-height);

  }
  double bc[] = {extraXTrans, extraYTrans, 0};
  cylinder->SetBaseCenter(bc);
  cylinder->Update();

  Data->SetBlock(Data->GetNumberOfBlocks()-1, cylinder->GetOutput());
}

void cmbNucCore::drawCylinder(double r, int i)
{
  if(r <= 0 || i <= 0)
  {
    return;
  }
  if(!hasCylinder)
  {
    cmbNucMaterialColors::instance()->getUnknownMaterial()->inc();
  }
  hasCylinder = true;
  cylinderRadius = r;
  cylinderOuterSpacing = i;
  drawCylinder();
}

void cmbNucCore::clearCylinder()
{
  if(hasCylinder)
  {
    cmbNucMaterialColors::instance()->getUnknownMaterial()->dec();
  }
  hasCylinder = false;
  if(Data != NULL)
  {
    Data->SetBlock(Data->GetNumberOfBlocks()-1, NULL);
  }
}
