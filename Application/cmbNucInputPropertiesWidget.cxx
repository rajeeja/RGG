
#include "cmbNucInputPropertiesWidget.h"
#include "ui_qInputPropertiesWidget.h"

#include "cmbNucAssembly.h"
#include "cmbNucCore.h"
#include "cmbNucPinCellEditor.h"
#include "cmbNucDuctCellEditor.h"
#include "cmbNucPinCell.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucMainWindow.h"
#include "cmbCoreParametersWidget.h"
#include "cmbAssyParametersWidget.h"
#include "cmbNucDefaultWidget.h"
#include "cmbNucDefaults.h"

#include <QLabel>
#include <QPointer>
#include <QtDebug>
#include <QColorDialog>
#include <QMessageBox>
#include <QComboBox>

#define set_and_test_for_change(X, Y)\
   change |= ((Y) != (X)); \
   X = (Y)

class cmbNucInputPropertiesWidgetInternal :
  public Ui::InputPropertiesWidget
{
public:
  QPointer <cmbNucPinCellEditor> PinCellEditor;
  QPointer <cmbNucDuctCellEditor> DuctCellEditor;
};

//-----------------------------------------------------------------------------
cmbNucInputPropertiesWidget::cmbNucInputPropertiesWidget(cmbNucMainWindow *mainWindow)
  : QWidget(mainWindow),
    MainWindow(mainWindow)
{
  this->Internal = new cmbNucInputPropertiesWidgetInternal;
  this->Internal->setupUi(this);
  this->initUI();
  this->CurrentObject = NULL;
  this->RebuildCoreGrid = false;
  this->Assembly = NULL;
  this->Core = NULL;
}

//-----------------------------------------------------------------------------
cmbNucInputPropertiesWidget::~cmbNucInputPropertiesWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::initUI()
{
  this->CoreProperties = new cmbCoreParametersWidget(this);
  this->Internal->CoreConfLayout->addWidget(this->CoreProperties);
  connect(this->CoreProperties, SIGNAL(valuesChanged()),
          this, SIGNAL(valuesChanged()));
  connect(this->CoreProperties, SIGNAL(drawCylinder(double, int)),
          this, SIGNAL(drawCylinder(double,int)));
  connect(this->CoreProperties, SIGNAL(clearCylinder()),
          this, SIGNAL(clearCylinder()));

  this->assyConf = new cmbAssyParametersWidget(this);
  this->Internal->AssyConfLayout->addWidget(this->assyConf);
  connect(this->assyConf, SIGNAL(valuesChanged()),
          this, SIGNAL(valuesChanged()));

  this->Internal->colorSwatch->setFrameStyle(QFrame::Box | QFrame::Plain);
  this->Internal->assyColorSwatch->setFrameStyle(QFrame::Box | QFrame::Plain);

  QObject::connect(this->Internal->ApplyButton, SIGNAL(clicked()),
    this, SLOT(onApply()));
  QObject::connect(this->Internal->ResetButton, SIGNAL(clicked()),
    this, SLOT(onReset()));

  QObject::connect(this->Internal->ResetButton, SIGNAL(clicked()),
                   this, SIGNAL(reset()));

  QObject::connect(this->Internal->latticeX, SIGNAL(valueChanged(int)),
                   this, SIGNAL(sendXSize(int)));
  QObject::connect(this->Internal->latticeY, SIGNAL(valueChanged(int)),
                   this, SIGNAL(sendYSize(int)));
  QObject::connect(this->Internal->coreLatticeX, SIGNAL(valueChanged(int)),
                   this, SIGNAL(sendXSize(int)));
  QObject::connect(this->Internal->coreLatticeY, SIGNAL(valueChanged(int)),
                   this, SIGNAL(sendYSize(int)));

  // pincell related connections
  QObject::connect(this->Internal->colorSelectButton, SIGNAL(clicked()),
    this, SLOT(choosePinLegendColor()));
  QObject::connect(this->Internal->assyColorSelectButton, SIGNAL(clicked()),
    this, SLOT(chooseAssyLegendColor()));

  CoreDefaults = new cmbNucDefaultWidget();
  this->Internal->CoreDefaults->addWidget(CoreDefaults);

  connect( CoreDefaults, SIGNAL(commonChanged()), this, SIGNAL(valuesChanged()));

  assyDefaults = new cmbNucDefaultWidget();
  this->Internal->AssyDefaults->addWidget(assyDefaults);
}

//-----------------------------------------------------------------------------
bool cmbNucInputPropertiesWidget::ductCellIsCrossSectioned()
{
  return this->Internal->DuctCellEditor->isCrossSectioned();
}

