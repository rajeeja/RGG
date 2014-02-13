
#include "cmbAssyParametersWidget.h"
#include "ui_qAssyParameters.h"

#include "cmbNucAssembly.h"

#include <QLabel>
#include <QPointer>
#include <QtDebug>

class cmbAssyParametersWidget::cmbAssyParametersWidgetInternal :
  public Ui::qAssyParametersWidget
{
public:

};

//-----------------------------------------------------------------------------
cmbAssyParametersWidget::cmbAssyParametersWidget(QWidget *p)
  : QWidget(p)
{
  this->Internal = new cmbAssyParametersWidgetInternal;
  this->Internal->setupUi(this);
  this->initUI();
  this->Assembly = NULL;
}

//-----------------------------------------------------------------------------
cmbAssyParametersWidget::~cmbAssyParametersWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void cmbAssyParametersWidget::initUI()
{
}

//-----------------------------------------------------------------------------
void cmbAssyParametersWidget::setAssembly(cmbNucAssembly *assyObj)
{
  if(this->Assembly == assyObj)
    {
    return;
    }
  this->Assembly = assyObj;
  this->onReset();
}
// Invoked when Apply button clicked
//-----------------------------------------------------------------------------
void cmbAssyParametersWidget::onApply()
{
  if(this->Assembly == NULL)
    {
    return;
    }
  this->applyToAssembly(this->Assembly);
}
// Invoked when Reset button clicked
//-----------------------------------------------------------------------------
void cmbAssyParametersWidget::onReset()
{
  if(this->Assembly == NULL)
    {
    return;
    }

  this->resetAssembly(this->Assembly);
}

//Helpers
namespace
{

void convert(QString qw, std::string & result)
{
  result = qw.toStdString();
}

void convert(QString qw, double & result)
{
  if(qw.isEmpty())
  {
    result = ASSY_NOT_SET_VALUE;
  }
  bool ok;
  double previous = result;
  result = qw.toDouble(&ok);
  if(!ok)
  {
    result = previous;
  }
}

void convert(QString qw, int & result)
{
  bool ok;
  int previous = result;
  result = qw.toInt(&ok);
  if(!ok)
  {
    result = previous;
  }
}

void convert(bool qw, bool & result)
{
  result = qw;
}

void setValue(std::string &to, QComboBox * from)
{
  QString tmp = from->currentText();
  convert(tmp, to);
}

void setValue(QComboBox * to, std::string &from)
{
  QString tmp(from.c_str());
  tmp = tmp.simplified();
  qDebug() << "Settign combo box: " << tmp;
  to->setCurrentIndex(to->findText(tmp, Qt::MatchFixedString));
}

void setValue(double &to, QLineEdit * from)
{
  if(from->text().isEmpty())
  {
    to = ASSY_NOT_SET_VALUE;
    return;
  }
  convert(from->text(), to);
}

void setValue(int &to, QLineEdit * from)
{
  if(from->text().isEmpty())
  {
    to = ASSY_NOT_SET_VALUE;
    return;
  }
  convert(from->text(), to);
}

void setValue(std::string &to, QLineEdit * from)
{
  if(from->text().isEmpty())
  {
    to = ASSY_NOT_SET_KEY;
    return;
  }
  convert(from->text(), to);
}

void setValue(QLineEdit * to, std::string &from)
{
  if(from == ASSY_NOT_SET_KEY)
  {
    to->setText("");
    return;
  }
  QString tmp(from.c_str());
  to->setText(tmp);
}

void setValue(QLineEdit * to, double &from)
{
  if(from == ASSY_NOT_SET_VALUE)
  {
    to->setText("");
    return;
  }
  QString tmp = QString::number(from);
  to->setText(tmp);
}

void setValue(QLineEdit * to, int &from)
{
  if(from == ASSY_NOT_SET_VALUE)
  {
    to->setText("");
    return;
  }
  QString tmp = QString::number(from);
  to->setText(tmp);
}

void setValue(bool &to, QCheckBox * from)
{
  to = from->isChecked();
}

void setValue(QCheckBox * to, bool &from)
{
  to->setChecked(from);
}
}

#define EASY_ASSY_PARAMS_MACRO()\
  FUN(TetMeshSize) \
  FUN(RadialMeshSize) \
  FUN(AxialMeshSize) \
  FUN(NeumannSet_StartId) \
  FUN(MaterialSet_StartId) \
  FUN(EdgeInterval)\
  FUN(MergeTolerance) \
  FUN(CreateFiles) \
  FUN(RotateXYZ) \
  FUN(RotateAngle) \
  FUN(CenterXYZ) \
  FUN(Info) \
  FUN(SectionXYZ) \
  FUN(SectionOffset) \
  FUN(StartPinId) \
  FUN(CellMaterial) \
  FUN(CreateMatFiles) \
  FUN(Save_Exodus) \
  FUN(List_NeumannSet_StartId) \
  FUN(List_MaterialSet_StartId) \
  FUN(NumSuperBlocks) \
  FUN(SuperBlocks) \
  FUN(MeshType)



//-----------------------------------------------------------------------------
void cmbAssyParametersWidget::applyToAssembly(cmbNucAssembly* assy)
{
  qDebug() << QString::number(reinterpret_cast<unsigned long>(assy),16) << " " << assy->label.c_str();
  cmbAssyParameters* parameters = assy->GetParameters();
#define FUN(X) setValue(parameters->X, this->Internal->X);
  EASY_ASSY_PARAMS_MACRO()
#undef FUN
  std::stringstream ss(Internal->Unknown->toPlainText().toStdString().c_str());
  std::string line;
  parameters->UnknownParams.clear();
  while( std::getline(ss, line))
  {
    qDebug() << line.c_str();
    parameters->UnknownParams.push_back(line);
    line.clear();
  }
  qDebug() << "extracted: " << parameters->UnknownParams.size();
}

//-----------------------------------------------------------------------------
void cmbAssyParametersWidget::resetAssembly(cmbNucAssembly* assy)
{
  qDebug() << QString::number(reinterpret_cast<unsigned long>(assy),16) << " " << assy->label.c_str();
  cmbAssyParameters* parameters = assy->GetParameters();
#define FUN(X) setValue(this->Internal->X, parameters->X);
  EASY_ASSY_PARAMS_MACRO()
#undef FUN
  qDebug() << "Number of knowns: " << parameters->UnknownParams.size();
  std::string unknowns;
  for(unsigned int i = 0; i < parameters->UnknownParams.size(); ++i)
  {
    unknowns += parameters->UnknownParams[i] + "\n";
  }
  Internal->Unknown->setPlainText(QString::fromStdString(unknowns));
}
