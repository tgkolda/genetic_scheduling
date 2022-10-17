#include "Schedule.hpp"

#include <QDebug>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QTableView>

Schedule::Schedule(int nrows, int ncols, Rooms* rooms, Minisymposia* mini, QObject *parent) : 
  mini_indices_("indices", nrows, ncols), rooms_(rooms), mini_(mini), QAbstractTableModel(parent)
{
  // Create a table to display the schedule
  auto tableView = new QTableView();
  tableView->setModel(this);
  tableView->setSelectionMode(QAbstractItemView::SingleSelection);
  tableView->setDragEnabled(true);
  tableView->setDefaultDropAction(Qt::MoveAction);
  tableView->setDragDropMode(QAbstractItemView::InternalMove);

  // Create a window
  window_.setCentralWidget(tableView);

  // Create a save action
  auto saveAct = new QAction(tr("&Save"), &window_);
  saveAct->setShortcuts(QKeySequence::Save);
  saveAct->setStatusTip(tr("Save the schedule to disk"));
  window_.connect(saveAct, &QAction::triggered, this, &Schedule::save);

  // Create a load action
  auto loadAct = new QAction(tr("&Load"), &window_);
  loadAct->setStatusTip(tr("Load a schedule from disk"));
  window_.connect(loadAct, &QAction::triggered, this, &Schedule::load);

  // Create a menu bar
  auto fileMenu = window_.menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(saveAct);
  fileMenu->addAction(loadAct);

  // Display the window
  window_.show();
}

Schedule::~Schedule() {

}

int Schedule::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return mini_indices_.extent(0);
}

int Schedule::columnCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return mini_indices_.extent(1);
}

QVariant Schedule::data(const QModelIndex &index, int role) const {
  if(index.isValid() && role == Qt::DisplayRole) {
    unsigned id = mini_indices_(index.row(), index.column());
    if(id < mini_->size()) {
      return QVariant(tr(mini_->get_title(id).c_str()));
    }
  }

  return QVariant();
}

QVariant Schedule::headerData(int section, Qt::Orientation orientation, int role) const {
  if(role==Qt::DisplayRole){
    if(orientation==Qt::Horizontal) {
      // Return the session number
      return QString::number(section+1);
    }
    else {
      // Return the name of the room
      return QVariant(tr(rooms_->name(section).c_str()));
    }
  }
  return QVariant();
}

bool Schedule::setData(const QModelIndex &index, const QVariant &value, int role) {
  if(role==Qt::EditRole) {
    mini_indices_(index.row(), index.column()) = value.toInt();
    return true;
  }
  return false;
}

Qt::ItemFlags Schedule::flags(const QModelIndex &index) const {
  if (index.isValid())
    return Qt::ItemIsDragEnabled  | Qt::ItemIsDropEnabled  | QAbstractTableModel::flags(index);
  return QAbstractTableModel::flags(index);
}

Qt::DropActions Schedule::supportedDropActions() const
{
  return Qt::MoveAction | QAbstractTableModel::supportedDropActions();
}

QMimeData *Schedule::mimeData(const QModelIndexList &indices) const
{
  QMimeData *data=QAbstractTableModel::mimeData(indices);
  if(data){
    data->setData("row", QByteArray::number(indices.at(0).row()));
    data->setData("col", QByteArray::number(indices.at(0).column()));
  }
  return data;
}

bool Schedule::dropMimeData(const QMimeData *data, Qt::DropAction action, 
                            int row, int column, const QModelIndex &parent)
{
  if(!data || action!=Qt::MoveAction)
    return false;

  const QModelIndex old_index=index(data->data("row").toInt(),
                                    data->data("col").toInt());
  const QModelIndex current_index=parent;
  std::swap(mini_indices_(old_index.row(), old_index.column()),
            mini_indices_(current_index.row(), current_index.column()));
  return true;
}

void Schedule::save() {
  QString fileName = QFileDialog::getSaveFileName(&window_,
    tr("Save Schedule"), "",
    tr("Schedule (*.sched);;All Files (*)"));

  if (!fileName.isEmpty()) {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
      QMessageBox::information(&window_, tr("Unable to open file"), file.errorString());
      return;
    }
    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_4_5);
    for(unsigned i=0; i<mini_indices_.extent(0); i++) {
      for(unsigned j=0; j<mini_indices_.extent(1); j++) {
        out << mini_indices_(i,j);
      }
    }
  }
}

void Schedule::load() {
  QString fileName = QFileDialog::getOpenFileName(&window_,
    tr("Open Schedule"), "",
    tr("Schedule (*.sched);;All Files (*)"));
  if (!fileName.isEmpty()) {
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly)) {
      QMessageBox::information(&window_, tr("Unable to open file"), file.errorString());
      return;
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_4_5);
    for(unsigned i=0; i<mini_indices_.extent(0); i++) {
      for(unsigned j=0; j<mini_indices_.extent(1); j++) {
        in >> mini_indices_(i,j);
      }
    }
  }
}