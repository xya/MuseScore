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

#include <QAction>
#include <QPainter>
#include <QScreen>
#include <QTextStream>
#include <cmath>
#include "mreader.h"
#include "libmscore/chordrest.h"
#include "libmscore/element.h"
#include "libmscore/excerpt.h"
#include "libmscore/instrument.h"
#include "libmscore/lyrics.h"
#include "libmscore/measure.h"
#include "libmscore/page.h"
#include "libmscore/part.h"
#include "libmscore/staff.h"
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
    
    ScoreWidget View(Pager);
    View.setWindowState(Qt::WindowMaximized);
    View.show();
    
    App.exec();
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

ScoreWidget::ScoreWidget(ScorePager &Pager) : QWidget(), m_pager(Pager)
{
    m_lastScreenState = windowState();
    
    // Set up pager actions.
    auto prevPageKeys = {QKeySequence::MoveToPreviousPage,
                         QKeySequence::MoveToPreviousChar};
    auto nextPageKeys = {QKeySequence::MoveToNextPage,
                         QKeySequence::MoveToNextChar};
    m_previousPart = pagerAction("Previous part",
                                 QKeySequence::MoveToPreviousWord,
                                 SLOT(previousPart()));
    m_nextPart = pagerAction("Next part", QKeySequence::MoveToNextWord,
                             SLOT(nextPart()));
    m_prevPage = pagerAction("Previous page", prevPageKeys, SLOT(previousPage()));
    m_nextPage = pagerAction("Next page", nextPageKeys, SLOT(nextPage()));
    m_firstPage = pagerAction("First page", QKeySequence::MoveToStartOfDocument,
                              SLOT(firstPage()));
    m_lastPage = pagerAction("Last page", QKeySequence::MoveToEndOfDocument,
                             SLOT(lastPage()));
    m_zoomIn = pagerAction("Zoom in", QKeySequence::ZoomIn, SLOT(zoomIn()));
    m_zoomOut = pagerAction("Zoom out", QKeySequence::ZoomOut, SLOT(zoomOut()));
    m_twoSided = pagerAction("Two-sided layout", QKeySequence(Qt::Key_T),
                             Pager.isTwoSided(), SLOT(setTwoSided(bool)));
    m_concertPitch = pagerAction("Concert pitch", QKeySequence(Qt::Key_C),
                                 Pager.concertPitch(), SLOT(setConcertPitch(bool)));
    m_showLyrics = pagerAction("Show lyrics", QKeySequence(Qt::Key_L),
                               Pager.showLyrics(), SLOT(setShowLyrics(bool)));
    m_showInstrumentNames = pagerAction("Show instrument names",
                                        QKeySequence(Qt::Key_I),
                                        Pager.showInstrumentNames(),
                                        SLOT(setShowInstrumentNames(bool)));
    
    // Set up fullscreen action.
    m_fullscreen = new QAction("Fullscreen", this);
    m_fullscreen->setShortcut(QKeySequence(Qt::Key_F));
    m_fullscreen->setCheckable(true);
    m_fullscreen->setChecked(m_lastScreenState == Qt::WindowFullScreen);
    addAction(m_fullscreen);
    QObject::connect(m_fullscreen, SIGNAL(triggered(bool)),
                     this, SLOT(setFullscreen(bool)));
    
    // Other connections.
    QObject::connect(&Pager, SIGNAL(updated()), this, SLOT(update()));
    QObject::connect(&Pager, SIGNAL(partChanged()), this, SLOT(updateTitle()));
    
    updateTitle();
}

ScoreWidget::~ScoreWidget()
{
}

QAction * ScoreWidget::pagerAction(QString text, QKeySequence key,
                                   const char *slotName)
{
    QAction *a = new QAction(text, this);
    a->setShortcut(key);
    addAction(a);
    if (slotName)
    {
        QObject::connect(a, SIGNAL(triggered(bool)), &m_pager, slotName);
    }
    return a;
}

