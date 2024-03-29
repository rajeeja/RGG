#include "cmbNucMaterialColors.h"

#include <QSettings>
#include <QFileInfo>
#include <QComboBox>
#include <QMetaType>
#include <QDebug>
#include <QTreeWidget>

#include "cmbNucAssembly.h"
#include "cmbNucPartDefinition.h"
#include "cmbNucMaterial.h"
#include "cmbNucMaterialTreeItem.h"

#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkMath.h"

#include <cassert>

// QSettings material Group name
#define GROUP_MATERIAL "MaterialColors"

#define numInitialNewMaterialColors 42

static int initialNewMaterialColors[][3] = 
{
  {66,146,198},
  {241,105,19},
  {65,171,93},
  {239,59,44},
  {128,125,186},
  {115,115,115},  
  {198,219,239},
  {253,208,162},
  {199,233,192},
  {252,187,161},
  {218,218,235},
  {217,217,217}, 
  {8,81,156},
  {166,54,3},
  {0,109,44},
  {165,15,21},
  {84,39,143},
  {37,37,37},  
  {158,202,225},
  {253,174,107},
  {161,217,155},
  {252,146,114},
  {188,189,220},
  {189,189,189},  
  {33,113,181},
  {217,72,1},
  {35,139,69},
  {203,24,29},
  {106,81,163},
  {82,82,82},  
  {107,174,214},
  {253,141,60},
  {116,196,118},
  {251,106,74},
  {158,154,200},
  {150,150,150}, 
  {8,48,10},
  {127,39,4},
  {0,68,27},
  {103,0,13},
  {63,0,125},
  {0,0,0}
};

//----------------------------------------------------------------------------
cmbNucMaterialColors* cmbNucMaterialColors::Instance = 0;

cmbNucMaterialColors* cmbNucMaterialColors::instance()
{
  return cmbNucMaterialColors::Instance;
}

//-----------------------------------------------------------------------------
cmbNucMaterialColors::cmbNucMaterialColors(bool reset_instance)
  : MaterialTree(NULL), Ulimit(0.9), Llimit(0.1), numNewMaterials(0), newID(0)
{
  showMode = 0;
  UnknownMaterial = new cmbNucMaterial("UnknownMaterial", "Unknown",
                                       QColor::fromRgbF(1.0,1.0,1.0));
  connect(UnknownMaterial, SIGNAL(nameHasChanged(QString, QPointer<cmbNucMaterial>)),
          this, SLOT(UnknownRename(QString)));
  connect(UnknownMaterial, SIGNAL(labelHasChanged(QString, QPointer<cmbNucMaterial>)),
          this, SLOT(UnknownRelabel(QString)));
  connect(UnknownMaterial, SIGNAL(colorChanged()), this, SIGNAL(materialColorChanged()));
  connect(UnknownMaterial, SIGNAL(useChanged()), this, SLOT(testShow()));
  if (!cmbNucMaterialColors::Instance || reset_instance)
    {
    cmbNucMaterialColors::Instance = this;
    }
}

cmbNucMaterialColors::~cmbNucMaterialColors()
{
  this->clear();
  if (cmbNucMaterialColors::Instance == this)
    {
    cmbNucMaterialColors::Instance = 0;
    }
  delete UnknownMaterial;
}

//-----------------------------------------------------------------------------
bool cmbNucMaterialColors::OpenFile(const QString& name)
{
  QFileInfo finfo(name);
  if(!finfo.exists())
    {
    return false;
    }

  QSettings settings(name, QSettings::IniFormat);
  if(!settings.childGroups().contains(GROUP_MATERIAL))
    {
    return false;
    }

  settings.beginGroup(GROUP_MATERIAL);
  QString settingKey(GROUP_MATERIAL), strcolor;
  settingKey.append("/");
  QStringList list1;
  foreach(QString mat, settings.childKeys())
    {
    strcolor = settings.value(mat).toString();
    list1 = strcolor.split(",");
    if(list1.size() == 4)
      {
      this->AddMaterial(mat,
        list1.value(0).toDouble(),
        list1.value(1).toDouble(),
        list1.value(2).toDouble(),
        list1.value(3).toDouble());
      }
    else if(list1.size() == 5) // with label
      {
      this->AddMaterial(mat, list1.value(0),
        list1.value(1).toDouble(),
        list1.value(2).toDouble(),
        list1.value(3).toDouble(),
        list1.value(4).toDouble());
      }
    }

  settings.endGroup();
  return true;
}

