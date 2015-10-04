#include <QAction>
#include <QPainter>
#include <QScreen>
#include <cmath>
#include "mreader.h"
#include "libmscore/element.h"
#include "libmscore/page.h"
#include "libmscore/system.h"

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
    if (!Pager.title().isEmpty()) {
        title += " - ";
        title += Pager.title();
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
    m_previousPage = new QAction("Previous page", this);
    m_nextPage = new QAction("Next page", this);
    m_firstPage = new QAction("First page", this);
    m_lastPage = new QAction("Last page", this);
    m_zoomIn = new QAction("Zoom in", this);
    m_zoomOut = new QAction("Zoom out", this);
    m_twoSided = new QAction("Two-sided layout", this);
    m_previousPage->setShortcut(QKeySequence::MoveToPreviousPage);
    m_nextPage->setShortcut(QKeySequence::MoveToNextPage);
    m_firstPage->setShortcut(QKeySequence::MoveToStartOfDocument);
    m_lastPage->setShortcut(QKeySequence::MoveToEndOfDocument);
    m_zoomIn->setShortcut(QKeySequence::ZoomIn);
    m_zoomOut->setShortcut(QKeySequence::ZoomOut);
    m_twoSided->setShortcut(QKeySequence(Qt::Key_T));
    m_twoSided->setCheckable(true);
    m_twoSided->setChecked(Pager.isTwoSided());
    addAction(m_previousPage);
    addAction(m_nextPage);
    addAction(m_firstPage);
    addAction(m_lastPage);
    addAction(m_zoomIn);
    addAction(m_zoomOut);
    addAction(m_twoSided);
    QObject::connect(m_previousPage, SIGNAL(triggered(bool)),
                     &Pager, SLOT(previousPage()));
    QObject::connect(m_nextPage, SIGNAL(triggered(bool)),
                     &Pager, SLOT(nextPage()));
    QObject::connect(m_firstPage, SIGNAL(triggered(bool)),
                     &Pager, SLOT(firstPage()));
    QObject::connect(m_lastPage, SIGNAL(triggered(bool)),
                     &Pager, SLOT(lastPage()));
    QObject::connect(m_zoomIn, SIGNAL(triggered(bool)),
                     &Pager, SLOT(zoomIn()));
    QObject::connect(m_zoomOut, SIGNAL(triggered(bool)),
                     &Pager, SLOT(zoomOut()));
    QObject::connect(m_twoSided, SIGNAL(triggered(bool)),
                     &Pager, SLOT(setTwoSided(bool)));
    QObject::connect(&Pager, SIGNAL(updated()), this, SLOT(update()));
}

ScoreWidget::~ScoreWidget()
{
}

void ScoreWidget::wheelEvent(QWheelEvent *e)
{
    int pages = -qRound(e->delta() / 120.0);
    for (int i = 0; i < qAbs(pages); i++)
    {
        if (pages > 0)
            m_pager.nextPage();
        else
            m_pager.previousPage();
    }
}

void ScoreWidget::resizeEvent(QResizeEvent *e)
{
    m_pager.setPageSize(QSizeF(e->size().width(), e->size().height()));
}

void ScoreWidget::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);    
    p.fillRect(e->rect(), QColor("white"));

    QVector<PageElement> items;
    QVector<QPointF> pageOffsets;
    m_pager.addPageItems(items, pageOffsets);
    for (const PageElement &pe : items)
    {
        Ms::Element *e = pe.E;
        e->itemDiscovered = 0;
        if (!e->visible())
            continue;
        if ((pe.PageIdx < 0) || (pe.PageIdx >= pageOffsets.size()))
            continue;
        QPointF pageOffset(pageOffsets[pe.PageIdx]);
        QPointF pos(pageOffset + e->pagePos());
        p.translate(pos);
        e->draw(&p);
        p.translate(-pos);
    }
    p.setClipping(false);
}

////////////////////////////////////////////////////////////////////////////////

ScorePager::ScorePager(Ms::MScore &App, QObject *parent) : QObject(parent),
    m_app(App), m_score(nullptr), m_workingScore(nullptr), m_twoSided(false)
{
    m_pageIdx = 0;
    m_scale = 1.0;
    m_pageSize = QSizeF(1920.0, 1080.0);
    m_twoSided = true;
}

ScorePager::~ScorePager()
{
    delete m_workingScore;
    delete m_score;
}

QString ScorePager::title() const
{
    if (m_score)
    {
        return m_score->title();
    }
    return QString();
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
    style->set(Ms::StyleIdx::hideInstrumentNameIfOneInstrument, true);
    m_workingScore = m_score->clone();
    m_workingScore->setShowVBox(false);
    m_workingScore->setShowUnprintable(false);
    m_workingScore->setShowInvisible(false);
    return true;
}

int ScorePager::numPhysPages() const
{
    if (m_workingScore)
        return m_workingScore->pages().size();
    return 0;
}

int ScorePager::numPages() const
{
    int pages = numPhysPages();
    return m_twoSided ? (pages + 1) / 2 : pages;
}

void ScorePager::setTwoSided(bool newVal)
{
    if (newVal != m_twoSided)
    {
        m_twoSided = newVal;
        updateLayout();
    }
}

