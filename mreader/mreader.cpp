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
    m_pageIdx = 1;
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
    
    {
        double width = 192.0;
        double height = 108.0;
        double f  = 1.0/Ms::INCH;
        double marginMm = 10.0;
        double staffSpace = 1.5;
  
        Ms::PageFormat pf;
        pf.setEvenTopMargin(marginMm * f);
        pf.setEvenBottomMargin(marginMm * f);
        pf.setEvenLeftMargin(marginMm * f);
        pf.setOddTopMargin(marginMm * f);
        pf.setOddBottomMargin(marginMm * f);
        pf.setOddLeftMargin(marginMm * f);
        pf.setSize(QSizeF(width, height) * f);
        pf.setPrintableWidth((width - marginMm * 2.0)  * f);
        pf.setTwosided(false);
  
        m_score->setPageFormat(pf);
        m_score->style()->setSpatium(staffSpace * f * Ms::MScore::DPI);
        m_score->setPrinting(true);
        m_score->update();
    }
    
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

static void addElementToList(void *data, Ms::Element *el)
{
    if (!data)
        return;
    QList<Ms::Element*> *ell = reinterpret_cast<QList<Ms::Element*> *>(data);
    ell->append(el);
}

void ScoreWidget::paint(const QRect& r, QPainter& p)
{
    p.save();
    p.fillRect(r, QColor("white"));
    
    p.setTransform(m_matrix);

    qDebug("Pages: %d",  m_score->pages().size());
    if (m_score->pages().size() > m_pageIdx)
    {
        Ms::Page *page = m_score->pages().at(m_pageIdx);
        QRectF bounds = page->abbox();
        QList<Ms::Element*> ell = page->items(bounds);
        page->scanElements(&ell, addElementToList);
        unsigned numSystems = page->systems()->size();
        qDebug("Systems: %d, elements: %d W: %f, H: %f", numSystems, ell.size(),
               bounds.width(), bounds.height());
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