//-----------------------------------------------------------------------------
void cmbNucMaterialColors::SaveToFile(const QString& name)
{
  QSettings settings(name, QSettings::IniFormat);
  QString settingKey(GROUP_MATERIAL), strcolor;
  settingKey.append("/");
  QColor color;
  foreach(QPointer<cmbNucMaterial> mat, NameToMaterial.values())
    {
    color = mat->getColor();
    strcolor = QString("%1, %2, %3, %4, %5").arg(
      mat->getLabel()).
      arg(color.redF()).arg(color.greenF()).arg(
      color.blueF()).arg(color.alphaF());
    settings.setValue(settingKey+mat->getName(),strcolor);
    }
}

//-----------------------------------------------------------------------------
void cmbNucMaterialColors::clear()
{
  foreach(QPointer<cmbNucMaterial> value, this->NameToMaterial)
  {
    delete value;
  }
  this->NameToMaterial.clear();
  this->LabelToMaterial.clear();
  this->Llimit = 0.1;
  this->Ulimit = 0.9;
  this->numNewMaterials = 0;
  this->newID = 0;
}

//-----------------------------------------------------------------------------
QPointer<cmbNucMaterial>
cmbNucMaterialColors
::getMaterialByName(QString const& name) const
{
  Material_Map::const_iterator iter =
      this->find(name, this->NameToMaterial);
  if(iter != this->NameToMaterial.end()) return iter.value();
  return UnknownMaterial;
}

//-----------------------------------------------------------------------------
QPointer<cmbNucMaterial>
cmbNucMaterialColors
::getMaterialByLabel(QString const& label) const
{
  Material_Map::const_iterator iter =
      this->find(label, this->LabelToMaterial);
  if(iter != this->LabelToMaterial.end()) return iter.value();
  return UnknownMaterial;
}

//-----------------------------------------------------------------------------
void cmbNucMaterialColors
::sendMaterialFromName(QString const& name)
{
  QPointer<cmbNucMaterial> result = this->getMaterialByName(name);
  emit materialSelected(result);
}

//-----------------------------------------------------------------------------
void cmbNucMaterialColors
::sendMaterialFromLabel(QString const& label)
{
  QPointer<cmbNucMaterial> result = this->getMaterialByLabel(label);
  emit materialSelected(result);
}

//-----------------------------------------------------------------------------
QPointer<cmbNucMaterial> cmbNucMaterialColors::getUnknownMaterial() const
{
  return UnknownMaterial;
}

//-----------------------------------------------------------------------------
QPointer<cmbNucMaterial>
cmbNucMaterialColors::AddMaterial(const QString& name, const QString& label,
                                  const QColor& color)
{
  return this->AddMaterial(name, label, color.redF(), color.greenF(),
                           color.blueF(), color.alphaF());
}

//-----------------------------------------------------------------------------
QPointer<cmbNucMaterial>
cmbNucMaterialColors::AddMaterial(const QString& name, const QString& label,
                                  double r, double g, double b, double a)
{
  if(this->nameUsed(name) || this->labelUsed(label))
  {
    return NULL;
  }
  QPointer< cmbNucMaterial > mat = new cmbNucMaterial( name, label,
                                                       QColor::fromRgbF(r, g,
                                                                        b, a) );

  this->insert(name, mat, this->NameToMaterial);
  this->insert(label, mat, this->LabelToMaterial);
  assert(this->nameUsed(name));
  assert(this->labelUsed(label));
  connect(mat, SIGNAL(nameHasChanged(QString, QPointer<cmbNucMaterial>)),
          this, SLOT(testAndRename(QString, QPointer<cmbNucMaterial>)));
  connect(mat, SIGNAL(labelHasChanged(QString, QPointer<cmbNucMaterial>)),
          this, SLOT(testAndRelabel(QString, QPointer<cmbNucMaterial>)));
  connect(mat, SIGNAL(colorChanged()), this, SIGNAL(materialColorChanged()));
  connect(mat, SIGNAL(useChanged()), this, SLOT(testShow()));
  return mat;
}