void ScorePager::setPageIndex(int newIndex)
{
    if ((newIndex < 0) || (newIndex >= numPhysPages()))
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
    double twoSidedMod = m_twoSided ? 0.5 : 1.0;
    double widthInch = (m_pageSize.width() * twoSidedMod) / Ms::MScore::DPI;
    double heightInch = m_pageSize.height() / Ms::MScore::DPI;
    double marginVertInch = 0.005 * heightInch;
    double marginHorInch = 0.015 * widthInch / twoSidedMod;
    double marginHorInnerInch = marginHorInch * twoSidedMod;
    double staffSpaceInch = m_scale * 0.08;

    Ms::PageFormat pf;
    pf.setEvenTopMargin(marginVertInch);
    pf.setEvenBottomMargin(marginVertInch);
    pf.setEvenLeftMargin(marginHorInnerInch);
    pf.setOddTopMargin(marginVertInch);
    pf.setOddBottomMargin(marginVertInch);
    pf.setOddLeftMargin(marginHorInch);
    pf.setSize(QSizeF(widthInch, heightInch));
    pf.setPrintableWidth(widthInch - marginHorInch - marginHorInnerInch);
    pf.setTwosided(m_twoSided);
    //qDebug("Width %f PW %f ELM %f ERM %f OLM %f ORM %f",
    //       pf.width(), pf.printableWidth(),
    //       pf.evenLeftMargin(), pf.evenRightMargin(),
    //       pf.oddLeftMargin(), pf.oddRightMargin());

    m_workingScore->setPageFormat(pf);
    m_workingScore->style()->setSpatium(staffSpaceInch * Ms::MScore::DPI);
    m_workingScore->setPrinting(true);
    m_workingScore->setLayoutAll(true);
    m_workingScore->update();
    alignLastPageSystems();
    
    // Adjust the page index when the number of pages decreases.
    if (m_pageIdx >= m_workingScore->pages().size())
    {
        m_pageIdx = m_workingScore->pages().size() - 1;
    }
    
    emit updated();
}

void ScorePager::alignLastPageSystems()
{
    // We can only align a page's systems if there is a previous page.
    if (numPhysPages() < 2)
        return;
    
    // Determine the previous page's system distances.
    Ms::Page *prevPage = m_workingScore->pages().value(numPhysPages() - 2);
    QVector<qreal> prevDistances;
    for (Ms::System *sys : *prevPage->systems())
    {
        prevDistances.append(sys->addStretch() ? sys->stretchDistance() : 0.0);
    }
    
    // Align all systems in the page.
    int i = 0;
    qreal yOffset = 0;
    Ms::Page *page = m_workingScore->pages().value(numPhysPages() - 1);
    for (Ms::System *sys : *page->systems())
    {
        // Apply any vertical offset.
        sys->move(QPointF(0.0, yOffset));
        
        // Reduce the system's stretch distance if it is larger than the
        // corresponding system on the previous page.
        if (i >= prevDistances.size())
            break;
        qreal prevDist = prevDistances[i];
        qreal dist = sys->addStretch() ? sys->stretchDistance() : 0.0;
        if (dist > prevDist)
        {
            // Stretch is added after a system, it affects following systems.
            sys->setStretchDistance(prevDist);
            yOffset += (prevDist - dist);
        }
        
        i++;
    }
}

static bool pageElementLessThan(const PageElement &e1, const PageElement &e2)
{
    return Ms::elementLessThan(e1.E, e2.E) && (e1.PageIdx < e2.PageIdx);
}

void ScorePager::addPageItems(QVector<PageElement> &elements,
                              QVector<QPointF> &pageOffets)
{
    pageOffets.clear();
    if (m_workingScore)
    {
        QPointF pageOffset(0.0, 0.0);
        Ms::Page *prevPage = nullptr;
        for (int i = 0; i < numPagesShown(); i++)
        {
            Ms::Page *page = m_workingScore->pages().value(m_pageIdx + i);
            if (!page)
                break;
            if (prevPage)
                pageOffset += QPointF(page->bbox().size().width(), 0.0);
            QRectF bounds = page->bbox();
            QList<Ms::Element *> items = page->items(bounds);
            for(Ms::Element *el : items)
            {
                elements.push_back(PageElement(el, i));
            }
            pageOffets.append(pageOffset);
            prevPage = page;
        }
        qStableSort(elements.begin(), elements.end(), pageElementLessThan);
    }
}

void ScorePager::nextPage()
{
    setPageIndex(pageIndex() + numPagesShown());
}

void ScorePager::previousPage()
{
    setPageIndex(pageIndex() - numPagesShown());
}

void ScorePager::firstPage()
{
    setPageIndex(0);
}

void ScorePager::lastPage()
{
    int pages = numPhysPages();
    int lastPageIndex = pages - 1;
    if (m_twoSided)
        lastPageIndex = (lastPageIndex / numPagesShown()) * numPagesShown();
    setPageIndex(lastPageIndex);
}

void ScorePager::zoomIn()
{
    setScale(scale() + 0.1);
}

void ScorePager::zoomOut()
{
    setScale(scale() - 0.1);
}
