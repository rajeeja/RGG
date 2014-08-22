
#include "vtkCmbLayeredConeSource.h"

#include <vtkObjectFactory.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>

#include <vtkCellArray.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include "vtkPolyDataNormals.h"
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkNew.h>
#include <vtkMath.h>
#include <vtkAppendPolyData.h>
#include <vtkDoubleArray.h>

#include <vtkDelaunay2D.h>
#include <vtkPolygon.h>
#include <vtkSmartPointer.h>

#include <QDebug>

#include <cassert>
#include <algorithm>

vtkStandardNewMacro(vtkCmbLayeredConeSource);

vtkCmbLayeredConeSource::vtkCmbLayeredConeSource()
{
  this->SetNumberOfInputPorts(0);

  this->Height = 1.0;

  this->BaseCenter[0] = 0.0;
  this->BaseCenter[1] = 0.0;
  this->BaseCenter[2] = 0.0;

  this->Direction[0] = 0.0;
  this->Direction[1] = 0.0;
  this->Direction[2] = 1.0;
  this->GenerateNormals = 1;
  this->GenerateEnds = 1;
}

vtkCmbLayeredConeSource::~vtkCmbLayeredConeSource()
{
}

void vtkCmbLayeredConeSource::SetNumberOfLayers(int layers)
{
  this->LayerRadii.resize(layers);
  this->Modified();
}

int vtkCmbLayeredConeSource::GetNumberOfLayers()
{
  return this->LayerRadii.size();
}

void vtkCmbLayeredConeSource::SetTopRadius(int layer, double r1, double r2)
{
  this->LayerRadii[layer].TopRadii[0] = r1;
  this->LayerRadii[layer].TopRadii[1] = r2;
  this->Modified();
}

void vtkCmbLayeredConeSource::SetBaseRadius(int layer, double r1, double r2)
{
  this->LayerRadii[layer].BaseRadii[0] = r1;
  this->LayerRadii[layer].BaseRadii[1] = r2;
  this->Modified();
}

void vtkCmbLayeredConeSource::SetTopRadius(int layer, double radius)
{
  this->LayerRadii[layer].TopRadii[0] = this->LayerRadii[layer].TopRadii[1] = radius;
  this->Modified();
}

double vtkCmbLayeredConeSource::GetTopRadius(int layer)
{
  return this->LayerRadii[layer].TopRadii[0];
}

void vtkCmbLayeredConeSource::SetBaseRadius(int layer, double radius)
{
  this->LayerRadii[layer].BaseRadii[0] = this->LayerRadii[layer].BaseRadii[1] = radius;
  this->Modified();
}

void vtkCmbLayeredConeSource::SetResolution(int layer, int res)
{
  this->LayerRadii[layer].Resolution = res;
}

#define ADD_DATA(IN)                         \
if (this->GenerateNormals)                   \
{                                            \
  vtkNew<vtkPolyDataNormals> normals;        \
  normals->SetInputDataObject(IN);           \
  normals->ComputePointNormalsOn();          \
  normals->Update();                         \
  output->SetBlock(i, normals->GetOutput()); \
}                                            \
else                                         \
{                                            \
  output->SetBlock(i, IN);                   \
}

double vtkCmbLayeredConeSource::GetBaseRadius(int layer)
{
  return this->LayerRadii[layer].BaseRadii[0];
}