//-----------------------------------------------------------------------------
QPointer<cmbNucMaterial>
cmbNucMaterialColors::AddMaterial( const QString& name, double r,
                                   double g, double b, double a)
{
  return this->AddMaterial(name, name, r, g, b, a);
}

//-----------------------------------------------------------------------------
void cmbNucMaterialColors::RemoveMaterialByName(const QString& name)
{
  Material_Map::iterator it = this->find(name, this->NameToMaterial);
  if(it != this->NameToMaterial.end())
    {
      bool is_used = false;
      if(it.value())
        {
        QString label = it.value()->getLabel();
        is_used = it.value()->isUsed();
        delete it.value();
        RemoveMaterialByLabel(label);
        }
    this->NameToMaterial.erase(it);
    if(is_used)
      {
      emit materialColorChanged();
      }
    }
}

//-----------------------------------------------------------------------------
bool cmbNucMaterialColors::nameUsed( const QString& name ) const
{
  return name.toLower() == UnknownMaterial->getName().toLower() ||
         this->find(name, this->NameToMaterial) != this->NameToMaterial.end();
}

//-----------------------------------------------------------------------------
bool cmbNucMaterialColors::labelUsed( const QString& label ) const
{
  return label.toLower() == UnknownMaterial->getLabel().toLower() ||
         this->find(label, this->LabelToMaterial) !=
         this->LabelToMaterial.end();
}

//-----------------------------------------------------------------------------
void cmbNucMaterialColors::RemoveMaterialByLabel(const QString& name)
{
  Material_Map::iterator it = this->find(name, this->LabelToMaterial);
  if(it != this->LabelToMaterial.end())
  {
    if(it.value())
    {
      QString tmpName = it.value()->getName();
      delete it.value();
      RemoveMaterialByName(tmpName);
    }
    this->LabelToMaterial.erase(it);
  }
}


//-----------------------------------------------------------------------------
void cmbNucMaterialColors::SetBlockMaterialColor(
  vtkCompositeDataDisplayAttributes *attributes, unsigned int flatIdx,
  QPointer<cmbNucMaterial> material)
{
  QColor bColor = material->getColor();
  bool visible = material->isVisible();
  double color[] = { bColor.redF(), bColor.greenF(), bColor.blueF() };
  attributes->SetBlockColor(flatIdx, color);
  attributes->SetBlockOpacity(flatIdx, bColor.alphaF());
  attributes->SetBlockVisibility(flatIdx, visible);
}

//----------------------------------------------------------------------------
void cmbNucMaterialColors::CalcRGB(double &r, double &g, double &b)
{
  double l;
  while(1)
    {
    r = vtkMath::Random(0.0, 1.0);
    g = vtkMath::Random(0.0, 1.0);
    b = vtkMath::Random(0.0, 1.0);

    l = (0.11 *b) + (0.3 *r) + (0.59*g);
    if ((l >= this->Llimit) && ( l <= this->Ulimit))
      {
      return;
      }
    }
}

//----------------------------------------------------------------------------
void cmbNucMaterialColors::testAndRename(QString oldn,
                                         QPointer<cmbNucMaterial> material)
{
  QString newN = material->getName();
  if(this->nameUsed(newN))
  {
    material->revertName(oldn);
    if(MaterialTree) MaterialTree->update();
    return;
  }
  Material_Map::iterator it = this->find(oldn, this->NameToMaterial);
  this->NameToMaterial.erase(it);
  this->insert(newN, material, this->NameToMaterial);
  material->emitMaterialChange();
}

