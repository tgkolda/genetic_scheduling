#include "Schedule.hpp"

#include <QDebug>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>

Schedule::Schedule(int nrows, int ncols, Rooms* rooms, Minisymposia* mini, QObject *parent) : 
  mini_indices_("indices", nrows, ncols), rooms_(rooms), mini_(mini), QAbstractTableModel(parent)
{
  // Create a table to display the schedule
  tableView_ = new QTableView();
  tableView_->setModel(this);
  tableView_->setSelectionMode(QAbstractItemView::SingleSelection);
  tableView_->setDragEnabled(true);
  tableView_->setDefaultDropAction(Qt::MoveAction);
  tableView_->setDragDropMode(QAbstractItemView::InternalMove);
  selectionModel_ = tableView_->selectionModel();

  // Create a window
  window_.setCentralWidget(tableView_);

  // Create a search dialog
  dialog_ = new QDialog(&window_);
  dialog_->setWindowTitle(tr("Find a Minisymposium"));

  auto findLabel = new QLabel(tr("Enter the name of a minisymposium:"));
  searchTerm_ = new QLineEdit;
  auto findButton = new QPushButton(tr("&Find"));
  dialog_->connect(findButton, &QPushButton::clicked, this, &Schedule::search);

  auto layout = new QHBoxLayout;
  layout->addWidget(findLabel);
  layout->addWidget(searchTerm_);
  layout->addWidget(findButton);

  dialog_->setLayout(layout);

  // Create a save action
  auto saveAct = new QAction(tr("&Save"), &window_);
  saveAct->setShortcuts(QKeySequence::Save);
  saveAct->setStatusTip(tr("Save the schedule to disk"));
  window_.connect(saveAct, &QAction::triggered, this, &Schedule::save);

  // Create a load action
  auto loadAct = new QAction(tr("&Load"), &window_);
  loadAct->setStatusTip(tr("Load a schedule from disk"));
  window_.connect(loadAct, &QAction::triggered, this, &Schedule::load);

  // Create a score action
  auto scoreAct = new QAction(tr("&Compute Score"), &window_);
  scoreAct->setStatusTip(tr("Compute the score of the current schedule"));
  window_.connect(scoreAct, &QAction::triggered, this, &Schedule::computeScore);

  // Create a find action
  auto findAct = new QAction(tr("Find"), &window_);
  findAct->setShortcuts(QKeySequence::Find);
  findAct->setStatusTip(tr("Find a string within minisymposia titles"));
  window_.connect(findAct, &QAction::triggered, dialog_, &QDialog::show);

  // Create a menu bar
  auto fileMenu = window_.menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(saveAct);
  fileMenu->addAction(loadAct);
  auto scoreMenu = window_.menuBar()->addMenu(tr("&Score"));
  scoreMenu->addAction(scoreAct);
  auto searchMenu = window_.menuBar()->addMenu(tr("&Search"));
  searchMenu->addAction(findAct);

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
      return QVariant(tr(mini_->get(id).full_title().c_str()));
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

void Schedule::computeScore() {

}

void Schedule::search() {
  QString text = searchTerm_->text();

  if (text.isEmpty()) {
    QMessageBox::information(dialog_, tr("Empty Field"),
      tr("Please enter a name."));
  } else {
    // Get the currently selected indices
    QModelIndex start_index;
    if(selectionModel_->hasSelection()) {
      start_index = selectionModel_->selectedIndexes().at(0);
    }
    else {
      start_index = tableView_->indexAt(QPoint(0,0));
    }
    auto index = start_index;

    while(true) {
      // Move to next cell
      if(index.siblingAtRow(index.row()+1).isValid()) {
        index = index.siblingAtRow(index.row()+1);
      }
      else {
        if(index.sibling(0, index.column()+1).isValid()) {
          index = index.sibling(0, index.column()+1);
        }
        else {
          index = index.sibling(0, 0);
        }
      }

      // Check whether the search string is in this entry
      unsigned id = mini_indices_(index.row(), index.column());
      if(id < mini_->size()) {
        bool found = tr(mini_->get(id).full_title().c_str()).contains(text, Qt::CaseInsensitive);
        if(found) {
          // Set the selection
          selectionModel_->select(index, QItemSelectionModel::ClearAndSelect);
          return;
        }
      }

      // Looped all the way around and never found the item
      if(index == start_index) {
        QMessageBox::information(dialog_, tr("Not Found"), text + tr(" could not be found."));
        return;
      }
    }
  }
}