int vtkCmbLayeredConeSource::RequestData(
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
  int innerRes = 0;
  int outerRes = 0;
  double * innerBottomR = NULL;
  double * innerTopR = NULL;
  double * outerBottomR = NULL;
  double * outerTopR = NULL;

  vtkTransform *t = vtkTransform::New();
  t->Translate(this->BaseCenter[0], this->BaseCenter[1], this->BaseCenter[2]);
  if( this->Direction[0] != 0.0 || this->Direction[1] != 0.0 || this->Direction[2] != 1.0 )
    {
    assert(!"Change in direction is currently not supported");
    }
  vtkNew<vtkTransformPolyDataFilter> filter;
  filter->SetTransform(t);
  t->Delete();

  for(int i = 0; i < this->GetNumberOfLayers(); i++)
    {
    outerBottomR = this->LayerRadii[i].BaseRadii;
    outerTopR = this->LayerRadii[i].TopRadii;
    outerRes = this->LayerRadii[i].Resolution;
    vtkPolyData * tmpLayer = CreateLayer( this->Height,
                                          innerBottomR, outerBottomR,
                                          innerTopR,    outerTopR,
                                          innerRes,     outerRes );
    innerBottomR = outerBottomR;
    innerTopR    = outerTopR;
    innerRes     = outerRes;
    if(tmpLayer == NULL)
    {
      output->SetBlock(i, tmpLayer);
      continue;
    }

    filter->SetInputDataObject(tmpLayer);
    if(tmpLayer != NULL) tmpLayer->Delete();
    filter->Update();
    if (this->GenerateNormals)
      {
      vtkNew<vtkPolyDataNormals> normals;
      normals->SetInputConnection(filter->GetOutputPort());
      normals->ComputePointNormalsOn();
      normals->Update();
      output->SetBlock(i, normals->GetOutput());
      }
    else
      {
      output->SetBlock(i, filter->GetOutput());
      }
    }

  return 1;
}

namespace
{
  void Upsample(vtkPoints *points, double * pt0, double * pt1, int number)
  {
    double tmpPt[] = {pt0[0],pt0[1],pt0[2]};
    int id = points->InsertNextPoint(tmpPt);
    double d[] = {pt1[0]-pt0[0],pt1[1]-pt0[1]};
    for(int i = 1; i < (number-1); ++i)
    {
      double r = static_cast<double>(i)/(number-1.0);
      tmpPt[0] = pt0[0] + r*d[0];
      tmpPt[1] = pt0[1] + r*d[1];
      id = points->InsertNextPoint(tmpPt);
    }
    pt0[0] = pt1[0];
    pt0[1] = pt1[1];
  }

  class GeneratePoints
  {
  public:
    GeneratePoints(int res)
    {
      bool rect = false;
      if(res == 4)
      {
        res = 6;
        rect = true;
      }
      multX.resize(res);
      multY.resize(res);
      if(rect)
      {
        multX[0] = 1;
        multY[0] = 0;

        multX[1] = 1;
        multY[1] = 1;

        multX[2] = -1;
        multY[2] = 1;

        multX[3] = -1;
        multY[3] = 0;

        multX[4] = -1;
        multY[4] = -1;

        multX[5] = 1;
        multY[5] = -1;
      }
      else
      {
        double angle = 2.0*3.141592654/res;
        for(int j = 0; j < res; j++)
        {
          multX[j] = cos(j * angle);
          multY[j] = sin(j * angle);
        }
      }
    }
    int usedResolution() { return static_cast<int>(multX.size()); }
    void AddPoints(vtkPoints *points, double h, double * r, double shift)
    {
      double point[] = {0,0,h};
      int res = usedResolution();
      for(int j = 0; j < res; j++)
      {
        compPt(j, point, r, shift);
        points->InsertNextPoint(point);
      }
    }
    void AddPointsUpsampled(vtkPoints *points, double h, double * r, int maxRes, double shift)
    {
      double start[] = {0,0,h};
      double point[] = {0,0,h};
      double prevPoint[] = {0,0,h};
      int res = usedResolution();
      if(res == 0) return;
      int ptsPerSide = maxRes/res + 1;
      assert(maxRes%res == 0); //right now only handle res that are multiples of each other
      assert(multX[0] == 1 && multY[0] == 0);
      start[0] = prevPoint[0] = (r[0] + shift);
      start[1] = prevPoint[1] = 0;
      for(int j = 1; j < res; j++)
      {
        compPt(j, point, r, shift);
        Upsample(points, prevPoint, point, ptsPerSide);
      }
      Upsample(points, prevPoint, start, ptsPerSide);
    }
  protected:
    std::vector<double> multX;
    std::vector<double> multY;
    inline void compPt(int i, double * point, double * r, double shift)
    {
      point[0] = (r[0] + shift) * multX[i];
      point[1] = (r[1] + shift) * multY[i];
    }
  };
};