//----------------------------------------------------------------------------
void cmbNucMaterialColors::testAndRelabel(QString oldl,
                                          QPointer<cmbNucMaterial> material)
{
  QString newL = material->getLabel();
  if(this->labelUsed(newL))
  {
    material->revertLabel(oldl);
    if(MaterialTree) MaterialTree->update();
    return;
  }
  Material_Map::iterator it = this->find( oldl, this->LabelToMaterial);
  this->LabelToMaterial.erase(it);
  this->insert(newL, material, this->LabelToMaterial);
  material->emitMaterialChange();
}

//----------------------------------------------------------------------------
void cmbNucMaterialColors::UnknownRename(QString oldn)
{
  UnknownMaterial->revertName(oldn);
  if(MaterialTree) MaterialTree->update();
}

//----------------------------------------------------------------------------
void cmbNucMaterialColors::UnknownRelabel(QString oldl)
{
  UnknownMaterial->revertLabel(oldl);
  if(MaterialTree) MaterialTree->update();
}

//----------------------------------------------------------------------------
QPointer<cmbNucMaterial>
cmbNucMaterialColors::AddMaterial(const QString& name, const QString& label)
{
  QColor color;
  if (this->numNewMaterials < numInitialNewMaterialColors)
    {
    color = QColor( initialNewMaterialColors[this->numNewMaterials][0],
                    initialNewMaterialColors[this->numNewMaterials][1],
                    initialNewMaterialColors[this->numNewMaterials][2]);
    ++this->numNewMaterials;
    }
  else
    {
    double r, g, b;
    this->CalcRGB(r, g, b);
    color = QColor::fromRgbF(r, g, b, 1.0);
    }

  return this->AddMaterial(name, label, color);
}

//----------------------------------------------------------------------------
void cmbNucMaterialColors::setUp(QComboBox *comboBox) const
{
  comboBox->clear();
  comboBox->addItem(UnknownMaterial->getName());
  foreach(QPointer<cmbNucMaterial> mat, NameToMaterial.values())
  {
    comboBox->addItem(mat->getName());
  }
}

//----------------------------------------------------------------------------
void cmbNucMaterialColors::selectIndex(QComboBox *comboBox,
                                       QPointer<cmbNucMaterial> mat) const
{
  int idx = -1;
  for(int j = 0; j < comboBox->count() && mat != NULL; j++)
  {
    if(comboBox->itemText(j) == mat->getName())
    {
      idx = j;
      break;
    }
  }
  if(idx == -1)  comboBox->setCurrentIndex(0); //unknown
  else           comboBox->setCurrentIndex(idx);
}

//----------------------------------------------------------------------------
QPointer<cmbNucMaterial>
cmbNucMaterialColors::getMaterial(QComboBox *comboBox) const
{
  QPointer<cmbNucMaterial> result =
      this->getMaterialByName(comboBox->currentText());
  if(result == NULL)
  {
    return UnknownMaterial;
  }
  return result;
}

//----------------------------------------------------------------------------
void cmbNucMaterialColors::buildTree(QTreeWidget * tree)
{
  this->MaterialTree = tree;
  this->MaterialTree->clear();
  UnknownMaterialTreeItem = this->addToTree(UnknownMaterial);
  UnknownMaterialTreeItem->setSelected(true);
  foreach(QPointer<cmbNucMaterial> mat, NameToMaterial.values())
    {
    this->addToTree(mat);
    }
}

//----------------------------------------------------------------------------
cmbNucMaterialTreeItem *
cmbNucMaterialColors::addToTree(QPointer<cmbNucMaterial> mat)
{
  cmbNucMaterialTreeItem* mNode =
      new cmbNucMaterialTreeItem(MaterialTree->invisibleRootItem(),
                                 mat);
  connect(this, SIGNAL(showModeSig(int)),
          mNode->getConnection(), SLOT(show(int)));
  return mNode;
}

//----------------------------------------------------------------------------
QString
cmbNucMaterialColors
::generateString(QString prefix, Material_Map const& mat)
{
  QString matname;
  while(true)
  {
    matname = prefix + QString::number(newID);
    if(this->find(matname, mat) == mat.end()) break;
    newID++;
  }
  return matname;
}

