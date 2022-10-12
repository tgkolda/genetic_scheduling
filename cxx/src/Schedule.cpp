#include "Schedule.hpp"

#include <QMimeData>
#include <QDebug>

Schedule::Schedule(int nrows, int ncols, Rooms* rooms, Minisymposia* mini, QObject *parent) : 
  mini_indices_("indices", nrows, ncols), rooms_(rooms), mini_(mini)
{

}

Schedule::~Schedule() {

}

int Schedule::rowCount(const QModelIndex &parent) const {
  return mini_indices_.extent(0);
}

int Schedule::columnCount(const QModelIndex &parent) const {
  return mini_indices_.extent(1);
}

QVariant Schedule::data(const QModelIndex &index, int role) const {
  if(role == Qt::DisplayRole) {
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
  return Qt::ItemIsEditable;
}