#include "Schedule.hpp"
#include "Scheduler.hpp"
#include <iostream>
#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QTableView>

int main(int argc, char* argv[]) {
  int ret_code;
  QApplication app(argc, argv);
  Kokkos::initialize(argc, argv);
  {
    const unsigned nslots = 13;

    // Read the rooms from yaml
    Rooms rooms("../../data/rooms.yaml");

    // Read the minisymposia from yaml
    Minisymposia mini("../../data/minisymposia_improved_themes.yaml", rooms.size(), nslots);
 
    // Run the genetic algorithm
    Scheduler s(mini, rooms, nslots);
    s.run_genetic(1000, 200, 0.01, 10);
    s.record("schedule.md");

    // Create a table to display the schedule
    auto tableView = new QTableView();
    Schedule sched(rooms.size(), nslots, &rooms, &mini);
    tableView->setModel(&sched);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->setDragEnabled(true);
    tableView->setDefaultDropAction(Qt::MoveAction);
    tableView->setDragDropMode(QAbstractItemView::InternalMove);

    // Populate the schedule
    s.populate(sched);

    // Create a window
    QMainWindow window;
    window.setCentralWidget(tableView);

    // Create a save action
    auto saveAct = new QAction(QObject::tr("&Save"), &window);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(QObject::tr("Save the schedule to disk"));
    window.connect(saveAct, &QAction::triggered, &sched, &Schedule::save);

    // Create a load action
    auto loadAct = new QAction(QObject::tr("&Load"), &window);
    loadAct->setStatusTip(QObject::tr("Load a schedule from disk"));
    window.connect(loadAct, &QAction::triggered, &sched, &Schedule::load);

    // Create a menu bar
    auto fileMenu = window.menuBar()->addMenu(QObject::tr("&File"));
    fileMenu->addAction(saveAct);
    fileMenu->addAction(loadAct);

    // Display the window
    window.show();
    ret_code = app.exec();
  }
  Kokkos::finalize();
  return ret_code;
}