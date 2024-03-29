
#include "cmbNucInputListWidget.h"
#include "ui_qInputListWidget.h"

#include "cmbNucAssembly.h"
#include "cmbNucCore.h"
#include "cmbNucPartsTreeItem.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucDefaults.h"

#include <QLabel>
#include <QPointer>
#include <QTreeWidget>
#include <QAction>
#include <QBrush>
#include <QFileDialog>
#include <QColorDialog>
#include <QHeaderView>
#include <QItemDelegate>
#include <QMenu>
#include <QHeaderView>
#include <QPainter>
#include <QTreeWidgetItem>

class PartsItemDelegate: public QItemDelegate
{
public:
  PartsItemDelegate(QObject* pParent = 0) : QItemDelegate(pParent)
  {
  }

  void paint(QPainter* pPainter, const QStyleOptionViewItem& rOption, const QModelIndex& rIndex) const
  {
    QStyleOptionViewItemV4 ViewOption(rOption);

    QString oldText;

    QColor ItemForegroundColor = rIndex.data(Qt::ForegroundRole).value<QColor>();
    if (ItemForegroundColor.isValid())
    {
      if (ItemForegroundColor != rOption.palette.color(QPalette::WindowText))
      {
        ViewOption.palette.setColor(QPalette::Highlight, ItemForegroundColor);
      }
    }
    QItemDelegate::paint(pPainter, ViewOption, rIndex);
  }

  QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const
  {
    QSize tmp = QItemDelegate::sizeHint(option, index);
    tmp.setHeight(15);
    return tmp;
  }
};

class MaterialItemDelegate: public QItemDelegate
{
public:
  MaterialItemDelegate(QObject* pParent = 0) : QItemDelegate(pParent)
  {
  }

  void paint(QPainter* pPainter, const QStyleOptionViewItem& rOption, const QModelIndex& rIndex) const
  {
    if(rOption.state & QStyle::State_Selected)
    {

      if(rIndex.column() == 3)
      {
        QStyleOptionViewItem unSelect = rOption;
        unSelect.state &= (~QStyle::State_Selected);
        QItemDelegate::paint(pPainter,unSelect,rIndex);
        return;
      }
    }
    QItemDelegate::paint(pPainter,rOption,rIndex);
  }
  QTreeWidget* treeWidget;
};


class cmbNucInputListWidgetInternal :
  public Ui::InputListWidget
{
public:
  cmbNucInputListWidgetInternal()
  {
    this->RootCoreNode = NULL;
    TreeItemDelegate = new PartsItemDelegate();
    MaterialDelegate = new MaterialItemDelegate();
  }
  virtual ~cmbNucInputListWidgetInternal()
  {
    delete Action_NewAssembly;
    delete Action_NewPin;
    delete Action_DeletePart;
    delete TreeItemDelegate;
    delete MaterialDelegate;
  }
  void initActions()
    {
    this->Action_NewAssembly = new QAction("Create Assembly", this->PartsList);
    this->Action_NewPin = new QAction("Create Pin", this->PartsList);
    this->Action_DeletePart = new QAction("Delete Selected", this->PartsList);
    this->PartsList->addAction(this->Action_NewAssembly);
    this->PartsList->addAction(this->Action_NewPin);
    this->PartsList->addAction(this->Action_DeletePart);
    }

  QPointer<QAction> Action_NewAssembly;
  QPointer<QAction> Action_NewPin;
  QPointer<QAction> Action_DeletePart;

  cmbNucPartsTreeItem* RootCoreNode;

  PartsItemDelegate * TreeItemDelegate;
  MaterialItemDelegate * MaterialDelegate;
};

