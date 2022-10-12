#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "Minisymposia.hpp"
#include "Rooms.hpp"
#include <QAbstractTableModel>

class Schedule : public QAbstractTableModel
{
public:
  Schedule(int nrows, int ncols, Rooms* rooms, Minisymposia* mini, QObject *parent = nullptr);
  ~Schedule();
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
  Qt::ItemFlags flags(const QModelIndex &index) const;
private:
  Rooms* rooms_;
  Minisymposia* mini_;
  Kokkos::View<int**, Kokkos::HostSpace> mini_indices_;
};

#endif // SCHEDULE_H