void vtkCmbLayeredConeSource
::TriangulateEnd(const int innerRes, const int outerRes,
                 bool forceDelaunay, vtkCellArray *cells, vtkPoints * fullPoints)
{
  vtkIdType pts[32];
  const int offset = outerRes+innerRes;
  const bool has_hole = innerRes != 0;
  if(!forceDelaunay && has_hole && outerRes == innerRes && outerRes < 32)
  {
    for(int i = 0; i < innerRes; ++i)
    {
      pts[0] = i;
      pts[3] = (i+1)%innerRes;
      pts[2] = (i+1)%innerRes + innerRes;
      pts[1] = i+innerRes;
      cells->InsertNextCell(4, pts);
    }
    for(int i = 0; i < innerRes; ++i)
    {
      pts[0] = i + offset;
      pts[3] = (i+1)%innerRes + offset;
      pts[2] = (i+1)%innerRes + innerRes + offset;
      pts[1] = i+innerRes + offset;
      cells->InsertNextCell(4, pts);
    }
  }
  else if(!forceDelaunay && !has_hole && outerRes<32)
  {
    for(int i = 0; i < outerRes; ++i)
    {
      pts[outerRes - 1 - i] = i;
    }
    cells->InsertNextCell(outerRes, pts);
    for(int i = 0; i < outerRes; ++i)
    {
      pts[outerRes - 1 - i] = i+offset;
    }
    cells->InsertNextCell(outerRes, pts);
  }
  else
  {
    vtkPoints *points = vtkPoints::New();
    // Create a cell array to store the polygon in
    vtkSmartPointer<vtkCellArray> aCellArray =
      vtkSmartPointer<vtkCellArray>::New();

    // Define a polygonal hole with a clockwise polygon
    vtkSmartPointer<vtkPolygon> aPolygon =
      vtkSmartPointer<vtkPolygon>::New();
    for(int i = 0; i < outerRes; ++i)
    {
      double * pts = fullPoints->GetPoint(i);
      points->InsertNextPoint(pts[0], pts[1], 0);
    }
    for(int i = outerRes; i < offset; ++i)
    {
      double * pts = fullPoints->GetPoint(i);
      points->InsertNextPoint(pts[0], pts[1], 0);
      aPolygon->GetPointIds()->InsertNextId(offset-(i-outerRes)-1);
    }

    vtkSmartPointer<vtkPolyData> aPolyData =
      vtkSmartPointer<vtkPolyData>::New();
    aPolyData->SetPoints(points);

    aCellArray->InsertNextCell(aPolygon);
    vtkSmartPointer<vtkPolyData> boundary =
      vtkSmartPointer<vtkPolyData>::New();
    boundary->SetPoints(aPolyData->GetPoints());
    boundary->SetPolys(aCellArray);
    vtkSmartPointer<vtkDelaunay2D> delaunay =
      vtkSmartPointer<vtkDelaunay2D>::New();

    delaunay->SetInputData(aPolyData);
    delaunay->SetSourceData(boundary);

    delaunay->Update();

    vtkPolyData * pd = delaunay->GetOutput();

    vtkCellArray * tri = pd->GetPolys();
    tri->InitTraversal();

    vtkIdType npts;
    vtkIdType * tmppts;
    while(tri->GetNextCell(npts, tmppts))
    {
      assert(npts == 3);
      pts[0] = tmppts[0];
      pts[1] = tmppts[2];
      pts[2] = tmppts[1];
      cells->InsertNextCell(3,pts);
    }

    tri->InitTraversal();
    while(tri->GetNextCell(npts, tmppts))
    {
      for(int j = 0; j < npts; ++j) pts[j] = tmppts[j] + offset;
      cells->InsertNextCell(npts,pts);
    }
    points->Delete();
  }
}

