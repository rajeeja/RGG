
#include "cmbNucAssembly.h"
#include "inpFileIO.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <limits>

#include "vtkCmbDuctSource.h"
#include "cmbNucMaterialColors.h"
#include "vtkCmbConeSource.h"
#include "vtkCmbLayeredConeSource.h"
#include <vtkClipClosedSurface.h>
#include <vtkPlaneCollection.h>

#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkPolyDataNormals.h"
#include "vtkNew.h"
#include "vtkMath.h"

#include "vtkXMLMultiBlockDataWriter.h"
#include <QMap>
#include <QDebug>

cmbNucAssembly::cmbNucAssembly()
{
  this->Data = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  this->LegendColor = Qt::white;
  this->Parameters = new cmbAssyParameters;
}

cmbNucAssembly::~cmbNucAssembly()
{
  AssyPartObj::deleteObjs(this->PinCells);
  delete this->Parameters;
}

QColor cmbNucAssembly::GetLegendColor() const
{
  return this->LegendColor;
}

void cmbNucAssembly::SetLegendColor(const QColor& color)
{
  this->LegendColor = color;
}

void cmbNucAssembly::UpdateGrid()
{
  std::pair<int, int> dim = this->AssyLattice.GetDimensions();
  for(size_t i = 0; i < dim.first; i++)
    {
    size_t layerCells = this->AssyLattice.GetGeometryType() == HEXAGONAL ?
      6*i : dim.second;
    for(size_t j = 0; j < layerCells; j++)
      {
      std::string label = this->AssyLattice.GetCell(i, j).label;
      PinCell* pc = this->GetPinCell(label);
      if(pc)
        {
        this->AssyLattice.SetCell(i, j, label, pc->GetLegendColor());
        }
      else
        {
        this->AssyLattice.ClearCell(i, j);
        }
      }
    }
}

void cmbNucAssembly::AddPinCell(PinCell *pincell)
{
  this->PinCells.push_back(pincell);
}

void cmbNucAssembly::RemovePinCell(const std::string &label)
{
  for(size_t i = 0; i < this->PinCells.size(); i++)
    {
    if(this->PinCells[i]->label == label)
      {
      delete this->PinCells[i];
      this->PinCells.erase(this->PinCells.begin() + i);
      break;
      }
    }
  // update the Grid
  this->AssyLattice.ClearCell(label);
}

void cmbNucAssembly::RemoveMaterial(const std::string &name)
{
  // update all places that references materials: ducts, pins
   for(size_t i = 0; i < this->AssyDuct.Ducts.size(); i++)
    {
    Duct *duct = this->AssyDuct.Ducts[i];
    for(size_t j = 0; j < duct->materials.size(); j++)
      {
      if(duct->materials[j] == name)
        {
        duct->materials[j] = "";
        }
     }
    }
  for(size_t i = 0; i < this->PinCells.size(); i++)
    {
    PinCell* pincell = this->PinCells[i];
    pincell->RemoveMaterial(name);
    }
}

PinCell* cmbNucAssembly::GetPinCell(const std::string &label)
{
  for(size_t i = 0; i < this->PinCells.size(); i++)
    {
    if(this->PinCells[i]->label == label)
      {
      return this->PinCells[i];
      }
    }

  return 0;
}

bool cmbNucAssembly::IsHexType()
{
  std::string strGeoType = this->GeometryType;
  std::transform(this->GeometryType.begin(), this->GeometryType.end(),
    strGeoType.begin(), ::tolower);
  return strGeoType == "hexagonal";
}

void cmbNucAssembly::ReadFile(const std::string &FileName)
{
  inpFileReader freader;
  if(!freader.open(FileName))
  {
    return;
  }
  freader.read(*this);
}

#define WRITE_PARAM_VALUE(X)\
if(this->Parameters->isValueSet(this->Parameters->X))\
   output << #X << " " << this->Parameters->X << std::endl

void cmbNucAssembly::WriteFile(const std::string &fname)
{
  qDebug() << "Writing: " << fname.c_str();
  inpFileWriter::write(fname, *this, true);
}