QAction * ScoreWidget::pagerAction(QString text, KeyIList &keys,
                                   const char *slotName)
{
    QAction *a = pagerAction(text, QKeySequence(), slotName);
    QList<QKeySequence> keyList;
    for (auto key : keys)
        keyList.append(QKeySequence(key));
    a->setShortcuts(keyList);
    return a;
}

QAction * ScoreWidget::pagerAction(QString text, QKeySequence key,
                                   bool checked, const char *slotName)
{
    QAction *a = new QAction(text, this);
    a->setShortcut(key);
    a->setCheckable(true);
    a->setChecked(checked);
    addAction(a);
    if (slotName)
    {
        QObject::connect(a, SIGNAL(triggered(bool)), &m_pager, slotName);
    }
    return a;
}

void ScoreWidget::setFullscreen(bool newVal)
{
    if (newVal)
    {
        m_lastScreenState = windowState();
        setWindowState(Qt::WindowFullScreen);
    }
    else
    {
        setWindowState(m_lastScreenState);
    }
    m_fullscreen->setChecked(newVal);
}

void ScoreWidget::updateTitle()
{
    QString scoreTitle = m_pager.title();
    QString partName = m_pager.partName();
    QString title = "MuseReader";
    if (!scoreTitle.isEmpty())
    {
        title += " - ";
        title += scoreTitle;
        if (!partName.isEmpty())
        {
            title += " [";
            title += partName;
            title += "]";
        }
    }
    setWindowTitle(title);
}

