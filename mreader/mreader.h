#ifndef MSCORE_MREADER_MREADER_H
#define MSCORE_MREADER_MREADER_H

#include <QApplication>
#include <QFileInfo>
#include <QMouseEvent>
#include <QColor>
#include <QLineF>
#include <QFont>
#include <QFontMetrics>
#include <QPainterPath>
#include <QTextCursor>
#include <QQmlEngine>
#include <QQmlListProperty>
#include <QQueue>
#include <QList>
#include <QWidget>
#include "libmscore/mscore.h"
#include "libmscore/score.h"

// Presents single pages from a score.
class ScorePager : public QObject {
Q_OBJECT
public:
  ScorePager(Ms::MScore &App, QObject *Parent = nullptr);
  virtual ~ScorePager();
  
  // Current page index.
  int pageIndex() const { return m_pageIdx; }
  void setPageIndex(int newIndex);
  
  // Page size.
  QSizeF pageSize() const { return m_pageSize; }
  void setPageSize(QSizeF newSize);
  
  // Staff scaling factor.
  double scale() const { return m_scale; }
  void setScale(double newScale);
  
  // Currently loaded score.
  Ms::Score * score() const { return m_score; }
  
  // Load a score file.
  bool loadScore(QString path);
  
  // Return the currently visible page items.
  QList<Ms::Element*> pageItems();
  
public slots:
  void nextPage();
  void previousPage();
  void firstPage();
  void lastPage();
  
signals:
  void updated();
  
private:
  void updateLayout();
  
  Ms::MScore &m_app;
  Ms::Score *m_score;
  int m_pageIdx;
  double m_scale;
  QSizeF m_pageSize;
};

class ScoreWidget : public QWidget {
Q_OBJECT
  
public:
  ScoreWidget(ScorePager &pager);
  virtual ~ScoreWidget();
  
protected:
  virtual void paintEvent(QPaintEvent*);
  virtual void resizeEvent(QResizeEvent*);
  virtual void keyReleaseEvent(QKeyEvent*);
  virtual void wheelEvent(QWheelEvent*);
  
private:
  ScorePager &m_pager;
};

#endif
