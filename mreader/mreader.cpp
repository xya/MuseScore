#include <QPainter>
#include <QScreen>
#include <cmath>
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
    
    QScreen *screen = App.primaryScreen();
    QString path(argv[1]);
    Ms::MScore MSApp;
    Ms::MScore::DPI = screen->logicalDotsPerInch();
    Ms::MScore::PDPI = Ms::MScore::DPI;
    MSApp.init();
    ScorePager Pager(MSApp);
    if (!Pager.loadScore(path))
    {
        return 1;
    }
    
    QString title = "MuseReader";
    if (!Pager.score()->title().isEmpty()) {
        title += " - ";
        title += Pager.score()->title();
    }
    
    ScoreWidget View(Pager);
    View.setWindowTitle(title);
    View.setWindowState(Qt::WindowMaximized);
    View.show();
    
    App.exec();
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

ScoreWidget::ScoreWidget(ScorePager &Pager) : QWidget(), m_pager(Pager)
{
    QObject::connect(&Pager, SIGNAL(updated()), this, SLOT(update()));
}

ScoreWidget::~ScoreWidget()
{
}

void ScoreWidget::keyReleaseEvent(QKeyEvent *e)
{
    if (e->matches(QKeySequence::MoveToNextPage))
    {
        m_pager.nextPage();
    }
    else if (e->matches(QKeySequence::MoveToPreviousPage))
    {
        m_pager.previousPage();
    }
}

void ScoreWidget::wheelEvent(QWheelEvent *e)
{
     m_pager.setScale(m_pager.scale() + e->delta() / 1200.0);
}

void ScoreWidget::resizeEvent(QResizeEvent *e)
{
    m_pager.setPageSize(QSizeF(e->size().width(), e->size().height()));
}

void ScoreWidget::paintEvent(QPaintEvent *e)
{
    if (!m_pager.score())
        return;
    
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    
    p.save();
    p.fillRect(e->rect(), QColor("white"));

    QList<Ms::Element*> items = m_pager.pageItems();
    for (const Ms::Element* e : items)
    {
        e->itemDiscovered = 0;
        if (!e->visible())
            continue;
        QPointF pos(e->pagePos());
        p.translate(pos);
        e->draw(&p);
        p.translate(-pos);
    }
    p.restore();
    
    p.setClipping(false);
}

////////////////////////////////////////////////////////////////////////////////

ScorePager::ScorePager(Ms::MScore &App, QObject *parent) : QObject(parent),
    m_app(App), m_score(nullptr)
{
    m_pageIdx = 0;
    m_scale = 1.0;
    m_pageSize = QSizeF(1920.0, 1080.0);
}

ScorePager::~ScorePager()
{
    delete m_score;
}

bool ScorePager::loadScore(QString path)
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
    Ms::MStyle *style = m_score->style();
    m_score->setLayoutMode(Ms::LayoutMode::PAGE);
    style->set(Ms::StyleIdx::showFooter, false);
    style->set(Ms::StyleIdx::showHeader, false);
    style->set(Ms::StyleIdx::showPageNumber, false);
    return true;
}

void ScorePager::setPageIndex(int newIndex)
{
    if (!m_score || (newIndex < 0) || (newIndex >= m_score->pages().size()))
        return;
    if (newIndex != m_pageIdx)
    {
        m_pageIdx = newIndex;
        emit updated();
    }
}

void ScorePager::setPageSize(QSizeF newSize)
{
    m_pageSize = newSize;
    updateLayout();
}

void ScorePager::setScale(double newScale)
{
    if (newScale <= 0.0)
        return;
    if (std::fabs(newScale - m_scale) > 1e-3)
    {
        m_scale = newScale;
        updateLayout();
    }
}

void ScorePager::updateLayout()
{
    double widthInch = m_pageSize.width() / Ms::MScore::DPI;
    double heightInch = m_pageSize.height() / Ms::MScore::DPI;
    double f  = 1.0 / Ms::INCH;
    double marginMm = 10.0;
    double staffSpaceMm = m_scale * 2.0;

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
    m_score->setLayoutAll(true);
    m_score->update();
    emit updated();
}

QList<Ms::Element *> ScorePager::pageItems()
{
    QList<Ms::Element*> items;
    if (m_score->pages().size() > m_pageIdx)
    {
        Ms::Page *page = m_score->pages().at(m_pageIdx);
        QRectF bounds = page->bbox();
        items = page->items(bounds);
        qStableSort(items.begin(), items.end(), Ms::elementLessThan);
    }
    return items;
}

void ScorePager::nextPage()
{
    setPageIndex(pageIndex() + 1);
}

void ScorePager::previousPage()
{
    setPageIndex(pageIndex() - 1);
}

void ScorePager::firstPage()
{
    setPageIndex(0);
}

void ScorePager::lastPage()
{
    if (m_score)
    {
        setPageIndex(m_score->pages().size() - 1);
    }
}