void cmbNucAssembly::updateMaterialColors(
  unsigned int& realflatidx,
  vtkCompositeDataDisplayAttributes *attributes)
{
// count number of pin blocks in the data set
int pins = this->AssyLattice.GetNumberOfCells();
int pin_count = 0;
int ducts_count = 0;
cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();

std::string pinMaterial = "pin";
int numAssyBlocks = this->Data->GetNumberOfBlocks();
for(unsigned int idx = 0; idx < numAssyBlocks; idx++)
  {
  realflatidx++;
  if(pin_count < pins)
    {
    std::string label = this->AssyLattice.GetCell(pin_count).label;
    PinCell* pinCell = this->GetPinCell(label);

    if(pinCell)
      {
      std::string pinMaterial;

      for(unsigned int cidx = 0; cidx < pinCell->cylinders.size(); cidx++)
        {
        realflatidx++; // increase one for this cylinder
        for(int k = 0; k < pinCell->GetNumberOfLayers(); k++)
          {
          pinMaterial = pinCell->cylinders[cidx]->materials[k];
          matColorMap->SetBlockMaterialColor(attributes, ++realflatidx, pinMaterial);
          }
        }
      for(unsigned int fidx = 0; fidx < pinCell->frustums.size(); fidx++)
        {
        realflatidx++; // increase one for this frustum
        for(int k = 0; k < pinCell->GetNumberOfLayers(); k++)
          {
          pinMaterial = pinCell->frustums[fidx]->materials[k];
          matColorMap->SetBlockMaterialColor(attributes, ++realflatidx, pinMaterial);
          }
        }
      }
    pin_count++;
    }
  else // ducts need to color by layers
    {
    if(vtkMultiBlockDataSet* ductBlock =
      vtkMultiBlockDataSet::SafeDownCast(this->Data->GetBlock(idx)))
      {
      Duct* duct = this->AssyDuct.Ducts[ducts_count];
      unsigned int numBlocks = ductBlock->GetNumberOfBlocks();
      for(unsigned int b = 0; b < numBlocks; b++)
        {
        std::string layerMaterial =
          (duct && b < duct->materials.size()) ? duct->materials[b] : "duct";
        if(layerMaterial.empty())
          {
          layerMaterial = "duct";
          }
        layerMaterial = QString(layerMaterial.c_str()).toLower().toStdString();
        matColorMap->SetBlockMaterialColor(attributes, ++realflatidx, layerMaterial);
        }
      ducts_count++;
      }
    }
  }
}

vtkSmartPointer<vtkMultiBlockDataSet> cmbNucAssembly::GetData()
{
  if(this->Data->GetNumberOfBlocks() > 0)
    {
    return this->Data;
    }
  return this->CreateData();
}

