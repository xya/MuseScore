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
    View.setWindowTitle("MuseReader");
    View.setWindowState(Qt::WindowMaximized);
    View.show();
    App.exec();
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

ScoreWidget::ScoreWidget(Ms::MScore &App) : QWidget(), m_app(App), m_score(nullptr)
{
    m_pageIdx = 0;
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
    return true;
}

void ScoreWidget::setPageIndex(int newIndex)
{
    if (!m_score || (newIndex < 0) || (newIndex >= m_score->pages().size()))
        return;
    if (newIndex != m_pageIdx)
    {
        m_pageIdx = newIndex;
        update();
    }
}

void ScoreWidget::updateLayout(QSize viewSize)
{
    double widthInch = viewSize.width() / (double)Ms::MScore::DPI;
    double heightInch = viewSize.height() / (double)Ms::MScore::DPI;
    double f  = 1.0 / Ms::INCH;
    double marginMm = 10.0;
    double staffSpaceMm = 1.5;

    Ms::PageFormat pf;
    pf.setEvenTopMargin(marginMm * f);
    pf.setEvenBottomMargin(marginMm * f);
    pf.setEvenLeftMargin(marginMm * f);
    pf.setOddTopMargin(marginMm * f);
    pf.setOddBottomMargin(marginMm * f);
    pf.setOddLeftMargin(marginMm * f);
    pf.setSize(QSizeF(widthInch, heightInch));
    pf.setPrintableWidth(widthInch - (marginMm * 2.0 * f));
    pf.setTwosided(false);

    m_score->setPageFormat(pf);
    m_score->style()->setSpatium(staffSpaceMm * f * Ms::MScore::DPI);
    m_score->setPrinting(true);
    m_score->update();
}

void ScoreWidget::keyReleaseEvent(QKeyEvent *e)
{
    if (e->matches(QKeySequence::MoveToNextPage))
    {
        setPageIndex(m_pageIdx + 1);
    }
    else if (e->matches(QKeySequence::MoveToPreviousPage))
    {
        setPageIndex(m_pageIdx - 1);
    }
}

void ScoreWidget::resizeEvent(QResizeEvent *e)
{
    updateLayout(e->size());
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

    //qDebug("Pages: %d",  m_score->pages().size());
    if (m_score->pages().size() > m_pageIdx)
    {
        Ms::Page *page = m_score->pages().at(m_pageIdx);
        QRectF bounds = page->abbox();
        QList<Ms::Element*> ell = page->items(bounds);
        //unsigned numSystems = page->systems()->size();
        //qDebug("Systems: %d, elements: %d W: %f, H: %f", numSystems, ell.size(),
        //       bounds.width(), bounds.height());
        qStableSort(ell.begin(), ell.end(), Ms::elementLessThan);
        drawElements(p, ell);
    }
    p.restore();
}

void ScoreWidget::drawElements(QPainter &painter, const QList<Ms::Element*> &el)
{
    unsigned i = 0;
    for (const Ms::Element* e : el)
    {
        e->itemDiscovered = 0;
        if (!e->visible())
            continue;
        QPointF pos(e->pagePos());
        painter.translate(pos);
        e->draw(&painter);
        painter.translate(-pos);
        i++;
    }
}