//-----------------------------------------------------------------------------
cmbNucInputListWidget::cmbNucInputListWidget(QWidget* _p)
  : QWidget(_p)
{
  this->NuclearCore = NULL;
  this->Internal = new cmbNucInputListWidgetInternal;
  this->Internal->setupUi(this);
  this->Internal->initActions();

  this->modelIsLoaded(false);
  this->Internal->tabInputs->setTabEnabled(2, false);

  this->Internal->PartsList->setAlternatingRowColors(true);
  this->Internal->PartsList->header()->setResizeMode(QHeaderView::ResizeToContents);

  // set up the UI trees
  QTreeWidget* treeWidget = this->Internal->PartsList;
  treeWidget->setItemDelegate(this->Internal->TreeItemDelegate);

  this->Internal->MaterialTree->setItemDelegate(this->Internal->MaterialDelegate);
  this->Internal->MaterialDelegate->treeWidget = this->Internal->MaterialTree;

  // context menu for parts tree
  QObject::connect(this->Internal->Action_NewAssembly, SIGNAL(triggered()),
    this, SLOT(onNewAssembly()));
  QObject::connect(this->Internal->Action_NewPin, SIGNAL(triggered()),
    this, SLOT(onNewPin()));
  QObject::connect(this->Internal->Action_DeletePart, SIGNAL(triggered()),
    this, SLOT(onRemoveSelectedPart()));

  QObject::connect(this->Internal->newMaterial, SIGNAL(clicked()),
                   cmbNucMaterialColors::instance(), SLOT(CreateNewMaterial()));
  QObject::connect(this->Internal->delMaterial, SIGNAL(clicked()),
                   cmbNucMaterialColors::instance(), SLOT(deleteSelected()));
  QObject::connect(this->Internal->hideLabels, SIGNAL(clicked(bool)),
                   this,                       SLOT(hideLabels(bool)));
  QObject::connect(this->Internal->importMaterial, SIGNAL(clicked()),
                   this, SLOT(onImportMaterial()));
  QObject::connect(this->Internal->saveMaterial, SIGNAL(clicked()),
                   this, SLOT(onSaveMaterial()));
  QObject::connect(this->Internal->materialDisplayed, SIGNAL(currentIndexChanged(int)),
                   cmbNucMaterialColors::instance(), SLOT(controlShow(int)));

  QObject::connect(this->Internal->PartsList, SIGNAL(itemSelectionChanged()),
                   this, SLOT(onPartsSelectionChanged()), Qt::QueuedConnection);

  QObject::connect(this->Internal->MaterialTree, SIGNAL(itemClicked (QTreeWidgetItem*, int)),
                   this, SLOT(onMaterialClicked(QTreeWidgetItem*, int)), Qt::QueuedConnection);

  QObject::connect(this->Internal->MeshComponents, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem * )),
                   this, SIGNAL(subMeshSelected(QTreeWidgetItem *)), Qt::QueuedConnection);

  QObject::connect(this->Internal->MeshComponents, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
                   this, SIGNAL(meshValueChanged(QTreeWidgetItem*)));

  QObject::connect(this->Internal->tabInputs, SIGNAL(currentChanged(int)),
                   this, SLOT(onTabChanged(int)));

  QObject::connect(this, SIGNAL(deleteAssembly(QTreeWidgetItem*)),
                   this, SLOT(onDeleteAssembly(QTreeWidgetItem*)));

  QObject::connect(this->Internal->color_control, SIGNAL(currentIndexChanged(int)),
                   this, SIGNAL(sendColorControl(int)));
  QObject::connect(this->Internal->edge_control, SIGNAL(clicked(bool)),
                   this, SIGNAL(sendEdgeControl(bool)));

  QObject::connect(this->Internal->meshMajorSet, SIGNAL(currentIndexChanged(int)),
                   this, SIGNAL(majorMeshSelection(int)));

  QObject::connect(this->Internal->resetCamera, SIGNAL(clicked()), this, SIGNAL(resetMeshCamera()));

  this->initUI();
}

//-----------------------------------------------------------------------------
cmbNucInputListWidget::~cmbNucInputListWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::setPartOptions(QMenu * qm) const
{
  qm->clear();
  qm->addAction(this->Internal->Action_NewAssembly);
  qm->addAction(this->Internal->Action_NewPin);
  qm->addAction(this->Internal->Action_DeletePart);
}

void cmbNucInputListWidget::clear()
{
  this->clearTable();
  this->setCore(NULL);
  this->setEnabled(false);
  this->initPartsTree();
  if(this->Internal->RootCoreNode)
  {
    this->Internal->RootCoreNode = NULL;
  }
  this->modelIsLoaded(false);
  this->Internal->tabInputs->setTabEnabled(2, false);
}