//----------------------------------------------------------------------------
void cmbNucMaterialColors::CreateNewMaterial()
{
  QPointer<cmbNucMaterial> mat =
      this->AddMaterial( generateString("MATERIAL_", NameToMaterial),
                         generateString("LABEL_", LabelToMaterial) );
  if(mat == NULL)
  {
    qDebug() << "ERROR creating new material should not be null";
  }
  cmbNucMaterialTreeItem *node = this->addToTree(mat);
  QList<QTreeWidgetItem*> selItems = MaterialTree->selectedItems();
  if(selItems.count()>0)
  {
    selItems[0]->setSelected(false);
  }
  node->setSelected(true);
}

void cmbNucMaterialColors::deleteSelected()
{
  if(MaterialTree == NULL) return;
  foreach(QTreeWidgetItem* selitem, MaterialTree->selectedItems())
  {
    cmbNucMaterialTreeItem * cnmti = dynamic_cast<cmbNucMaterialTreeItem *>(selitem);
    if(cnmti == NULL) continue;
    if(cnmti->getMaterial() == UnknownMaterial) continue;
    disconnect(this, SIGNAL(showModeSig(int)),
               cnmti->getConnection(), SLOT(show(int)));
    this->RemoveMaterialByName(cnmti->getMaterial()->getName());
    UnknownMaterialTreeItem->getConnection()->show(showMode);
    delete cnmti;
  }
}

//------------------------------------------------------------------------------
void cmbNucMaterialColors::controlShow(int sc)
{
  showMode = sc;
  this->testShow();
}

void cmbNucMaterialColors::testShow()
{
  emit showModeSig(this->showMode);
}

//------------------------------------------------------------------------------
void cmbNucMaterialColors
::insert( QString key, QPointer<cmbNucMaterial> mat, Material_Map & map) const
{
  map.insert(key.toLower(), mat);
}

cmbNucMaterialColors::Material_Map::iterator
cmbNucMaterialColors
::find( QString key, Material_Map & map)
{
  return map.find(key.toLower());
}

//------------------------------------------------------------------------------
cmbNucMaterialColors::Material_Map::const_iterator
cmbNucMaterialColors
::find(QString key, Material_Map const& map) const
{
  return map.find(key.toLower());
}

//------------------------------------------------------------------------------
void cmbNucMaterialColors
::clearDisplayed()
{
  UnknownMaterial->clearDisplayed();
  foreach(QPointer<cmbNucMaterial> mat, NameToMaterial.values())
  {
    mat->clearDisplayed();
  }
}

//------------------------------------------------------------------------------
std::vector< QPointer<cmbNucMaterial> >
cmbNucMaterialColors
::getInvisibleMaterials()
{
  std::vector< QPointer<cmbNucMaterial> > result;
  if(!UnknownMaterial->isVisible())
  {
    result.push_back(UnknownMaterial);
  }
  foreach(QPointer<cmbNucMaterial> mat, NameToMaterial.values())
  {
    if(!mat->isVisible())
    {
      result.push_back(mat);
    }
  }
  return result;
}

QString
cmbNucMaterialColors
::createMaterialLabel(const char * name)
{
  if(name == NULL) return QString();
  QString result(name);
  if(result.endsWith("_top_ss"))
  {
    return result.remove("_top_ss");
  }
  if(result.endsWith("_bot_ss"))
  {
    return result.remove("_bot_ss");
  }
  if(result.endsWith("_side_ss"))
  {
    return result.remove("_side_ss");
  }
  if(result.endsWith("_side1_ss"))
  {
    return result.remove("_side1_ss");
  }
  if(result.endsWith("_side2_ss"))
  {
    return result.remove("_side2_ss");
  }
  if(result.endsWith("_side3_ss"))
  {
    return result.remove("_side3_ss");
  }
  if(result.endsWith("_side4_ss"))
  {
    return result.remove("_side4_ss");
  }
  if(result.endsWith("_side5_ss"))
  {
    return result.remove("_side5_ss");
  }
  if(result.endsWith("_side6_ss"))
  {
    return result.remove("_side6_ss");
  }
  return QString(&(name[2]));
}
