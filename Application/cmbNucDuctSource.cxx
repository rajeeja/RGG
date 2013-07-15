/*=========================================================================

Copyright (c) 1998-2012 Kitware Inc. 28 Corporate Drive,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced,
distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO
PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
=========================================================================*/
#include "cmbNucDuctSource.h"

#include "vtkMultiBlockDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPolyData.h"
#include "vtkTransform.h"
#include "vtkCellArray.h"

#include <math.h>

vtkStandardNewMacro(cmbNucDuctSource);

//----------------------------------------------------------------------------
cmbNucDuctSource::cmbNucDuctSource()
{
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
cmbNucDuctSource::~cmbNucDuctSource()
{
}

//----------------------------------------------------------------------------
int cmbNucDuctSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get multi-block data set
  vtkMultiBlockDataSet *output =
    vtkMultiBlockDataSet::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT())
    );

  // create and add each layer to the output
  output->SetNumberOfBlocks(this->GetNumberOfLayers());

  for(int i = 0; i < this->GetNumberOfLayers(); i++)
    {
    vtkPolyData *polyData = this->CreateLayer(i);
    output->SetBlock(i, polyData);
    polyData->Delete();
    }

  return 1;
}

//----------------------------------------------------------------------------
int cmbNucDuctSource::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
//  vtkInformation *outInfo = outputVector->GetInformationObject(0);
//  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);
  return 1;
}

//----------------------------------------------------------------------------
vtkPolyData* cmbNucDuctSource::CreateLayer(int layer)
{
  // get parameters for the layer
  double x = this->Origin[0];
  double y = this->Origin[1];
  double z = this->Origin[2];

  // width (x), length (y), height (z)
  double w = this->Layers[layer*2+0];
  double l = this->Layers[layer*2+1];
  double h = this->Height;

  // adjust layer origin and size based on the sizes of the previous layers
  x += this->Layers[this->Layers.size()-2] - w;
  y += this->Layers[this->Layers.size()-1] - l;

  w -= this->Layers[this->Layers.size()-2] - w;
  l -= this->Layers[this->Layers.size()-1] - l;

  vtkPolyData *polyData = vtkPolyData::New();
  polyData->Allocate();

  vtkPoints *points = vtkPoints::New();
  vtkCellArray *cells = vtkCellArray::New();

  // create layer
  vtkIdType p0 = points->InsertNextPoint(x, y, z);
  vtkIdType p1 = points->InsertNextPoint(x+l, y, z);
  vtkIdType p2 = points->InsertNextPoint(x+l, y+w, z);
  vtkIdType p3 = points->InsertNextPoint(x, y+w, z);

  vtkIdType p4 = points->InsertNextPoint(x, y, z+h);
  vtkIdType p5 = points->InsertNextPoint(x+l, y, z+h);
  vtkIdType p6 = points->InsertNextPoint(x+l, y+w, z+h);
  vtkIdType p7 = points->InsertNextPoint(x, y+w, z+h);

  vtkIdType c0[] = { p0, p4, p5, p1 };
  cells->InsertNextCell(4, c0);

  vtkIdType c1[] = { p1, p5, p6, p2 };
  cells->InsertNextCell(4, c1);

  vtkIdType c2[] = { p2, p6, p7, p3 };
  cells->InsertNextCell(4, c2);

  vtkIdType c3[] = { p3, p7, p4, p0 };
  cells->InsertNextCell(4, c3);

  polyData->SetPoints(points);
  polyData->SetPolys(cells);

  points->Delete();
  cells->Delete();

  return polyData;
}

//----------------------------------------------------------------------------
void cmbNucDuctSource::AddLayer(double x, double y)
{
  this->Layers.push_back(x);
  this->Layers.push_back(y);
}

//----------------------------------------------------------------------------
int cmbNucDuctSource::GetNumberOfLayers()
{
  return this->Layers.size() / 2;
}

//----------------------------------------------------------------------------
void cmbNucDuctSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}