vtkSmartPointer<vtkMultiBlockDataSet> cmbNucAssembly::CreateData()
{
  if(this->AssyDuct.Ducts.size()==0 || this->AssyLattice.Grid.size() == 0)
    {
    return NULL;
    }
  double outerDuctHeight = this->AssyDuct.Ducts[0]->thicknesses.back();
  double innerDuctHeight = this->AssyDuct.Ducts[0]->thicknesses.front();
  double chamberStart = outerDuctHeight - innerDuctHeight;
  double chamberEnd = innerDuctHeight;

  // setup data
  this->Data->SetNumberOfBlocks(this->AssyLattice.GetNumberOfCells() +
                                this->AssyDuct.Ducts.size());

  // For Hex type
  Duct *hexDuct = this->AssyDuct.Ducts[0];
  double layerCorners[6][2], hexRadius, hexDiameter, layerRadius;
  hexDiameter = hexDuct->thicknesses[0];
  hexRadius = hexDiameter / (double)(2 * cos(30.0 * vtkMath::Pi() / 180.0));
  hexRadius = hexRadius / (double)(2*this->AssyLattice.Grid.size()-1);

  double cellLength = (chamberEnd - chamberStart) / this->AssyLattice.Grid.size();

  for(size_t i = 0; i < this->AssyLattice.Grid.size(); i++)
    {
    // For hex geometry type, figure out the six corners first
    if(this->AssyLattice.GetGeometryType() == HEXAGONAL && i>0)
      {
      for(int c = 0; c < 6; c++)
        {
        double angle = 2 * (vtkMath::Pi() / 6.0) * (c + 3.5);
        layerCorners[c][0] = cos(angle);
        layerCorners[c][1] = sin(angle);
        }
      }

    size_t startBlock = this->AssyLattice.GetGeometryType() == HEXAGONAL ?
      (i==0 ? 0 : (1 + 3*i*(i-1))) : (i*this->AssyLattice.Grid.size());
    const std::vector<LatticeCell> &row = this->AssyLattice.Grid[i];
    for(size_t j = 0; j < row.size(); j++)
      {
      const std::string &type = row[j].label;

      if(!type.empty() && type != "xx" && type != "XX")
        {
        PinCell* pincell = this->GetPinCell(type);
        if(pincell && (pincell->cylinders.size()+pincell->frustums.size())>0)
          {
          // create polydata for the pincell
          if(!pincell->CachedData)
            {
            pincell->CachedData.TakeReference(
              cmbNucAssembly::CreatePinCellMultiBlock(pincell));
            }
          vtkMultiBlockDataSet *dataSet = pincell->CachedData;
          vtkNew<vtkMultiBlockDataSet> pinDataSet;
          pinDataSet->SetNumberOfBlocks(dataSet->GetNumberOfBlocks());
          for(int block=0; block<dataSet->GetNumberOfBlocks(); block++)
            {
            vtkMultiBlockDataSet* sectionBlock =
              vtkMultiBlockDataSet::SafeDownCast(dataSet->GetBlock(block));
            if(!sectionBlock)
              {
              continue;
              }
            vtkNew<vtkMultiBlockDataSet> transdataSet;
            transdataSet->SetNumberOfBlocks(sectionBlock->GetNumberOfBlocks());
            for(int layer=0; layer<sectionBlock->GetNumberOfBlocks(); layer++)
              {
              vtkPolyData* polyBlock =
                vtkPolyData::SafeDownCast(sectionBlock->GetBlock(layer));
              if(!polyBlock)
                {
                continue;
                }

              // move the polydata to the correct position
              vtkTransform *transform = vtkTransform::New();

              if(this->AssyLattice.GetGeometryType() == HEXAGONAL)
                {
                double pinDistFromCenter = pincell->pitchX * (i);
                  /*
                  if(this->AssyLattice.GetGeometryType() == HEXAGONAL && i>0)
                  {
                    layerRadius = pincell->pitchX * (i);
                    for(int c = 0; c < 6; c++)
                    {
                      double angle = 2 * (vtkMath::Pi() / 6.0) * (c + 3.5);
                      layerCorners[c][0] = layerRadius * cos(angle);
                      layerCorners[c][1] = layerRadius * sin(angle);
                    }
                  }*/
                double tX=hexDuct->x, tY=hexDuct->y, tZ=0.0;
                int cornerIdx;
                if(i == 1)
                  {
                  cornerIdx = j%6;
                  tX += pinDistFromCenter*layerCorners[cornerIdx][0];
                  tY += pinDistFromCenter*layerCorners[cornerIdx][1];
                  }
                else if( i > 1)
                  {
                  cornerIdx = j / i;
                  int idxOnEdge = j%i;
                  if(idxOnEdge == 0) // one of the corners
                    {
                    tX += pinDistFromCenter*layerCorners[cornerIdx][0];
                    tY += pinDistFromCenter*layerCorners[cornerIdx][1];
                    }
                  else
                    {
                    // for each layer, we should have (numLayers-2) middle hexes
                    // between the corners
                    double deltx, delty, numSegs = i, centerPos[2];
                    int idxNext = cornerIdx==5 ? 0 : cornerIdx+1;
                    deltx = pinDistFromCenter*(layerCorners[idxNext][0] - layerCorners[cornerIdx][0]) / numSegs;
                    delty = pinDistFromCenter*(layerCorners[idxNext][1] - layerCorners[cornerIdx][1]) / numSegs;
                    centerPos[0] = pinDistFromCenter*layerCorners[cornerIdx][0] + deltx * (idxOnEdge);
                    centerPos[1] = pinDistFromCenter*layerCorners[cornerIdx][1] + delty * (idxOnEdge);
                    tX += centerPos[0];
                    tY += centerPos[1];
                    }
                  }

                transform->Translate(tX, tY, tZ);
                }
              else
                {
                transform->Translate(chamberStart + i * cellLength + 0.5 * pincell->pitchX,
                  chamberStart + j * cellLength + 0.5 * pincell->pitchY,
                  0);
                }

              vtkNew<vtkTransformFilter> filter;
              filter->SetTransform(transform);
              transform->Delete();
              filter->SetInputDataObject(polyBlock);
              filter->Update();
              transdataSet->SetBlock(layer, filter->GetOutput());
              }
            pinDataSet->SetBlock(block, transdataSet.GetPointer());
            }
//          dataSet->Delete();
          this->Data->SetBlock(startBlock+j, pinDataSet.GetPointer());
          }
        else
          {
          this->Data->SetBlock(startBlock+j, NULL);
          }
        }
      else
        {
        this->Data->SetBlock(startBlock+j, NULL);
        }
      }
    }

  // setup ducts
  double z, deltaZ, height;
  size_t numDucts = this->AssyDuct.Ducts.size();
  for(size_t i = 0; i < numDucts; i++)
    {
    Duct *duct = this->AssyDuct.Ducts[i];

    vtkCmbDuctSource *ductSource = vtkCmbDuctSource::New();
    z = duct->z1;
    height = duct->z2 - duct->z1;
    double deltaZ = height * 0.0005;
    // For first duct, move the Origin up in z by 0.05 % of the the Height so
    // that the bottoms of the pins are not covered by duct's bottom
    // For last duct, Reduce the height by 0.1 % of the Height so
    // that the tops of the pins are not covered by duct's top
    if(i == 0) // first duct
      {
      z = duct->z1 + deltaZ;
      // if more than one duct, first duct height need to be reduced by deltaZ
      height = numDucts > 1 ? height - deltaZ : height - 2*deltaZ;
      }
    else if (i == numDucts - 1) // last duct
      {
      height -= 2*deltaZ;
      }

    ductSource->SetOrigin(duct->x, duct->y, z);
    ductSource->SetHeight(height);
    ductSource->SetGeometryType(
      this->AssyLattice.GetGeometryType()== HEXAGONAL ?
      CMBNUC_ASSY_HEX_DUCT : CMBNUC_ASSY_RECT_DUCT);

    for(size_t j = 0; j < duct->thicknesses.size()/2; j++)
      {
      ductSource->AddLayer(duct->thicknesses[2*j], duct->thicknesses[2*j+1]);
      }

    ductSource->Update();

    this->Data->SetBlock(this->Data->GetNumberOfBlocks() - i - 1, ductSource->GetOutput());
    ductSource->Delete();
    }

  return this->Data;
}

