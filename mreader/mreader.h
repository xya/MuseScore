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
#include <QVector>
#include <QWidget>
#include "libmscore/mscore.h"
#include "libmscore/score.h"

class QAction;

// Element, page pair.
struct PageElement
{
    Ms::Element *E;
    Ms::Page *P;
    
    PageElement() : E(nullptr), P(nullptr) {}
    PageElement(Ms::Element *e, Ms::Page *p) : E(e), P(p) {}
};

// Presents single pages from a score.
class ScorePager : public QObject
{
Q_OBJECT
public:
  ScorePager(Ms::MScore &App, QObject *Parent = nullptr);
  virtual ~ScorePager();
  
  // Current page index.
  int pageIndex() const { return m_pageIdx; }
  void setPageIndex(int newIndex);
  
  // Number of logical pages.
  int numPages() const;
  
  // Number of physical pages.
  int numPhysPages() const;
  
  // Number of pages shown on screen at the same time.
  int numPagesShown() const { return m_twoSided ? 2 : 1; }
  
  // Whether to disaply one or two pages at the same time.
  bool isTwoSided() const { return m_twoSided; }
  void setTwoSided(bool newVal);
  
  // Page size.
  QSizeF pageSize() const { return m_pageSize; }
  void setPageSize(QSizeF newSize);
  
  // Staff scaling factor.
  double scale() const { return m_scale; }
  void setScale(double newScale);
  
  // Currently loaded score.
  Ms::Score * loadedScore() const { return m_score; }
  
  // Currently displayed score.
  Ms::Score * workingScore() const { return m_workingScore; }
  
  // Score title.
  QString title() const;
  
  // Load a score file.
  bool loadScore(QString path);
  
  // Return the currently visible page items.
  void addPageItems(QVector<PageElement> &elements,
                    QVector<QPointF> &pageOffets);
  
public slots:
  void nextPage();
  void previousPage();
  void firstPage();
  void lastPage();
  
signals:
  void updated();
  
private:
  // Recalculate the layout of the whole score.
  void updateLayout();
  
  // Try to align the last page's systems with the previous page's.
  void alignLastPageSystems();
  
  Ms::MScore &m_app;
  Ms::Score *m_score;
  Ms::Score *m_workingScore;
  int m_pageIdx;
  double m_scale;
  QSizeF m_pageSize;
  bool m_twoSided;
};

class ScoreWidget : public QWidget
{
Q_OBJECT
  
public:
  ScoreWidget(ScorePager &pager);
  virtual ~ScoreWidget();
  
protected:
  virtual void paintEvent(QPaintEvent*);
  virtual void resizeEvent(QResizeEvent*);
  virtual void wheelEvent(QWheelEvent*);
  
private:
  ScorePager &m_pager;
  QAction *m_previousPage;
  QAction *m_nextPage;
  QAction *m_firstPage;
  QAction *m_lastPage;
};

#endif