bool cmbNucInputListWidget::onlyMeshLoaded()
{
  return this->isEnabled() && !this->Internal->tabInputs->isTabEnabled(0);
}

void cmbNucInputListWidget::meshIsLoaded(bool v)
{
  if(this->onlyMeshLoaded() || v) this->setEnabled(v);
  if(this->onlyMeshLoaded() || v) this->Internal->tabInputs->setTabEnabled(1, v);
  this->Internal->tabInputs->setTabEnabled(2, v);
  if(v) this->initMaterialsTree();
  if(v && onlyMeshLoaded()) this->Internal->tabInputs->setCurrentIndex(2);
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::setCore(cmbNucCore* core)
{
  if(this->NuclearCore == core)
    {
    return;
    }
  if(this->NuclearCore!=NULL)
  {
    disconnect(this->NuclearCore->GetConnection(), SIGNAL(dataChangedSig()),
               this, SLOT(repaintList()));
  }
  this->NuclearCore = core;
  if(this->NuclearCore!=NULL)
  {
    connect( this->NuclearCore->GetConnection(), SIGNAL(dataChangedSig()),
             this, SLOT(repaintList()) );
  }
}

//-----------------------------------------------------------------------------
AssyPartObj* cmbNucInputListWidget::getSelectedPart()
{
  cmbNucPartsTreeItem* selItem = this->getSelectedItem(
    this->Internal->PartsList);
  if(!selItem || !selItem->getPartObject())
    {
    return NULL;
    }
  return selItem->getPartObject();
}
//-----------------------------------------------------------------------------
cmbNucPartsTreeItem* cmbNucInputListWidget::getSelectedPartNode()
{
  return this->getSelectedItem(this->Internal->PartsList);
}

//-----------------------------------------------------------------------------
cmbNucPartsTreeItem* cmbNucInputListWidget::getCurrentAssemblyNode()
{
  cmbNucPartsTreeItem* selItem = this->getSelectedItem(
    this->Internal->PartsList);
  if(!selItem || !selItem->getPartObject())
    {
    return NULL;
    }

  AssyPartObj* selObj = selItem->getPartObject();
  QTreeWidgetItem* pItem=NULL;
  enumNucPartsType selType = selObj->GetType();
  switch(selType)
    {
    case CMBNUC_ASSY_LATTICE:
    case CMBNUC_ASSY_DUCTCELL:
    case CMBNUC_ASSY_PINCELL:
      pItem = selItem->parent();
      break;
    case CMBNUC_ASSY_FRUSTUM_PIN:
    case CMBNUC_ASSY_CYLINDER_PIN:
    case CMBNUC_ASSY_DUCT:
      pItem = selItem->parent()->parent();
      break;
    case CMBNUC_ASSEMBLY:
      pItem = selItem;
      break;
    case CMBNUC_CORE:
      if(selItem->childCount()==1)
        {
        pItem = selItem->child(0);
        }
      break;
    default:
      break;
    }
  return pItem ? dynamic_cast<cmbNucPartsTreeItem*>(pItem) : NULL;
}

//-----------------------------------------------------------------------------
cmbNucAssembly* cmbNucInputListWidget::getCurrentAssembly()
{
  cmbNucPartsTreeItem* assyItem = this->getCurrentAssemblyNode();
  if(assyItem)
    {
    return dynamic_cast<cmbNucAssembly*>(assyItem->getPartObject());
    }
  return NULL;
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::initUI()
{
  this->setActionsEnabled(false);
  if(this->Internal->RootCoreNode)
    {
    delete this->Internal->RootCoreNode;
    this->Internal->RootCoreNode = NULL;
    }
  this->initPartsTree();
  this->initMaterialsTree();
}
//----------------------------------------------------------------------------
void cmbNucInputListWidget::onTabChanged(int currentTab)
{
  if(currentTab == 1) // materials
    {
      int i = this->Internal->materialDisplayed->currentIndex();
      cmbNucMaterialColors::instance()->controlShow(i);
    }
 }
//----------------------------------------------------------------------------
void cmbNucInputListWidget::setActionsEnabled(bool val)
{
  this->Internal->Action_NewAssembly->setEnabled(val);
  this->Internal->Action_NewPin->setEnabled(val);
  this->Internal->Action_DeletePart->setEnabled(val);
}
//----------------------------------------------------------------------------
void cmbNucInputListWidget::onNewAssembly()
{
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  double r,g,b;
  matColorMap->CalcRGB(r,g,b);

  this->setEnabled(1);
  cmbNucAssembly* assembly = new cmbNucAssembly;
  assembly->SetLegendColor(QColor::fromRgbF(r,g,b));
  if(this->NuclearCore->IsHexType())
    {
    assembly->setGeometryLabel("Hexagonal");
    assembly->getLattice().SetDimensions(1, 0, true);
    }
  else
    {
    assembly->setGeometryLabel("Rectangular");
    }
  assembly->label = QString("Assy").append(
    QString::number(this->NuclearCore->GetNumberOfAssemblies()+1)).toStdString();

  this->NuclearCore->AddAssembly(assembly);
  assembly->computeDefaults();
  assembly->setFromDefaults(this->NuclearCore->GetDefaults());

  double thickX, thickY, h;
  assembly->getDefaults()->getDuctThickness(thickX,thickY);
  assembly->getDefaults()->getHeight(h);
  Duct * newduct = new Duct(h, thickX, thickY);

  assembly->AssyDuct.AddDuct(newduct);

  this->initCoreRootNode();
  this->updateWithAssembly(assembly);
  emit assembliesModified(this->NuclearCore);
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::onNewPin()
{
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  double rgb[3];
  matColorMap->CalcRGB(rgb[0],rgb[1],rgb[2]);

  cmbNucAssembly * assy = this->getCurrentAssembly();
  double px, py, h, r;
  if(!assy->getDefaults()->getPitch(px,py))
  {
    assy->calculatePitch(px,py);
  }
  assy->calculatePitch(px, py);
  assy->getDefaults()->getHeight(h);
  assy->calculateRadius(r);
  PinCell* newpin = new PinCell(px,py);
  newpin->SetLegendColor(QColor::fromRgbF(rgb[0],rgb[1],rgb[2]));
  newpin->AddCylinder(new Cylinder(r, 0, h));
  QString pinname = QString("PinCell").append(
    QString::number(this->getCurrentAssembly()->GetNumberOfPinCells()+1));
  newpin->name = newpin->label = pinname.toStdString();
  assy->AddPinCell(newpin);
  QTreeWidgetItem* partsRoot = this->getCurrentAssemblyNode();
  if(!partsRoot)
    {
    return;
    }
  cmbNucPartsTreeItem* pinNode = new cmbNucPartsTreeItem(partsRoot, newpin);
  pinNode->setText(0, newpin->name.c_str());
  Qt::ItemFlags itemFlags(
    Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  pinNode->setFlags(itemFlags); // not editable
  pinNode->setChildIndicatorPolicy(
    QTreeWidgetItem::DontShowIndicatorWhenChildless);
  cmbNucPartsTreeItem* selItem = this->getSelectedItem(
    this->Internal->PartsList);
  if(selItem)
    {
    selItem->setSelected(false);
    }
  pinNode->setSelected(true);
  this->onPartsSelectionChanged();
  emit pinsModified(this->getCurrentAssembly());
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::onRemoveSelectedPart()
{
  cmbNucPartsTreeItem* selItem = this->getSelectedItem(
    this->Internal->PartsList);
  if(!selItem || !selItem->getPartObject())
    {
    return;
    }
  AssyPartObj* selObj = selItem->getPartObject();
  PinCell* pincell = NULL;

  enumNucPartsType selType = selObj->GetType();
  std::string selText = selItem->text(0).toStdString();
  switch(selType)
  {
  case CMBNUC_CORE:
    emit deleteCore();
    break;
  case CMBNUC_ASSEMBLY:
    selItem->setSelected(false);
    emit deleteAssembly(selItem);
    break;
  case CMBNUC_ASSY_PINCELL:
    pincell = dynamic_cast<PinCell*>(selObj);
    if(pincell)
      {
      this->Internal->PartsList->blockSignals(true);
      cmbNucAssembly* assem = this->getCurrentAssembly();
      QTreeWidgetItem * p = selItem->parent();
      delete selItem;
      this->Internal->PartsList->setCurrentItem(p);
      assem->RemovePinCell(pincell->label);
      emit pincellDeleted();
      emit pinsModified(assem);
      this->Internal->PartsList->blockSignals(false);
      this->onPartsSelectionChanged();
      }
    break;
  case CMBNUC_ASSY_DUCT:
  case CMBNUC_ASSY_CYLINDER_PIN:
  case CMBNUC_ASSY_FRUSTUM_PIN:
  default:
    break;
  }
  emit checkSavedAndGenerate();
}

void cmbNucInputListWidget::onDeleteAssembly(QTreeWidgetItem* item)
{
  this->setCursor(Qt::BusyCursor);
  std::string selText = item->text(0).toStdString();
  this->Internal->PartsList->blockSignals(true);
  delete item;
  this->NuclearCore->RemoveAssembly(selText);
  this->Internal->PartsList->setCurrentItem(this->Internal->RootCoreNode);
  emit assembliesModified(this->NuclearCore);
  this->Internal->PartsList->blockSignals(false);
  this->Internal->RootCoreNode->setSelected(true);
  this->onPartsSelectionChanged();
  this->unsetCursor();
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::createMaterialItem(
  const QString& name, const QString& label, const QColor& color)
{
  QTreeWidgetItem* matRoot = this->Internal->MaterialTree->invisibleRootItem();
  Qt::ItemFlags matFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable |
                         Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
  cmbNucPartsTreeItem* mNode = new cmbNucPartsTreeItem(matRoot, NULL);
  mNode->setText(1, name);
  mNode->setText(2, label);
  QBrush bgBrush(color);
  mNode->setBackground(3, bgBrush);
  mNode->setFlags(matFlags);
  mNode->setCheckState(0, Qt::Checked);

  cmbNucPartsTreeItem* selItem = this->getSelectedItem(
    this->Internal->MaterialTree);
  if(selItem)
    {
    selItem->setSelected(false);
    }
  mNode->setSelected(true);
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::onImportMaterial()
{
  QStringList fileNames =
    QFileDialog::getOpenFileNames(this,
                                 "Open Material File...",
                                 QDir::homePath(),
                                 "INI Files (*.ini);;All Files (*.*)");
  if(fileNames.count() == 0)
    {
    return;
    }
  this->setCursor(Qt::BusyCursor);
  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  foreach(QString fileName, fileNames)
    {
    if(!fileName.isEmpty())
      {
      if(matColorMap->OpenFile(fileName))
        {
        this->initMaterialsTree();// reload the material tree.
        }
      }
    }
  this->unsetCursor();
}
//----------------------------------------------------------------------------
void cmbNucInputListWidget::onSaveMaterial()
{
  QString fileName =
    QFileDialog::getSaveFileName(this,
                                 "Save Material File...",
                                 QDir::homePath(),
                                 "INI Files (*.ini)");
  if(!fileName.isEmpty())
    {
    this->setCursor(Qt::BusyCursor);
    cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
    matColorMap->SaveToFile(fileName);
    this->unsetCursor();
    }
}
//----------------------------------------------------------------------------
void cmbNucInputListWidget::onMaterialClicked(QTreeWidgetItem* item, int col)
{
  if(col != 3)
    {
    return;
    }

  QBrush bgBrush = item->background(col);
  QColor color = QColorDialog::getColor(bgBrush.color(), this,
                                        "Select Color for Material",
                                        QColorDialog::ShowAlphaChannel);
  if(color.isValid() && color != bgBrush.color())
    {
    bgBrush.setColor(color);
    item->setBackground(col, bgBrush);
    }
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::updateUI(bool selCore)
{
  this->initUI();
  if(!this->NuclearCore)
    {
    this->setEnabled(0);
    return;
    }
  this->setEnabled(1);
  // Core node
  this->initCoreRootNode();
  // Assembly nodes
  for(int i=0; i<this->NuclearCore->GetNumberOfAssemblies(); i++)
    {
    this->updateWithAssembly(this->NuclearCore->GetAssembly(i),
      (!selCore && i == (this->NuclearCore->GetNumberOfAssemblies()-1)));
    }

  if(selCore)
    {
    this->Internal->RootCoreNode->setSelected(true);
    this->onPartsSelectionChanged();
    }
}
//-----------------------------------------------------------------------------
void cmbNucInputListWidget::initCoreRootNode()
{
  if(this->Internal->RootCoreNode == NULL)
    {
    Qt::ItemFlags itemFlags(
      Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    this->Internal->RootCoreNode = new cmbNucPartsTreeItem(
      this->Internal->PartsList->invisibleRootItem(), this->NuclearCore);
    this->Internal->RootCoreNode->setHightlights( this->NuclearCore->changeSinceLastSave(),
                                                  this->NuclearCore->changeSinceLastGenerate());
    connect(this, SIGNAL(checkSavedAndGenerate()),
            this->Internal->RootCoreNode->connection, SLOT(checkSaveAndGenerate()));
    this->Internal->RootCoreNode->setText(0, "Core");
    this->Internal->RootCoreNode->setFlags(itemFlags); // not editable
    this->Internal->RootCoreNode->setChildIndicatorPolicy(
      QTreeWidgetItem::DontShowIndicatorWhenChildless);
    this->Internal->RootCoreNode->setExpanded(true);
    }
}

void cmbNucInputListWidget::updateWithAssembly(cmbNucAssembly* assy, bool select)
{
  this->Internal->PartsList->blockSignals(true);
  // Assembly node
  Qt::ItemFlags itemFlags(
    Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  cmbNucPartsTreeItem* assyNode = new cmbNucPartsTreeItem(
    this->Internal->RootCoreNode, assy);
  connect(this, SIGNAL(checkSavedAndGenerate()),
          assyNode->connection, SLOT(checkSaveAndGenerate()));
  connect(assy->GetConnection(), SIGNAL(dataChangedSig()),
          assyNode->connection, SLOT(checkSaveAndGenerate()));
  connect(assy->GetConnection(), SIGNAL(dataChangedSig()),
          this->Internal->PartsList, SLOT(update()));
  assyNode->setText(0, assy->label.c_str());
  assyNode->setFlags(itemFlags); // not editable
  assyNode->setChildIndicatorPolicy(
    QTreeWidgetItem::DontShowIndicatorWhenChildless);

  assyNode->setHightlights(assy->changeSinceLastSave(),
                           assy->changeSinceLastGenerate());

  /// ******** populate parts tree ********
  QTreeWidgetItem* partsRoot = assyNode;

  // ducts
  cmbNucPartsTreeItem* ductsNode = new cmbNucPartsTreeItem(partsRoot,
    &assy->AssyDuct);
  ductsNode->setText(0, "Duct");
  ductsNode->setFlags(itemFlags); // not editable
  ductsNode->setChildIndicatorPolicy(
    QTreeWidgetItem::DontShowIndicatorWhenChildless);

  // pincells
  for(size_t i = 0; i < assy->GetNumberOfPinCells(); i++)
    {
    PinCell *pincell = assy->GetPinCell(i);
    cmbNucPartsTreeItem* pinNode = new cmbNucPartsTreeItem(partsRoot, pincell);
    pinNode->setText(0, QString::fromStdString(pincell->label));
    pinNode->setFlags(itemFlags); // not editable
    pinNode->setChildIndicatorPolicy(
      QTreeWidgetItem::DontShowIndicatorWhenChildless);
    }

  this->Internal->PartsList->blockSignals(false);

  if(select)
    {
    this->Internal->PartsList->setCurrentItem(partsRoot);
    this->onPartsSelectionChanged();
    }
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::onPartsSelectionChanged()
{
  cmbNucPartsTreeItem* selItem = this->getSelectedItem(
    this->Internal->PartsList);
  this->updateContextMenu(selItem ? selItem->getPartObject() : NULL);
  this->fireObjectSelectedSignal(selItem);
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::updateContextMenu(AssyPartObj* selObj)
{
  if(!selObj)
    {
    this->setActionsEnabled(false);
    return;
    }
  this->setActionsEnabled(false);
  this->Internal->Action_NewAssembly->setEnabled(true);
  this->Internal->Action_NewPin->setEnabled(true);

  switch(selObj->GetType())
  {
  case CMBNUC_ASSEMBLY:
    this->Internal->Action_DeletePart->setEnabled(true);
    break;
  case CMBNUC_CORE:
    this->Internal->Action_NewPin->setEnabled(false);
    this->Internal->Action_DeletePart->setEnabled(true);
    break;
  case CMBNUC_ASSY_PINCELL:
    this->Internal->Action_DeletePart->setEnabled(true);
    break;
  case CMBNUC_ASSY_FRUSTUM_PIN:
    this->Internal->Action_DeletePart->setEnabled(true);
    break;
  case CMBNUC_ASSY_CYLINDER_PIN:
    this->Internal->Action_DeletePart->setEnabled(true);
    break;
  default:
    break;
  }
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::labelChanged(QString newl)
{
  cmbNucPartsTreeItem* selItem =
    this->getSelectedItem(this->Internal->PartsList);
  if(selItem)
  {
    selItem->setText(0,newl);
  }
}

//-----------------------------------------------------------------------------
void cmbNucInputListWidget::fireObjectSelectedSignal(
  cmbNucPartsTreeItem* selItem)
{
  if(selItem)
    {
    emit this->objectSelected(selItem->getPartObject(),
      selItem->text(0).toStdString().c_str());
    }
  else
    {
    emit this->objectSelected(NULL, NULL);
    }
}

//-----------------------------------------------------------------------------
cmbNucPartsTreeItem* cmbNucInputListWidget::getSelectedItem(
  QTreeWidget* treeWidget)
{
  QList<QTreeWidgetItem*> selItems = treeWidget->selectedItems();
  return selItems.count()>0 ? dynamic_cast<cmbNucPartsTreeItem*>(
    selItems.value(0)) : NULL;
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::initPartsTree()
{
  QTreeWidget* treeWidget = this->Internal->PartsList;

  treeWidget->blockSignals(true);
  treeWidget->clear();
  treeWidget->setHeaderLabels(QStringList() << tr("Name") << tr("") << tr("") << tr("FileName"));
  treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
  treeWidget->setAcceptDrops(false);
  treeWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
  treeWidget->blockSignals(false);
}

//----------------------------------------------------------------------------
void cmbNucInputListWidget::initMaterialsTree()
{
  QTreeWidget* treeWidget = this->Internal->MaterialTree;

  treeWidget->blockSignals(true);
  treeWidget->clear();
  treeWidget->setHeaderLabels(
    QStringList() << tr("Show") << tr("Name") << tr("Label") << tr("Color") );
  treeWidget->setColumnCount(4);
  treeWidget->setAlternatingRowColors(true);
  treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
  treeWidget->header()->setResizeMode(0, QHeaderView::ResizeToContents);
  treeWidget->header()->setResizeMode(1, QHeaderView::ResizeToContents);
  treeWidget->header()->setResizeMode(2, QHeaderView::ResizeToContents);
  treeWidget->setAcceptDrops(false);
  treeWidget->setContextMenuPolicy(Qt::NoContextMenu);

  cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
  matColorMap->buildTree(treeWidget);

  treeWidget->blockSignals(false);
}

void cmbNucInputListWidget::assemblyModified(cmbNucPartsTreeItem* assyNode)
{
  cmbNucAssembly* assem = this->getCurrentAssembly();
  if(assem)
  {
    assem->setAndTestDiffFromFiles(true);
    NuclearCore->checkUsedAssembliesForGen();
    assyNode->setHightlights(assem->changeSinceLastSave(),
                             assem->changeSinceLastGenerate());
    Internal->RootCoreNode->setHightlights(NuclearCore->changeSinceLastSave(),
                                           NuclearCore->changeSinceLastGenerate());
    this->Internal->PartsList->repaint();
  }
}

void cmbNucInputListWidget::repaintList()
{
  if(Internal->RootCoreNode != NULL)
    Internal->RootCoreNode->setHightlights(NuclearCore->changeSinceLastSave(),
                                           NuclearCore->changeSinceLastGenerate());
  this->Internal->PartsList->repaint();
}

void cmbNucInputListWidget::coreModified()
{
  cmbNucPartsTreeItem* selItem = this->getSelectedPartNode();
  if(selItem && NuclearCore)
  {
    NuclearCore->setAndTestDiffFromFiles(true);
    NuclearCore->clearOldGeometry();
    selItem->setHightlights(NuclearCore->changeSinceLastSave(),
                            NuclearCore->changeSinceLastGenerate());
    this->Internal->PartsList->repaint();
  }
}

void cmbNucInputListWidget::valueChanged()
{
  AssyPartObj* part = this->getSelectedPart();
  cmbNucPartsTreeItem* selItem = this->getSelectedPartNode();
  if(part == NULL || selItem == NULL)
  {
    return;
  }
  switch(part->GetType())
  {
    case CMBNUC_ASSY_LATTICE:
    case CMBNUC_ASSY_DUCTCELL:
    case CMBNUC_ASSY_PINCELL:
      this->assemblyModified(dynamic_cast<cmbNucPartsTreeItem*>(selItem->parent()));
      break;
    case CMBNUC_ASSY_FRUSTUM_PIN:
    case CMBNUC_ASSY_CYLINDER_PIN:
    case CMBNUC_ASSY_DUCT:
    case CMBNUC_ASSY_BASEOBJ:
      this->assemblyModified(dynamic_cast<cmbNucPartsTreeItem*>(selItem->parent()->parent()));
      break;
    case CMBNUC_ASSEMBLY:
      this->assemblyModified(selItem);
      break;
    case CMBNUC_CORE:
      this->coreModified();
      break;
  }
}

AssyPartObj* cmbNucInputListWidget::getSelectedCoreOrAssembly()
{
  AssyPartObj* part = this->getSelectedPart();
  cmbNucPartsTreeItem* selItem = this->getSelectedPartNode();
  if(part == NULL || selItem == NULL)
  {
    return NULL;
  }
  switch(part->GetType())
  {
    case CMBNUC_ASSY_LATTICE:
    case CMBNUC_ASSY_DUCTCELL:
    case CMBNUC_ASSY_PINCELL:
      return dynamic_cast<cmbNucPartsTreeItem*>(selItem->parent())->getPartObject();
      break;
    case CMBNUC_ASSY_FRUSTUM_PIN:
    case CMBNUC_ASSY_CYLINDER_PIN:
    case CMBNUC_ASSY_DUCT:
    case CMBNUC_ASSY_BASEOBJ:
      return dynamic_cast<cmbNucPartsTreeItem*>(selItem->parent()->parent())->getPartObject();
      break;
    case CMBNUC_ASSEMBLY:
    case CMBNUC_CORE:
      return part;
      break;
  }
  return NULL;
}

void cmbNucInputListWidget::clearTable()
{
  this->Internal->PartsList->clear();
  this->Internal->MeshComponents->clear();
}

void cmbNucInputListWidget::hideLabels(bool v)
{
  this->Internal->MaterialTree->setColumnHidden(2,v);
}

void cmbNucInputListWidget::updateMeshTable(QList<QTreeWidgetItem*> meshParts)
{
  QTreeWidget* treeWidget = this->Internal->MeshComponents;

  treeWidget->blockSignals(true);
  treeWidget->clear();
  treeWidget->setHeaderLabels(QStringList() << tr("") << tr(""));
  treeWidget->setHeaderHidden(true);
  treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
  treeWidget->setAcceptDrops(false);
  treeWidget->addTopLevelItems( meshParts );
  treeWidget->resizeColumnToContents(0);
  meshParts.at(0)->setSelected(true);
  treeWidget->blockSignals(false);
  emit(subMeshSelected(meshParts.at(0)));
}

void cmbNucInputListWidget::updateMainMeshComponents(QStringList parts, int sel)
{
  this->Internal->meshMajorSet->blockSignals(true);
  this->Internal->meshMajorSet->clear();
  this->Internal->meshMajorSet->addItems(parts);
  this->Internal->meshMajorSet->blockSignals(false);
  this->Internal->meshMajorSet->setCurrentIndex ( sel );
}

void cmbNucInputListWidget::modelIsLoaded(bool v)
{
  if(v) this->Internal->tabInputs->setCurrentIndex(0);
  this->Internal->tabInputs->setTabEnabled(0, v);
  this->Internal->tabInputs->setTabEnabled(1, v);
}
