#ifndef __cmbNucInputPropertiesWidget_h
#define __cmbNucInputPropertiesWidget_h

#include <QWidget>
#include "cmbNucPartDefinition.h"
#include <QStringList>

class cmbNucInputPropertiesWidgetInternal;

class cmbNucInputPropertiesWidget : public QWidget
{
  Q_OBJECT

public:
  cmbNucInputPropertiesWidget(QWidget* parent=0);
  virtual ~cmbNucInputPropertiesWidget();

  // Description:
  // Set the assembly object the widget will be interacting
  void setObject(AssyPartObj* selObj, const char* name,
    const QStringList& materials);
  AssyPartObj* getObject() {return this->CurrentObject;}
  
signals:
  // Description:
  // Fired when the current object is modified
  void currentObjectModified(AssyPartObj* selObj);
  
protected slots:
  // Invoked when Apply button clicked
  void onApply();
  // Invoked when Reset button clicked
  void onReset();
  // reset property panel with given object
  void resetMaterial(Material* material);
  void resetPinCell(PinCell* pincell);
  void resetFrustum(Frustum* frust);
  void resetCylinder(Cylinder* cylin);
  void resetDuct(Duct* duct);
  // apply property panel to given object
  void applyToMaterial(Material* material);
  void applyToPinCell(PinCell* pincell);
  void applyToFrustum(Frustum* frust);
  void applyToCylinder(Cylinder* cylin);
  void applyToDuct(Duct* duct);

  // Slot for duct layers
  void onNumberOfDuctLayersChanged(int numLayers);
  void onCurrentDuctLayerChanged(int idx);
  // the following three works on current duct layer
  void onCurrentDuctMaterialChanged();
  void onDuctThicknessChanged();

private:
  cmbNucInputPropertiesWidgetInternal* Internal;

  void initUI();
  AssyPartObj* CurrentObject;
};
#endif