void ScoreWidget::mouseDoubleClickEvent(QMouseEvent *)
{
    setFullscreen(!isFullscreen());
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
    m_app(App), m_twoSided(true), m_alignSystems(true),
    m_showInstrumentNames(true), m_soloInstrument(false), m_showLyrics(true)
{
    m_partIdx = -1;
    m_pageIdx = 0;
    m_scale = 1.0;
    m_pageSize = QSizeF(1920.0, 1080.0);
}

ScorePager::~ScorePager()
{
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
    std::unique_ptr<Ms::Score> score(new Ms::Score(m_app.baseStyle()));
    QFileInfo fi(path);
    score->setName(fi.completeBaseName());

    Ms::Score::FileError rv = score->loadMsc(path, false);
    if (rv != Ms::Score::FileError::FILE_NO_ERROR)
    {
        return false;
    }
    m_score.swap(score);
    m_concertPitch = m_score->styleB(Ms::StyleIdx::concertPitch);
    loadPart(m_partIdx);
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

void ScorePager::setConcertPitch(bool newVal)
{
    if (newVal != m_concertPitch)
    {
        m_concertPitch = newVal;
        loadPart(m_partIdx);
    }
}

void ScorePager::setShowLyrics(bool newVal)
{
    if (newVal != m_showLyrics)
    {
        m_showLyrics = newVal;
        loadPart(m_partIdx);
    }
}

void ScorePager::setShowInstrumentNames(bool newVal)
{
    if (newVal != m_showInstrumentNames)
    {
        m_showInstrumentNames = newVal;
        updateStyle();
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

void ScorePager::setPartIndex(int newIndex)
{
    if (newIndex != m_partIdx)
    {
        loadPart(newIndex);
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

void ScorePager::loadPart(int partIdx)
{
    Ms::Score *base = m_score.get();
    if (!base)
        return;
    int numExcerpts = base->excerpts().size();
    int numParts = base->parts().size();
    if ((numExcerpts > 0) && (partIdx >= 0))
    {
        // Excerpts are called "Parts" in the MuseScore GUI.
        // They can include multiple instruments.
        m_partIdx = qMin(partIdx, numExcerpts - 1);
        Ms::Excerpt *part = base->excerpts().value(m_partIdx);
        base = part->partScore();
        m_partName = part->title();
        m_soloInstrument = false;
    }
    else if ((numParts > 0) && (partIdx >= 0))
    {
        // Parts are called "Instruments" in the MuseScore GUI.
        // They can include multiple staves.
        m_partIdx = qMin(partIdx, numParts - 1);
        Ms::Part *part = base->parts().value(m_partIdx);
        m_partName = part->partName();
        m_soloInstrument = true;
    }
    else
    {
        m_partIdx = -1;
        m_partName.clear();
        m_soloInstrument = false;
    }
    m_workingScore.reset(base->clone());
    if (m_soloInstrument)
    {
        // Show a single instrument as a part by hiding other instruments.
        QList<Ms::Part *> &parts = m_workingScore->parts();
        for (int i = 0; i < parts.size(); i++)
        {
            Ms::Part *part = parts[i];
            part->setShow(i == m_partIdx);
        }
    }

    // Hide lyrics.
    if (!m_showLyrics)
    {
        removeLyrics(m_workingScore.get());
    }
    
    // Transpose key signatures for concert pitch.
    bool scoreConcertPitch = m_workingScore->styleB(Ms::StyleIdx::concertPitch);
    if (scoreConcertPitch != m_concertPitch)
    {
        transposeKeySignatures(m_workingScore.get(), scoreConcertPitch);
    }
    
    emit partChanged();
    updateStyle();
}

void ScorePager::removeLyrics(Ms::Score *score)
{
    for (Ms::Measure *m = score->firstMeasure(); m; m = m->nextMeasure())
    {
        for (Ms::Segment *seg = m->first(); seg; seg = seg->next())
        {
            for (Ms::Element *el : seg->elist())
            {
                if (!el || !el->isChordRest())
                    continue;
                Ms::ChordRest *cr = static_cast<Ms::ChordRest *>(el);
                for (Ms::Lyrics *lyrics : cr->lyricsList())
                {
                    if (!lyrics)
                        continue;
                    cr->remove(lyrics);
                }
            }
        }
    }
}

void ScorePager::transposeKeySignatures(Ms::Score *score, bool flip)
{
    for (Ms::Part *part : score->parts())
    {
        Ms::Instrument *instr = part->instrument();
        Ms::Interval interval = instr->transpose();
        if (interval.isZero())
            continue;
        if (flip)
            interval.flip();
        int tickEnd = score->lastSegment() ? score->lastSegment()->tick() : 0;
        score->transposeKeys(part->startTrack() / Ms::VOICES,
                             part->endTrack() / Ms::VOICES, 0, tickEnd,
                             interval);
    }
}

void ScorePager::updateStyle()
{
    if (!m_workingScore)
        return;
    Ms::MStyle *style = m_workingScore->style();
    m_workingScore->setLayoutMode(Ms::LayoutMode::PAGE);
    style->set(Ms::StyleIdx::showFooter, false);
    style->set(Ms::StyleIdx::showHeader, false);
    style->set(Ms::StyleIdx::showPageNumber, false);
    style->set(Ms::StyleIdx::hideInstrumentNameIfOneInstrument, true);
    style->set(Ms::StyleIdx::concertPitch, m_concertPitch);
    m_workingScore->setShowVBox(false);
    m_workingScore->setShowUnprintable(false);
    m_workingScore->setShowInvisible(false);
    m_workingScore->setShowInstrumentNames(m_showInstrumentNames &&
                                           !m_soloInstrument);
    updateLayout();
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
    if (m_alignSystems)
    {
        alignLastPageSystems();
    }
    
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
    
    // Collect the previous page's systems.
    Ms::Page *prevPage = m_workingScore->pages().value(numPhysPages() - 2);
    QList<Ms::System *> &prevSystems = *prevPage->systems();
    
    // Align all systems in the page.
    int i = 0;
    Ms::Page *page = m_workingScore->pages().value(numPhysPages() - 1);
    for (Ms::System *sys : *page->systems())
    {
        Ms::System *prevSys = prevSystems.value(i);
        if (!prevSys)
            break;
        sys->rypos() = prevSys->rypos();
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

void ScorePager::nextPart()
{
    setPartIndex(partIndex() + 1);
}

void ScorePager::previousPart()
{
    setPartIndex(partIndex() - 1);
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