vtkMultiBlockDataSet* cmbNucAssembly::CreatePinCellMultiBlock(PinCell* pincell, bool cutaway)
{
  if(pincell->cylinders.size() + pincell->frustums.size() == 0)
    {
    return vtkMultiBlockDataSet::New();
    }

  // There are two child multibock, one for cylinders, one for frustums
  vtkMultiBlockDataSet *dataSet = vtkMultiBlockDataSet::New();
  dataSet->SetNumberOfBlocks(pincell->cylinders.size() + pincell->frustums.size());

  // build all cylinders and frustums
  const int PinCellResolution = 16;
  size_t numCyls = pincell->cylinders.size();
  for(size_t j = 0; j < numCyls; j++)
    {
    Cylinder *cylinder = pincell->cylinders[j];

    vtkSmartPointer<vtkCmbLayeredConeSource> coneSource =
      vtkSmartPointer<vtkCmbLayeredConeSource>::New();
    coneSource->SetNumberOfLayers(pincell->GetNumberOfLayers());
    coneSource->SetBaseCenter(0, 0, cylinder->z1);
    coneSource->SetHeight(cylinder->z2 - cylinder->z1);

    for(int k = 0; k < pincell->GetNumberOfLayers(); k++)
      {
      coneSource->SetBaseRadius(k, pincell->radii[k] * cylinder->r);
      coneSource->SetTopRadius(k, pincell->radii[k] * cylinder->r);
      }
    coneSource->SetResolution(PinCellResolution);
    double direction[] = { 0, 0, 1 };
    coneSource->SetDirection(direction);
    coneSource->Update();

    if(cutaway)
      {
      vtkMultiBlockDataSet *coneData = coneSource->GetOutput();
      for(int block = 0; block < coneData->GetNumberOfBlocks(); block++)
        {
        vtkPolyData *coneLayerData =
          vtkPolyData::SafeDownCast(coneData->GetBlock(block));
        vtkNew<vtkClipClosedSurface> clipper;
        vtkNew<vtkPlaneCollection> clipPlanes;
        vtkNew<vtkPlane> plane;
        vtkNew<vtkPolyDataNormals> normals;
        plane->SetOrigin(0, 0 + 0.001 * block, 0);
        plane->SetNormal(0, 1, 0);
        clipPlanes->AddItem(plane.GetPointer());
        clipper->SetClippingPlanes(clipPlanes.GetPointer());
        clipper->SetActivePlaneId(0);
        clipper->SetClipColor(1.0,1.0,1.0);
        clipper->SetActivePlaneColor(1.0,1.0,0.8);
        clipper->GenerateOutlineOff();
        clipper->SetInputData(coneLayerData);
        clipper->GenerateFacesOn();
        normals->SetInputConnection(clipper->GetOutputPort());
        normals->Update();
        coneData->SetBlock(block, normals->GetOutput());
        }
      dataSet->SetBlock(j, coneData);
      }
    else
      {
      dataSet->SetBlock(j, coneSource->GetOutput());
      }
    }

  for(size_t j = 0; j < pincell->frustums.size(); j++)
    {
    Frustum* frustum = pincell->frustums[j];

    vtkSmartPointer<vtkCmbLayeredConeSource> coneSource =
      vtkSmartPointer<vtkCmbLayeredConeSource>::New();
    coneSource->SetNumberOfLayers(pincell->GetNumberOfLayers());
    coneSource->SetBaseCenter(0, 0, frustum->z1);
    coneSource->SetHeight(frustum->z2 - frustum->z1);

    for(int k = 0; k < pincell->GetNumberOfLayers(); k++)
      {
      coneSource->SetBaseRadius(k, pincell->radii[k] * frustum->r1);
      coneSource->SetTopRadius(k, pincell->radii[k] * frustum->r2);
      }
    coneSource->SetResolution(PinCellResolution);
    double direction[] = { 0, 0, 1 };
    coneSource->SetDirection(direction);
    coneSource->Update();

    if(cutaway)
      {
      vtkMultiBlockDataSet *coneData = coneSource->GetOutput();
      for(int block = 0; block < coneData->GetNumberOfBlocks(); block++)
        {
        vtkPolyData *coneLayerData =
          vtkPolyData::SafeDownCast(coneData->GetBlock(block));
        vtkNew<vtkClipClosedSurface> clipper;
        vtkNew<vtkPlaneCollection> clipPlanes;
        vtkNew<vtkPlane> plane;
        vtkNew<vtkPolyDataNormals> normals;
        plane->SetOrigin(0, 0 + 0.001 * block, 0);
        plane->SetNormal(0, 1, 0);
        clipPlanes->AddItem(plane.GetPointer());
        clipper->SetClippingPlanes(clipPlanes.GetPointer());
        clipper->SetActivePlaneId(0);
        clipper->SetClipColor(1.0,1.0,1.0);
        clipper->SetActivePlaneColor(1.0,1.0,0.8);
        clipper->GenerateOutlineOff();
        clipper->SetInputData(coneLayerData);
        clipper->GenerateFacesOn();
        normals->SetInputConnection(clipper->GetOutputPort());
        normals->Update();
        coneData->SetBlock(block, normals->GetOutput());
        }
      dataSet->SetBlock(numCyls+j, coneData);
      }
    else
      {
      dataSet->SetBlock(numCyls+j, coneSource->GetOutput());
      }
    }

  return dataSet;
}
