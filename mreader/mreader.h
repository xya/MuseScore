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
#include <memory>
#include <initializer_list>
#include "libmscore/mscore.h"
#include "libmscore/score.h"

class QAction;

using KeyIList = std::initializer_list<QKeySequence::StandardKey>;

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
  
  // Current part index.
  int partIndex() const { return m_partIdx; }
  
  // Current page index.
  int pageIndex() const { return m_pageIdx; }
  
  // Number of logical pages.
  int numPages() const;
  
  // Number of physical pages.
  int numPhysPages() const;
  
  // Number of pages shown on screen at the same time.
  int numPagesShown() const { return m_twoSided ? 2 : 1; }
  
  // Whether to disaply one or two pages at the same time.
  bool isTwoSided() const { return m_twoSided; }
  
  // Whether to show scores in concert pitch or not.
  bool concertPitch() const { return m_concertPitch; }
  
  // Whether to show lyrics or not.
  bool showLyrics() const { return m_showLyrics; }
  
  // Whether to show instrument names or not.
  bool showInstrumentNames() const { return m_showInstrumentNames; }
  
  // How many semitones to transpose by.
  int semitoneDelta() const { return m_semitoneDelta; }
  
  // Page size.
  QSizeF pageSize() const { return m_pageSize; }
  void setPageSize(QSizeF newSize);
  
  // Staff scaling factor.
  double scale() const { return m_scale; }
  void setScale(double newScale);
  
  // Score title.
  QString title() const;
  
  // Part name.
  QString partName() const { return m_partName; }
  
  // Load a score file.
  bool loadScore(QString path);
  
  // Return the currently visible page items.
  void addPageItems(QVector<PageElement> &elements,
                    QVector<QPointF> &pageOffets);
  
public slots:
  void nextPart();
  void previousPart();
  void nextPage();
  void previousPage();
  void firstPage();
  void lastPage();
  void zoomIn();
  void zoomOut();
  void upSemitone();
  void downSemitone();
  void upOctave();
  void downOctave();
  void setTwoSided(bool newVal);
  void setConcertPitch(bool newVal);
  void setShowLyrics(bool newVal);
  void setShowInstrumentNames(bool newVal);
  void setPageIndex(int newIndex);
  void setPartIndex(int newIndex);
  void setSemitoneDelta(int newVal);
  
signals:
  void updated();
  void partChanged();
  
private:
  void loadPart(int partIdx);
  void removeLyrics(Ms::Score *score);
  void transposeKeySignatures(Ms::Score *score, bool flip);
  void transposeAll(Ms::Score *score, int semitones);
  void updateStyle();
  
  // Recalculate the layout of the whole score.
  void updateLayout();
  
  // Try to align the last page's systems with the previous page's.
  void alignLastPageSystems();
  
  Ms::MScore &m_app;
  std::unique_ptr<Ms::Score> m_score;
  std::unique_ptr<Ms::Score> m_workingScore;
  QString m_partName;
  int m_partIdx;
  int m_pageIdx;
  double m_scale;
  QSizeF m_pageSize;
  bool m_twoSided;
  bool m_alignSystems;
  bool m_showInstrumentNames;
  bool m_soloInstrument;
  bool m_showLyrics;
  bool m_concertPitch;
  int m_semitoneDelta;
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
  void updateTitle();
  
private:
  QAction *pagerAction(QString text, QKeySequence key,
                       const char *slotName = nullptr);
  QAction *pagerAction(QString text, KeyIList &keys,
                       const char *slotName = nullptr);
  QAction *pagerAction(QString text, QKeySequence key, bool initialVal,
                       const char *slotName = nullptr);
  
  ScorePager &m_pager;
  QAction *m_previousPart;
  QAction *m_nextPart;
  QAction *m_prevPage;
  QAction *m_nextPage;
  QAction *m_firstPage;
  QAction *m_lastPage;
  QAction *m_zoomIn;
  QAction *m_zoomOut;
  QAction *m_twoSided;
  QAction *m_concertPitch;
  QAction *m_showLyrics;
  QAction *m_showInstrumentNames;
  QAction *m_fullscreen;
  QAction *m_upSemitone;
  QAction *m_downSemitone;
  QAction *m_upOctave;
  QAction *m_downOctave;
  Qt::WindowStates m_lastScreenState;
};

#endif