//-----------------------------------------------------------------------------
bool cmbNucInputPropertiesWidget::pinCellIsCrossSectioned()
{
  return this->Internal->PinCellEditor->isCrossSectioned();
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::setObject(AssyPartObj* selObj, const char* name)
{
  if(this->CurrentObject == selObj)
    {
    return;
    }
  this->CurrentObject = selObj;
  if(!selObj)
    {
    emit currentObjectNameChanged("");
    this->setEnabled(0);
    return;
    }
  this->setEnabled(true);
  if(name != NULL)
    {
    emit currentObjectNameChanged(selObj->getTitle().c_str());
    }
  else
    {
    emit currentObjectNameChanged("");
    }

  this->onReset();
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::setAssembly(cmbNucAssembly *assyObj)
{
  if(this->Assembly == assyObj)
    {
    return;
    }
  this->Assembly = assyObj;
  assyConf->setAssembly(assyObj);
}

// Invoked when Apply button clicked
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::onApply()
{
  if(this->CurrentObject == NULL)
    {
    return;
    }
  AssyPartObj* selObj = this->CurrentObject;
  PinCell* pincell = NULL;
  cmbNucCore* nucCore = NULL;
  cmbNucAssembly* assy = NULL;
  switch(selObj->GetType())
    {
    case CMBNUC_CORE:
      emit(apply());
      nucCore = dynamic_cast<cmbNucCore*>(selObj);
      this->applyToCore(nucCore);
      break;
    case CMBNUC_ASSEMBLY:
      emit(apply());
      assy = dynamic_cast<cmbNucAssembly*>(selObj);
      this->applyToAssembly(assy);
      break;
    case CMBNUC_ASSY_PINCELL:
      pincell = dynamic_cast<PinCell*>(selObj);
      this->applyToPinCell(pincell);
      break;
    case CMBNUC_ASSY_FRUSTUM_PIN:
    case CMBNUC_ASSY_CYLINDER_PIN:
      /*handled in the pin editor*/
      break;
    case CMBNUC_ASSY_DUCTCELL:
      this->Internal->DuctCellEditor->Apply();
      break;
    default:
      this->setEnabled(0);
      break;
    }
}
// Invoked when Reset button clicked
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::onReset()
{
  if(this->CurrentObject == NULL)
    {
    return;
    }
  AssyPartObj* selObj = this->CurrentObject;
  PinCell* pincell = NULL;
  cmbNucCore* nucCore = NULL;
  cmbNucAssembly* assy = NULL;
  switch(selObj->GetType())
    {
    case CMBNUC_CORE:
      nucCore = dynamic_cast<cmbNucCore*>(selObj);
      this->Internal->stackedWidget->setCurrentWidget(this->Internal->pageCore);
      this->CoreDefaults->set(nucCore->GetDefaults(), true, nucCore->IsHexType());
      this->Internal->coreLabelY->setVisible(!nucCore->IsHexType());
      this->Internal->coreLatticeY->setVisible(!nucCore->IsHexType());
      if(nucCore->IsHexType())
        {
        this->Internal->coreLabelX->setText("Number Of Layers:");
        }
      else
        {
        this->Internal->coreLabelX->setText("X:");
        }
      this->resetCore(nucCore);
      sendLatticeFullMode(cmbNucDraw2DLattice::HEX_FULL);
      emit(sendLattice(nucCore));
      break;
    case CMBNUC_ASSEMBLY:
      assy = dynamic_cast<cmbNucAssembly*>(selObj);
      this->Internal->AssemblyLabelY->setVisible(!assy->IsHexType());
      this->Internal->latticeY->setVisible(!assy->IsHexType());
      this->assyDefaults->set(assy->Defaults, false, assy->IsHexType());
      if(assy->IsHexType())
        {
        this->Internal->AssemblyLabelX->setText("Number Of Layers:");
        }
      else
        {
        this->Internal->AssemblyLabelX->setText("X:");
        }
      this->Internal->stackedWidget->setCurrentWidget(this->Internal->pageAssembly);
      this->setAssembly(assy);
      this->resetAssembly(assy);
      if( this->Core->getLattice().subType & ANGLE_60 &&
          this->Core->getLattice().subType & VERTEX )
        {
        sendLatticeFullMode(cmbNucDraw2DLattice::HEX_FULL);
        }
      else
        {
        sendLatticeFullMode(cmbNucDraw2DLattice::HEX_FULL_30);
        }
      emit(sendLattice(assy));
      break;
    case CMBNUC_ASSY_PINCELL:
      pincell = dynamic_cast<PinCell*>(selObj);
      this->resetPinCell(pincell);
      this->Internal->stackedWidget->setCurrentWidget(
        this->Internal->pagePinCell);
      this->showPinCellEditor();
      emit(select3DModelView());
      emit(sendLattice(NULL));
      break;
    case CMBNUC_ASSY_FRUSTUM_PIN:
    case CMBNUC_ASSY_CYLINDER_PIN:
      /*handled in pin editor*/
      break;
    case CMBNUC_ASSY_DUCTCELL:
      this->Internal->stackedWidget->setCurrentWidget(this->Internal->pageDuctCell);
      this->showDuctCellEditor();
      emit(sendLattice(NULL));
      break;
    default:
      emit(sendLattice(NULL));
      this->setEnabled(0);
      break;
    }
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::clearPincellEditor()
{
  if(this->Internal->PinCellEditor)
  {
    this->Internal->PinCellEditor->clear();
  }
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::clear()
{
  clearPincellEditor();
  this->setObject(NULL,NULL);
  this->Internal->stackedWidget->setCurrentWidget(this->Internal->pageCore);
  this->setAssembly(NULL);
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::pinLabelChanged(PinCell* pincell,
                                                  QString previous,
                                                  QString current)
{
  if(this->CurrentObject == NULL && this->Assembly)
  {
    return;
  }
  //Check to make sure new label is unique
  for(unsigned int i = 0; i < this->Assembly->GetNumberOfPinCells(); ++i)
  {
    PinCell * tpc = this->Assembly->GetPinCell(i);
    if(tpc != NULL && pincell != tpc)
    {
      if(tpc->label == current.toStdString())
      {
        //ERROR!  Should be unique, revert
        QMessageBox msgBox;
        msgBox.setText(current +
                       QString(" is already use as a pin label, reverting to ")+
                       previous);
        msgBox.exec();
        emit(badPinLabel(previous));
        return;
      }
    }
  }
  AssyPartObj* selObj = this->CurrentObject;
  if(selObj->GetType() == CMBNUC_ASSY_PINCELL)
  {
    this->Assembly->getLattice().replaceLabel(previous.toStdString(),
                                             current.toStdString());
  }
  emit currentObjectNameChanged( selObj->getTitle().c_str() );
  emit sendLabelChange(current);
}

void cmbNucInputPropertiesWidget::colorChanged()
{
  if(this->CurrentObject == NULL)
  {
    return;
  }
  AssyPartObj* selObj = this->CurrentObject;
  switch(selObj->GetType())
  {
    case CMBNUC_CORE:
    case CMBNUC_ASSEMBLY:
      emit objGeometryChanged(selObj);
      break;
    case CMBNUC_ASSY_PINCELL:
    case CMBNUC_ASSY_FRUSTUM_PIN:
    case CMBNUC_ASSY_CYLINDER_PIN:
      this->Internal->PinCellEditor->UpdateData();
      break;
    case CMBNUC_ASSY_DUCTCELL:
      this->Internal->DuctCellEditor->update();
      break;
    case CMBNUC_ASSY_LATTICE:
    case CMBNUC_ASSY_BASEOBJ:
    case CMBNUC_ASSY_DUCT:
      this->setEnabled(0);
      break;
  }
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::pinNameChanged(PinCell* pincell,
                                                 QString previous,
                                                 QString current)
{
  if(this->CurrentObject == NULL && this->Assembly)
  {
    return;
  }
  //Check to make sure new label is unique
  for(unsigned int i = 0; i < this->Assembly->GetNumberOfPinCells(); ++i)
  {
    PinCell * tpc = this->Assembly->GetPinCell(i);
    if(tpc != NULL && pincell != tpc)
    {
      if(tpc->name == current.toStdString())
      {
        //ERROR!  Should be unique, revert
        QMessageBox msgBox;
        msgBox.setText(current +
                       QString(" is already use as a pin name, reverting to ")+
                       previous);
        msgBox.exec();
        emit(badPinName(previous));
        return;
      }
    }
  }
}

// reset property panel with given object
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetPinCell(PinCell* pincell)
{
  // Show color swatch with legendColor
  QPalette c_palette = this->Internal->colorSwatch->palette();
  c_palette.setColor(this->Internal->colorSwatch->backgroundRole(), pincell->GetLegendColor());
  this->Internal->colorSwatch->setPalette(c_palette);
  if(this->Internal->PinCellEditor)
  {
    this->Internal->PinCellEditor->Reset();
  }
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetLattice(Lattice* lattice)
{
  this->Internal->latticeX->blockSignals(true);
  this->Internal->latticeX->setValue(lattice->GetDimensions().first);
  if(lattice->GetGeometryType() == RECTILINEAR)
    {
    this->Internal->latticeY->blockSignals(true);
    this->Internal->latticeY->setValue(lattice->GetDimensions().second);
    this->Internal->latticeY->blockSignals(false);
    }
  this->Internal->latticeX->blockSignals(false);
  this->resetAssemblyLattice();
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetAssemblyEditor(cmbNucAssembly* assy)
{
  this->Assembly = assy;
  this->resetAssemblyLattice();
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetAssemblyLattice()
{
  if(this->Assembly)
    {
    QStringList actionList;
    actionList.append("xx");
    // pincells
    for(size_t i = 0; i < this->Assembly->GetNumberOfPinCells(); i++)
      {
      PinCell *pincell = this->Assembly->GetPinCell(i);
      actionList.append(pincell->label.c_str());
      }
    }
}

// apply property panel to given object
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToPinCell(PinCell* pincell)
{
  this->Internal->PinCellEditor->Apply();
  emit this->objGeometryChanged(pincell);
  this->Internal->PinCellEditor->UpdateData();
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToLattice(Lattice* lattice)
{
  bool change = false;
  if(change)
  {
    if(this->Assembly->isPinsAutoCentered()) this->Assembly->centerPins();
    emit valuesChanged();
  }
  emit this->objGeometryChanged(lattice);
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToAssembly(cmbNucAssembly* assy)
{
  std::string new_label = this->Internal->AssyLabel->text().toStdString();
  std::string old_label = assy->getLabel();
  std::replace(new_label.begin(), new_label.end(), ' ', '_');
  if( old_label != new_label)
  {
    if(this->Core->label_unique(new_label))
    {
      //change label
      if(this->Core->getLattice().replaceLabel(old_label,new_label))
      {
        this->Core->setAndTestDiffFromFiles(true);
      }
      assy->setLabel(new_label);
      this->Internal->AssyLabel->setText(new_label.c_str());
      emit currentObjectNameChanged( assy->getTitle().c_str() );
      emit sendLabelChange( QString(new_label.c_str()) );
      emit checkSaveAndGenerate();
    }
    else
    {
      this->Internal->AssyLabel->setText(old_label.c_str());
    }
  }
  this->assyConf->applyToAssembly(assy);
  this->assyDefaults->apply();
  double px, py;
  assy->Defaults->getPitch(px,py);
  bool checked = this->Internal->CenterPins->isChecked();
  this->assyDefaults->setPitchAvail(!checked);
  if(!checked)
  {
    assy->setPitch(px,py);
  }
  assy->setCenterPins(checked);
  emit this->objGeometryChanged(assy);
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::applyToCore(cmbNucCore* nucCore)
{
  bool changed = false;
  if(this->CoreDefaults->assyPitchChanged())
    {
    changed = true;
    nucCore->setAndTestDiffFromFiles(true);
    }
  this->CoreProperties->applyToCore(nucCore);
  this->CoreDefaults->apply();
  nucCore->sendDefaults();
  if(changed)
  {
    nucCore->clearOldGeometry();
    emit valuesChanged();
  }
  emit this->objGeometryChanged(nucCore);
}

//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetAssembly(cmbNucAssembly* assy)
{
  this->assyConf->resetAssembly(assy);
  this->Internal->CenterPins->setChecked(assy->isPinsAutoCentered());
  this->Internal->AssyLabel->setText(assy->getLabel().c_str());

  // Show color swatch with legendColor
  QLabel* swatch = this->Internal->assyColorSwatch;

  QPalette c_palette = swatch->palette();
  c_palette.setColor(swatch->backgroundRole(), assy->GetLegendColor());
  swatch->setPalette(c_palette);
  this->resetLattice(&assy->getLattice());
}
//-----------------------------------------------------------------------------
void cmbNucInputPropertiesWidget::resetCore(cmbNucCore* nucCore)
{
  if(nucCore)
    {
    this->Core = nucCore;
    this->CoreProperties->setCore(nucCore);
    this->CoreProperties->resetCore(nucCore);
    this->Internal->coreLatticeX->blockSignals(true);
    this->Internal->coreLatticeY->blockSignals(true);
    this->Internal->coreLatticeX->setValue(nucCore->GetDimensions().first);
    this->Internal->coreLatticeY->setValue(nucCore->GetDimensions().second);
    this->Internal->coreLatticeX->blockSignals(false);
    this->Internal->coreLatticeY->blockSignals(false);

    QStringList actionList;
    actionList.append("xx");

    if(this->RebuildCoreGrid)
      {
      nucCore->RebuildGrid();
      this->RebuildCoreGrid = false;
      }

    // build the action list
    for(int i = 0; i < nucCore->GetNumberOfAssemblies(); i++)
      {
      cmbNucAssembly *assy = nucCore->GetAssembly(i);
      actionList.append(assy->label.c_str());
      }
    }
}

void cmbNucInputPropertiesWidget::showDuctCellEditor()
{
  DuctCell* ductcell = dynamic_cast<DuctCell*>(this->CurrentObject);
  if(!ductcell)
  {
    return;
  }
  if(this->Internal->DuctCellEditor == NULL)
  {
    this->Internal->DuctCellEditor = new cmbNucDuctCellEditor(this);
    this->Internal->ductEditorContainer->addWidget(this->Internal->DuctCellEditor);
    //Setup connections
    QObject::connect(this->Internal->DuctCellEditor,
                     SIGNAL(ductcellModified(AssyPartObj*)),
                     this, SIGNAL(objGeometryChanged(AssyPartObj*)));
  }
  this->Internal->DuctCellEditor->SetDuctCell(ductcell, this->getAssembly()->IsHexType());
  this->Internal->DuctCellEditor->SetAssembly(this->getAssembly());
}

void cmbNucInputPropertiesWidget::showPinCellEditor()
{
  // get the current pincell
  PinCell* pincell = dynamic_cast<PinCell*>(this->CurrentObject);
  if(!pincell)
    {
    return;
    }
  if(!this->Internal->PinCellEditor)
    {
    this->Internal->PinCellEditor = new cmbNucPinCellEditor(this);
    this->Internal->pinEditorContainer->addWidget(
      this->Internal->PinCellEditor);
    QObject::connect(this->Internal->PinCellEditor,
                     SIGNAL(pincellModified(AssyPartObj*)),
                     this, SIGNAL(objGeometryChanged(AssyPartObj*)));
    QObject::connect( this->Internal->PinCellEditor,
                      SIGNAL(resetView()),
                      this, SIGNAL(resetView()));
    QObject::connect( this->Internal->PinCellEditor,
                      SIGNAL(labelChanged(PinCell*, QString, QString)),
                      this, SLOT(pinLabelChanged(PinCell*, QString, QString)));
    QObject::connect( this->Internal->PinCellEditor,
                      SIGNAL(nameChanged(PinCell*, QString, QString)),
                      this, SLOT(pinNameChanged(PinCell*, QString, QString)));
    QObject::connect( this,
                      SIGNAL(badPinLabel(QString)),
                      this->Internal->PinCellEditor,
                      SLOT(badLabel(QString)));
    QObject::connect( this,
                      SIGNAL(badPinName(QString)),
                      this->Internal->PinCellEditor,
                      SLOT(badName(QString)));
    QObject::connect( this->Internal->PinCellEditor, SIGNAL(valueChange()),
                      this, SIGNAL(valuesChanged()) );
    }
  this->Internal->PinCellEditor->SetPinCell(pincell, this->getAssembly()->IsHexType());
  this->Internal->PinCellEditor->SetAssembly(this->getAssembly());
}

void cmbNucInputPropertiesWidget::choosePinLegendColor()
{
  PinCell* pincell = dynamic_cast<PinCell*>(this->CurrentObject);
  if(!pincell)
    {
    std::cerr << "Error: don't have pincell" << std::endl;
    return;
    }
  QColor selected = QColorDialog::getColor(pincell->GetLegendColor(), this,
    "Select key color for pin cell type");
  if(selected.isValid())
    {
    pincell->SetLegendColor(selected);
    QPalette c_palette = this->Internal->colorSwatch->palette();
    c_palette.setColor(this->Internal->colorSwatch->backgroundRole(), selected);
    this->Internal->colorSwatch->setPalette(c_palette);

    this->resetAssemblyLattice();
    }
}

void cmbNucInputPropertiesWidget::chooseAssyLegendColor()
{
  cmbNucAssembly* assy = dynamic_cast<cmbNucAssembly*>(this->CurrentObject);
  if(!assy)
    {
    std::cerr << "Error: don't have assy" << std::endl;
    return;
    }
  QColor selected = QColorDialog::getColor(assy->GetLegendColor(), this,
    "Select key color for assembly type");
  if(selected.isValid())
    {
    assy->SetLegendColor(selected);
    QPalette c_palette = this->Internal->assyColorSwatch->palette();
    c_palette.setColor(this->Internal->assyColorSwatch->backgroundRole(), selected);
    this->Internal->assyColorSwatch->setPalette(c_palette);

    // We set this flag to denote that the grid should be rebuilt the
    // next time we select the core
    this->RebuildCoreGrid = true;
    }
}
