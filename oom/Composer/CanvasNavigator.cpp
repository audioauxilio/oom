#include <QtGui>
#include "CanvasNavigator.h"
#include "ComposerCanvas.h"
#include "config.h"
#include "globals.h"
#include "gconfig.h"
#include "song.h"
#include "track.h"
#include "part.h"

static const int MIN_PART_HEIGHT = 1;
static const int PIXEL_PER_BAR = 4;

CanvasNavigator::CanvasNavigator(QWidget* parent)
: QWidget(parent)
{
	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setContentsMargins(0,0,0,0);
	layout->setSpacing(0);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setFixedHeight(48);

	m_scene = new QGraphicsScene(0, 0, 400, 48);
	m_scene->setBackgroundBrush(QColor(23,23,23));
	//QGraphicsRectItem *item = new QGraphicsRectItem(0, m_scene);
	//item->setRect(20, 20, 40, 20);
	QGraphicsRectItem* item = m_scene->addRect(24, 24, 150, 1);
	item->setBrush(config.partColors[1]);
	item->setPen(Qt::NoPen);
	m_view = new QGraphicsView(m_scene);
	m_view->setRenderHints(QPainter::Antialiasing);
	m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	layout->addWidget(m_view);
}

void CanvasNavigator::setCanvas(ComposerCanvas* c)
{
	m_canvas = c;
	//TODO: Update all parts
}

void CanvasNavigator::updateParts()
{
	m_tracks.clear();
	m_scene->clear();
	int partHeight = 4;
	//if(partHeight < MIN_PART_HEIGHT)
	//	partHeight = MIN_PART_HEIGHT;
	int index = 0;
	TrackList* tl = song->visibletracks();
	for(ciTrack ci = tl->begin(); ci != tl->end(); ci++)
	{
		if((*ci)->type() == Track::WAVE || (*ci)->type() == Track::MIDI)
		{
			m_tracks.append((*ci)->id());

			Track* track = *ci;
			if(track)
			{
				PartList* parts = track->parts();
				if(parts)
				{
					for(iPart p = parts->begin(); p != parts->end(); p++)
					{
						Part *part =  p->second;

						int tick, len;
						if(part && track->isMidiTrack())
						{
							tick = part->tick();
							len = part->lenTick();
						}
						else
						{
							tick = tempomap.frame2tick(part->frame());
							len = tempomap.frame2tick(part->lenFrame());
						}
						/*int tenp = (len * 8)/100;
						int startpos = m_canvas->mapx(tick);
						int pos = (startpos * 8)/100;*/
						int w = m_canvas->mapx(len)+m_canvas->xOffsetDev();
						int pos = m_canvas->mapx(tick)+m_canvas->xOffsetDev();
						w = (w * 8)/100;
						pos = (pos * 8)/100;
						//QGraphicsRectItem* item = m_scene->addRect(pos, index*partHeight, w, partHeight);
						QGraphicsRectItem* item = m_scene->addRect(pos, index*partHeight, w, partHeight);
						item->setBrush(config.partColors[part->colorIndex()]);
						item->setPen(Qt::NoPen);
						qDebug("CanvasNavigator::updateParts: tick: %d, len: %d , pos: %d, w: %d", tick, len, pos, w);
					}
				}
				++index;
			}
		}
	}
	QRectF bounds = m_scene->itemsBoundingRect();
	m_view->fitInView(bounds, Qt::KeepAspectRatio);
}

void CanvasNavigator::resizeEvent(QResizeEvent*)
{
}