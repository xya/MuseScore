#include <QPainter>
#include "mreader.h"
#include "libmscore/element.h"
#include "libmscore/page.h"

namespace Ms {
QString revision;
bool enableTestMode = false;
}

int main(int argc, char **argv)
{
    QApplication App(argc, argv);
    if (argc < 2) {
        return 1;
    }
    
    QString path(argv[1]);
    Ms::MScore MSApp;
    Ms::MScore::DPI  = 120;
    Ms::MScore::PDPI = 120;
    MSApp.init();
    ScoreWidget View(MSApp);
    if (!View.loadScore(path))
    {
        return 1;
    }
    View.show();
    App.exec();
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

ScoreWidget::ScoreWidget(Ms::MScore &App) : QWidget(), m_app(App), m_score(nullptr)
{
    m_mag  = 1.0;
    m_matrix = QTransform(m_mag, 0.0, 0.0, m_mag, 0.0, 0.0);
    m_imatrix = m_matrix.inverted();
}

ScoreWidget::~ScoreWidget()
{
    delete m_score;
}

bool ScoreWidget::loadScore(QString path)
{
    QFileInfo fi(path);
    m_score = new Ms::Score(m_app.baseStyle());
    m_score->setName(fi.completeBaseName());

    Ms::Score::FileError rv = m_score->loadMsc(path, false);
    if (rv != Ms::Score::FileError::FILE_NO_ERROR)
    {
        delete m_score;
        m_score = nullptr;
        return false;
    }
    m_score->setLayoutMode(Ms::LayoutMode::PAGE);
    m_score->doLayout();
    return true;
}

void ScoreWidget::paintEvent(QPaintEvent *e)
{
    if (!m_score)
          return;
    QPainter vp(this);
    vp.setRenderHint(QPainter::Antialiasing, true);
    vp.setRenderHint(QPainter::TextAntialiasing, true);

    paint(e->rect(), vp);

    vp.setTransform(m_matrix);
    vp.setClipping(false);
}

void ScoreWidget::paint(const QRect& r, QPainter& p)
{
    p.save();
    p.fillRect(r, QColor("white"));
    
    p.setTransform(m_matrix);
    QRectF fr = m_imatrix.mapRect(QRectF(r));
    
    QRegion r1(r);
    foreach (Ms::Page* page, m_score->pages())
    {
        QRectF pr(page->abbox().translated(page->pos()));
        if (pr.right() < fr.left())
            continue;
        if (pr.left() > fr.right())
            break;
        QList<Ms::Element*> ell = page->items(fr.translated(-page->pos()));
        qStableSort(ell.begin(), ell.end(), Ms::elementLessThan);
        QPointF pos(page->pos());
        p.translate(pos);
        drawElements(p, ell);
        p.translate(-pos);
        r1 -= m_matrix.mapRect(pr).toAlignedRect();
    }
    p.restore();
}

void ScoreWidget::drawElements(QPainter &painter, const QList<Ms::Element*> &el)
{
    for (const Ms::Element* e : el)
    {
        e->itemDiscovered = 0;
        if (!e->visible())
            continue;
        QPointF pos(e->pagePos());
        painter.translate(pos);
        e->draw(&painter);
        painter.translate(-pos);
    }
}
