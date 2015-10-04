//=============================================================================
//  MuseScore Reader
//
//  Copyright (C) 2011 Werner Schweer and others
//  Copyright (C) 2015 Pierre-Andre Saulais
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

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
    int PageIdx;
    
    PageElement() : E(nullptr), PageIdx(0) {}
    PageElement(Ms::Element *e, int pageIdx) : E(e), PageIdx(pageIdx) {}
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
  
  // Whether to show instrument names or not
  bool showInstrumentNames() const { return m_showInstrumentNames; }
  
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
  void zoomIn();
  void zoomOut();
  void setTwoSided(bool newVal);
  void setShowInstrumentNames(bool newVal);
  
signals:
  void updated();
  
private:
  void updateStyle();
  
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
  bool m_showInstrumentNames;
};

class ScoreWidget : public QWidget
{
Q_OBJECT
  
public:
  ScoreWidget(ScorePager &pager);
  virtual ~ScoreWidget();
  
  bool isFullscreen() const { return windowState() == Qt::WindowFullScreen; }
  
protected:
  virtual void paintEvent(QPaintEvent*) override;
  virtual void resizeEvent(QResizeEvent*) override;
  virtual void mouseDoubleClickEvent(QMouseEvent*) override;
  virtual void wheelEvent(QWheelEvent*) override;
  
private slots:
  void setFullscreen(bool newVal);
  
private:
  QAction *pagerAction(QString text, QKeySequence key,
                       const char *slotName = nullptr);
  QAction *pagerAction(QString text, QKeySequence key, bool initialVal,
                       const char *slotName = nullptr);
  
  ScorePager &m_pager;
  QAction *m_previousPage;
  QAction *m_nextPage;
  QAction *m_firstPage;
  QAction *m_lastPage;
  QAction *m_zoomIn;
  QAction *m_zoomOut;
  QAction *m_twoSided;
  QAction *m_showInstrumentNames;
  QAction *m_fullscreen;
  Qt::WindowStates m_lastScreenState;
};

#endif
