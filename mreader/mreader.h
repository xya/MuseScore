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
#include <QWidget>
#include "libmscore/mscore.h"
#include "libmscore/score.h"

class ScoreWidget : public QWidget {
Q_OBJECT
  
public:
  ScoreWidget(Ms::MScore &App);
  virtual ~ScoreWidget();
  
  bool loadScore(QString path);
  
protected:
  virtual void paintEvent(QPaintEvent*);
  
private:
  void paint(const QRect& r, QPainter& p);
  void drawElements(QPainter &painter, const QList<Ms::Element*> &el);
  
  Ms::MScore &m_app;
  Ms::Score *m_score;
  double m_mag;
  unsigned m_pageIdx;
  QTransform m_matrix;
  QTransform m_imatrix;
};

#endif
