#ifndef __cmbAssyParametersWidget_h
#define __cmbAssyParametersWidget_h

#include <QWidget>
#include "cmbNucPartDefinition.h"
#include <QStringList>

class cmbNucAssembly;

class cmbAssyParametersWidget : public QWidget
{
  Q_OBJECT

public:
  cmbAssyParametersWidget(QWidget* p);
  virtual ~cmbAssyParametersWidget();

  // Description:
  // set/get the assembly that this widget with be interact with
  void setAssembly(cmbNucAssembly*);
  cmbNucAssembly* getAssembly(){return this->Assembly;}

signals:
  void valuesChanged();

public slots:
  // Invoked when Apply button clicked
  void onApply();
  // Invoked when Reset button clicked
  void onReset();

public slots:  // reset property panel with given object
  void resetAssembly(cmbNucAssembly* assy);

  // apply property panel to given object
  void applyToAssembly(cmbNucAssembly* assy);

  void addRotation();
  void addSection();
  void deleteSelected();

private:
  class cmbAssyParametersWidgetInternal;
  cmbAssyParametersWidgetInternal* Internal;

  void initUI();
  cmbNucAssembly *Assembly;
};
#endif