vtkPolyData *
vtkCmbLayeredConeSource
::CreateLayer( double h,
               double * innerBottomR, double * outerBottomR,
               double * innerTopR,    double * outerTopR,
               int innerRes, int outerRes )
{
  if(outerTopR == NULL || outerBottomR == NULL) return NULL;
  if(outerRes == 0) return NULL;
  vtkPolyData *polyData = vtkPolyData::New();
  polyData->Allocate();

  vtkPoints *points = vtkPoints::New();
  vtkCellArray *cells = vtkCellArray::New();

  points->SetDataTypeToDouble(); //used later during transformation

  vtkIdType pts[5];
  double tpt[3];
  bool forceDelaunay = false;

  if(innerRes == 0 && InnerPoints.empty())
  {
    GeneratePoints gp(outerRes);
    outerRes = gp.usedResolution();
    points->Allocate(outerRes*2);
    gp.AddPoints(points, 0, outerBottomR, 0);
    gp.AddPoints(points, h, outerTopR, 0);
  }
  else if(innerRes == 0 && !InnerPoints.empty())
  {
    GeneratePoints gp(outerRes);
    outerRes = gp.usedResolution();
    innerRes = InnerPoints.size();
    points->Allocate((outerRes+innerRes)*2);
    gp.AddPoints(points, 0, outerBottomR, 0);
    for(unsigned int i = 0; i < InnerPoints.size(); ++i)
    {
      points->InsertNextPoint(InnerPoints[i][0],InnerPoints[i][1],0);
    }
    gp.AddPoints(points, h, outerTopR, 0);
    for(unsigned int i = 0; i < InnerPoints.size(); ++i)
    {
      points->InsertNextPoint(InnerPoints[i][0],InnerPoints[i][1],h);
    }
    forceDelaunay = true;
  }
  else if(innerRes == outerRes)
  {
    GeneratePoints gp(outerRes);
    innerRes = outerRes = gp.usedResolution();
    points->Allocate(outerRes*4);
    gp.AddPoints(points, 0, outerBottomR, 0);
    gp.AddPoints(points, 0, innerBottomR, 0.0005);
    gp.AddPoints(points, h, outerTopR,    0);
    gp.AddPoints(points, h, innerTopR,    0.0005);
  }
  else
  {
    GeneratePoints gpO(outerRes);
    GeneratePoints gpI(innerRes);
    points->Allocate((outerRes+innerRes)*2);
    gpO.AddPoints(points, 0, outerBottomR, 0);
    gpI.AddPoints(points, 0, innerBottomR, 0.0005);
    gpO.AddPoints(points, h, outerTopR, 0);
    gpI.AddPoints(points, h, innerTopR,    0.0005);
  }

  //Add bottom calls
  if(GenerateEnds) { TriangulateEnd(innerRes, outerRes, forceDelaunay, cells, points); }
  //Outer Wall;
  if(1)
  {
    int offset = outerRes+innerRes;
    for(unsigned int i = 0; i < outerRes; ++i)
    {
      pts[0] = i;
      pts[1] = (i+1) % outerRes;
      pts[2] = (i+1) % outerRes + offset;
      pts[3] = i + offset;
      cells->InsertNextCell(4, pts);
    }
  }
  //Inner Wall
  if(1)
  {
    int offset = outerRes+innerRes;
    for(unsigned int i = 0; i < innerRes; ++i)
    {
      pts[0] = i + outerRes;
      pts[3] = (i+1) % innerRes + outerRes;
      pts[2] = (i+1) % innerRes + outerRes+ offset;
      pts[1] = i + outerRes + offset;
      cells->InsertNextCell(4, pts);
    }
  }
  polyData->SetPoints(points);
  polyData->SetPolys(cells);

  points->Delete();
  cells->Delete();

  return polyData;
}

void vtkCmbLayeredConeSource::addInnerPoint(double x, double y)
{
  std::vector<double> pt(2);
  pt[0] = x; pt[1] = y;
  InnerPoints.push_back(pt);
}
void vtkCmbLayeredConeSource::clearInnerPoints()
{
  InnerPoints.clear();
}

void vtkCmbLayeredConeSource::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
