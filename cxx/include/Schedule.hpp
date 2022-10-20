#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "Minisymposia.hpp"
#include "Rooms.hpp"
#include <QAbstractTableModel>
#include <QLineEdit>
#include <QMainWindow>
#include <QItemSelectionModel>
#include <QTableView>

class Schedule : public QAbstractTableModel
{
public:
  Schedule(Kokkos::View<unsigned**,Kokkos::LayoutStride> mini_indices, Rooms* rooms, Minisymposia* mini, QObject *parent = nullptr);
  ~Schedule();
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  Qt::DropActions supportedDropActions() const override;
  QMimeData* mimeData(const QModelIndexList &indices) const override;
  bool dropMimeData(const QMimeData *data, Qt::DropAction action, 
                    int row, int column, const QModelIndex &parent) override;
  void save();
  void load();
  void computeScore();
  void search();
private:
  QMainWindow window_;
  QItemSelectionModel *selectionModel_;
  QTableView* tableView_;
  QDialog* dialog_;
  QLineEdit* searchTerm_;
  Rooms* rooms_;
  Minisymposia* mini_;
  Kokkos::View<unsigned**> d_mini_indices_;
  Kokkos::View<unsigned**>::HostMirror h_mini_indices_;
};

#endif // SCHEDULE_H