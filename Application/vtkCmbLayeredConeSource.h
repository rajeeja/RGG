#ifndef __vtkCmbLayeredConeSource_h
#define __vtkCmbLayeredConeSource_h

#include <vector>

#include <vtkMultiBlockDataSetAlgorithm.h>
#include <vtkSmartPointer.h>

class vtkPolyData;
class vtkPoints;
class vtkCellArray;

// Creates a cone with multiple layers. Each layer is a separate block
// in the multi-block output data-set so that individual layers property's
// can be modified with the composite poly-data mapper.
class vtkCmbLayeredConeSource : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkCmbLayeredConeSource* New();
  vtkTypeMacro(vtkCmbLayeredConeSource, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent);

  void SetNumberOfLayers(int layers);
  int GetNumberOfLayers();

  void SetTopRadius(int layer, double radius);
  void SetTopRadius(int layer, double r1, double r2);
  double GetTopRadius(int layer, int s = 0);


  void SetBaseRadius(int layer, double radius);
  void SetBaseRadius(int layer, double r1, double r2);
  double GetBaseRadius(int layer, int s = 0);

  double GetTopThickness(int layer);
  double GetBaseThickness(int layer);

  void addInnerPoint(double x, double y);
  void clearInnerPoints();

  void SetResolution(int layer, int res);
  int GetResolution(int layer);

  vtkSetMacro(Height, double);
  vtkGetMacro(Height, double);

  vtkSetVector3Macro(BaseCenter,double);
  vtkGetVectorMacro(BaseCenter,double,3);

  vtkSetVector3Macro(Direction,double);
  vtkGetVectorMacro(Direction,double,3);

  // Determines whether surface normals should be generated
  // On by default
  vtkSetMacro(GenerateNormals,int);
  vtkGetMacro(GenerateNormals,int);
  vtkBooleanMacro(GenerateNormals,int);

  vtkSetMacro(GenerateEnds,int);
  vtkGetMacro(GenerateEnds,int);
  vtkBooleanMacro(GenerateEnds,int);

  vtkSmartPointer<vtkPolyData> CreateUnitLayer(int l);


protected:
  vtkCmbLayeredConeSource();
  ~vtkCmbLayeredConeSource();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkSmartPointer<vtkPolyData> CreateLayer( double h,
                                            double * innerBottomR, double * outerBottomR,
                                            double * innerTopR,    double * outerTopR,
                                            int innerRes, int outerRes);

  double Height;
  struct radii
  {
    int Resolution;
    double BaseRadii[2];
    double TopRadii[2];
  };
  std::vector<radii> LayerRadii;
  std::vector< std::vector<double> > InnerPoints;
  double BaseCenter[3];
  double Direction[3];
  int Resolution;
  int GenerateNormals;
  int GenerateEnds;

private:
  vtkCmbLayeredConeSource(const vtkCmbLayeredConeSource&);
  void operator=(const vtkCmbLayeredConeSource&);

  void TriangulateEnd(const int innerRes,
                      const int outerRes,
                      bool forceDelaunay,
                      vtkCellArray *cells,
                      vtkPoints * fullPoints);
};

#endif
