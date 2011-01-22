//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: ecanvas.cpp,v 1.8.2.6 2009/05/03 04:14:00 terminator356 Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include <errno.h>
#include <values.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <QKeyEvent>
#include <QDropEvent>
#include <QEvent>
#include <QMimeData>
#include <QByteArray>
#include <QDrag>

#include "xml.h"
#include "midieditor.h"
#include "ecanvas.h"
#include "song.h"
#include "event.h"
#include "shortcuts.h"
#include "audio.h"

//---------------------------------------------------------
//   EventCanvas
//---------------------------------------------------------

EventCanvas::EventCanvas(MidiEditor* pr, QWidget* parent, int sx,
		int sy, const char* name)
: Canvas(parent, sx, sy, name)
{
	editor = pr;
	_steprec = false;
	_midiin = false;
	_playEvents = false;
	curVelo = 70;

	setBg(QColor(226, 229, 229));
	setAcceptDrops(true);
	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);

        _curPart = (MidiPart*) (editor->parts()->begin()->second);
        _curPartId = _curPart->sn();
}

//---------------------------------------------------------
//   getCaption
//---------------------------------------------------------

QString EventCanvas::getCaption() const
{
	int bar1, bar2, xx;
	unsigned x;
	///sigmap.tickValues(curPart->tick(), &bar1, &xx, &x);
        AL::sigmap.tickValues(_curPart->tick(), &bar1, &xx, &x);
	///sigmap.tickValues(curPart->tick() + curPart->lenTick(), &bar2, &xx, &x);
        AL::sigmap.tickValues(_curPart->tick() + _curPart->lenTick(), &bar2, &xx, &x);

        return QString("OOMidi: Part <") + _curPart->name()
			+ QString("> %1-%2").arg(bar1 + 1).arg(bar2 + 1);
}

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void EventCanvas::leaveEvent(QEvent*)
{
	emit pitchChanged(-1);
	emit timeChanged(MAXINT);
}

//---------------------------------------------------------
//   enterEvent
//---------------------------------------------------------

void EventCanvas::enterEvent(QEvent*)
{
	emit enterCanvas();
}

//---------------------------------------------------------
//   raster
//---------------------------------------------------------

QPoint EventCanvas::raster(const QPoint& p) const
{
	int x = p.x();
	if (x < 0)
		x = 0;
	x = editor->rasterVal(x);
	int pitch = y2pitch(p.y());
	int y = pitch2y(pitch);
	return QPoint(x, y);
}

//---------------------------------------------------------
//   startUndo
//---------------------------------------------------------

void EventCanvas::startUndo(DragType)
{
	song->startUndo();
}

//---------------------------------------------------------
//   endUndo
//---------------------------------------------------------

void EventCanvas::endUndo(DragType dtype, int flags)
{
	song->endUndo(flags | ((dtype == MOVE_COPY || dtype == MOVE_CLONE)
			? SC_EVENT_INSERTED : SC_EVENT_MODIFIED));
}

//---------------------------------------------------------
//   mouseMove
//---------------------------------------------------------

void EventCanvas::mouseMove(const QPoint& pos)
{
	emit pitchChanged(y2pitch(pos.y()));
	int x = pos.x();
	emit timeChanged(editor->rasterVal(x));
}

//---------------------------------------------------------
//   updateSelection
//---------------------------------------------------------

void EventCanvas::updateSelection()
{
	song->update(SC_SELECTION);
}

//---------------------------------------------------------
//   songChanged(type)
//---------------------------------------------------------

void EventCanvas::songChanged(int flags)
{
	// Is it simply a midi controller value adjustment? Forget it.
	if (flags == SC_MIDI_CONTROLLER)
		return;

	if (flags & ~SC_SELECTION)
	{
                _items.clear();
		start_tick = MAXINT;
		end_tick = 0;
                _curPart = 0;
		for (iPart p = editor->parts()->begin(); p != editor->parts()->end(); ++p)
		{
			MidiPart* part = (MidiPart*) (p->second);
                        if (part->sn() == _curPartId)
                                _curPart = part;
			unsigned stick = part->tick();
			unsigned len = part->lenTick();
			unsigned etick = stick + len;
			if (stick < start_tick)
				start_tick = stick;
			if (etick > end_tick)
				end_tick = etick;

			EventList* el = part->events();
			for (iEvent i = el->begin(); i != el->end(); ++i)
			{
				Event e = i->second;
				// Added by T356. Do not add events which are either past, or extend past the end of the part.
				//if(e.tick() > len)
				if (e.endTick() > len)
					break;

				if (e.isNote())
				{
                                        addItem(part, e);
				}
			}
		}
	}

	Event event;
	MidiPart* part = 0;
	int x = 0;
	CItem* nevent = 0;

	int n = 0; // count selections
        for (iCItem k = _items.begin(); k != _items.end(); ++k)
	{
		Event ev = k->second->event();
		bool selected = ev.selected();
		if (selected)
		{
			k->second->setSelected(true);
			++n;
			if (!nevent)
			{
				nevent = k->second;
				Event mi = nevent->event();
				curVelo = mi.velo();
			}
		}
	}
	start_tick = song->roundDownBar(start_tick);
	end_tick = song->roundUpBar(end_tick);

	if (n == 1)
	{
		x = nevent->x();
		event = nevent->event();
		part = (MidiPart*) nevent->part();
                if (_curPart != part)
		{
                        _curPart = part;
                        _curPartId = _curPart->sn();
			curPartChanged();
		}
	}
	emit selectionChanged(x, event, part);
        if (_curPart == 0)
                _curPart = (MidiPart*) (editor->parts()->begin()->second);
	redraw();
}

//---------------------------------------------------------
//   selectAtTick
//---------------------------------------------------------

void EventCanvas::selectAtTick(unsigned int tick)
{
        CItemList list = getItemlistForCurrentPart();

        //Select note nearest tick, if none selected and there are any
        if (!list.empty() && selectionSize() == 0)
	{
                iCItem i = list.begin();
		CItem* nearest = i->second;

                while (i != list.end())
		{
			CItem* cur = i->second;
			unsigned int curtk = abs(cur->x() + cur->part()->tick() - tick);
			unsigned int neartk = abs(nearest->x() + nearest->part()->tick() - tick);

			if (curtk < neartk)
			{
				nearest = cur;
			}

			i++;
		}

                if (!nearest->isSelected())
		{
			selectItem(nearest, true);
			songChanged(SC_SELECTION);
		}
	}
}

//---------------------------------------------------------
//   track
//---------------------------------------------------------

MidiTrack* EventCanvas::track() const
{
        return ((MidiPart*) _curPart)->track();
}


//---------------------------------------------------------
//   keyPress
//---------------------------------------------------------

void EventCanvas::keyPress(QKeyEvent* event)
{
	int key = event->key();
	///if (event->state() & Qt::ShiftButton)
	if (((QInputEvent*) event)->modifiers() & Qt::ShiftModifier)
		key += Qt::SHIFT;
	///if (event->state() & Qt::AltButton)
	if (((QInputEvent*) event)->modifiers() & Qt::AltModifier)
		key += Qt::ALT;
	///if (event->state() & Qt::ControlButton)
	if (((QInputEvent*) event)->modifiers() & Qt::ControlModifier)
		key += Qt::CTRL;

	//
	//  Shortcut for DrumEditor & PianoRoll
	//  Sets locators to selected events
	//
	if (key == shortcuts[SHRT_LOCATORS_TO_SELECTION].key)
	{
		int tick_max = 0;
		int tick_min = INT_MAX;
		bool found = false;

                for (iCItem i = _items.begin(); i != _items.end(); i++)
		{
			if (!i->second->isSelected())
				continue;

			int tick = i->second->x();
			int len = i->second->event().lenTick();
			found = true;
			if (tick + len > tick_max)
				tick_max = tick + len;
			if (tick < tick_min)
				tick_min = tick;
		}
		if (found)
		{
			Pos p1(tick_min, true);
			Pos p2(tick_max, true);
			song->setPos(1, p1);
			song->setPos(2, p2);
		}
	}
		// Select items by key (PianoRoll & DrumEditor)
	else if (key == shortcuts[SHRT_SEL_RIGHT].key || key == shortcuts[SHRT_SEL_RIGHT_ADD].key)
	{
                if (key == shortcuts[SHRT_SEL_RIGHT].key && allItemsAreSelected())
                {
                        deselectAll();
                        selectAtTick(song->cpos());
                        return;
                }

		iCItem i, iRightmost;
		CItem* rightmost = NULL;

                // get a list of items that belong to the current part
                // since multiple parts have populated the _items list
                // we need to filter on the actual current Part!
                CItemList list = getItemlistForCurrentPart();

                //Get the rightmost selected note (if any)
                i = list.begin();
                while (i != list.end())
		{
			if (i->second->isSelected())
			{
				iRightmost = i;
				rightmost = i->second;
			}

                        ++i;
		}
		if (rightmost)
		{
			iCItem temp = iRightmost;
			temp++;
			//If so, deselect current note and select the one to the right
                        if (temp != list.end())
			{
				if (key != shortcuts[SHRT_SEL_RIGHT_ADD].key)
					deselectAll();

				iRightmost++;
				iRightmost->second->setSelected(true);
				updateSelection();
			}
                } else // there was no item selected at all? Then select nearest to tick if there is any
                {
                        selectAtTick(song->cpos());
                }
	}
		//Select items by key: (PianoRoll & DrumEditor)
	else if (key == shortcuts[SHRT_SEL_LEFT].key || key == shortcuts[SHRT_SEL_LEFT_ADD].key)
	{
                if (key == shortcuts[SHRT_SEL_LEFT].key && allItemsAreSelected())
                {
                        deselectAll();
                        selectAtTick(song->cpos());
                        return;
                }

		iCItem i, iLeftmost;
                CItem* leftmost = NULL;

                // get a list of items that belong to the current part
                // since multiple parts have populated the _items list
                // we need to filter on the actual current Part!
                CItemList list = getItemlistForCurrentPart();

                if (list.size() > 0)
		{
                        i = list.end();
                        while (i != list.begin())
			{
                                --i;

				if (i->second->isSelected())
				{
					iLeftmost = i;
					leftmost = i->second;
				}
			}
			if (leftmost)
			{
                                if (iLeftmost != list.begin())
                                {
                                        //Add item
                                        if (key != shortcuts[SHRT_SEL_LEFT_ADD].key)
                                                deselectAll();

                                        iLeftmost--;
                                        iLeftmost->second->setSelected(true);
                                        updateSelection();
                                } else {
                                        leftmost->setSelected(true);
                                        updateSelection();
                                }
                        } else // there was no item selected at all? Then select nearest to tick if there is any
                        {
                                selectAtTick(song->cpos());
                        }
                }
	}
	else if (key == shortcuts[SHRT_INC_PITCH].key)
	{
		modifySelected(NoteInfo::VAL_PITCH, 1);
	}
	else if (key == shortcuts[SHRT_DEC_PITCH].key)
	{
		modifySelected(NoteInfo::VAL_PITCH, -1);
	}
	else if (key == shortcuts[SHRT_INC_POS].key)
	{
		// TODO: Check boundaries
		modifySelected(NoteInfo::VAL_TIME, editor->raster());
	}
	else if (key == shortcuts[SHRT_DEC_POS].key)
	{
		// TODO: Check boundaries
		modifySelected(NoteInfo::VAL_TIME, 0 - editor->raster());
	}

	else if (key == shortcuts[SHRT_INCREASE_LEN].key)
	{
		// TODO: Check boundaries
		modifySelected(NoteInfo::VAL_LEN, editor->raster());
	}
	else if (key == shortcuts[SHRT_DECREASE_LEN].key)
	{
		// TODO: Check boundaries
		modifySelected(NoteInfo::VAL_LEN, 0 - editor->raster());
	}
        else if (key == shortcuts[SHRT_GOTO_SEL_NOTE].key)
        {
                CItem* leftmost = getLeftMostSelected();
                if (leftmost)
                {
                        unsigned newtick = leftmost->event().tick() + leftmost->part()->tick();
                        Pos p1(newtick, true);
                        song->setPos(0, p1, true, true, false);
                }
        }
        else if (key == shortcuts[SHRT_TRACK_TOGGLE_SOLO].key)
        {
                if (_curPart) {
                        Track* t = _curPart->track();
                        audio->msgSetSolo(t, !t->solo());
                        song->update(SC_SOLO);
                }
        }
        else if (key == shortcuts[SHRT_TRACK_TOGGLE_MUTE].key)
        {
                if (_curPart) {
                        Track* t = _curPart->track();
                        t->setMute(!t->mute());
                        song->update(SC_MUTE);
                }
                return;
        }
        else
		event->ignore();
}

//---------------------------------------------------------
//   getTextDrag
//---------------------------------------------------------

//QDrag* EventCanvas::getTextDrag(QWidget* parent)

QMimeData* EventCanvas::getTextDrag()
{
	//---------------------------------------------------
	//   generate event list from selected events
	//---------------------------------------------------

	EventList el;
	unsigned startTick = MAXINT;
        for (iCItem i = _items.begin(); i != _items.end(); ++i)
	{
		if (!i->second->isSelected())
			continue;
		///NEvent* ne = (NEvent*)(i->second);
		CItem* ne = i->second;
		Event e = ne->event();
		if (startTick == MAXINT)
			startTick = e.tick();
		el.add(e);
	}

	//---------------------------------------------------
	//    write events as XML into tmp file
	//---------------------------------------------------

	FILE* tmp = tmpfile();
	if (tmp == 0)
	{
		fprintf(stderr, "EventCanvas::getTextDrag() fopen failed: %s\n",
				strerror(errno));
		return 0;
	}
	Xml xml(tmp);

	int level = 0;
	xml.tag(level++, "eventlist");
	for (ciEvent e = el.begin(); e != el.end(); ++e)
		e->second.write(level, xml, -startTick);
	xml.etag(--level, "eventlist");

	//---------------------------------------------------
	//    read tmp file into drag Object
	//---------------------------------------------------

	fflush(tmp);
	struct stat f_stat;
	if (fstat(fileno(tmp), &f_stat) == -1)
	{
		fprintf(stderr, "PianoCanvas::copy() fstat failes:<%s>\n",
				strerror(errno));
		fclose(tmp);
		return 0;
	}
	int n = f_stat.st_size;
	char* fbuf = (char*) mmap(0, n + 1, PROT_READ | PROT_WRITE,
			MAP_PRIVATE, fileno(tmp), 0);
	fbuf[n] = 0;

	QByteArray data(fbuf);
	QMimeData* md = new QMimeData();
	//QDrag* drag = new QDrag(parent);

	md->setData("text/x-oom-eventlist", data);
	//drag->setMimeData(md);

	munmap(fbuf, n);
	fclose(tmp);

	//return drag;
	return md;
}

//---------------------------------------------------------
//   pasteAt
//---------------------------------------------------------

void EventCanvas::pasteAt(const QString& pt, int pos)
{
	QByteArray ba = pt.toLatin1();
	const char* p = ba.constData();
	Xml xml(p);
	for (;;)
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::TagStart:
				if (tag == "eventlist")
				{
					song->startUndo();
					EventList* el = new EventList();
					el->read(xml, "eventlist", true);
					int modified = SC_EVENT_INSERTED;
					for (iEvent i = el->begin(); i != el->end(); ++i)
					{
						Event e = i->second;
                                                int tick = e.tick() + pos - _curPart->tick();
						if (tick < 0)
						{
							printf("ERROR: trying to add event before current part!\n");
							song->endUndo(SC_EVENT_INSERTED);
							delete el;
							return;
						}

						e.setTick(tick);
                                                int diff = e.endTick() - _curPart->lenTick();
						if (diff > 0)
						{// too short part? extend it
                                                        Part* newPart = _curPart->clone();
							newPart->setLenTick(newPart->lenTick() + diff);
							// Indicate no undo, and do port controller values but not clone parts.
                                                        audio->msgChangePart(_curPart, newPart, false, true, false);
							modified = modified | SC_PART_MODIFIED;
                                                        _curPart = newPart; // reassign
						}
						// Indicate no undo, and do not do port controller values and clone parts.
                                                audio->msgAddEvent(e, _curPart, false, false, false);
					}
					song->endUndo(modified);
					delete el;
					return;
				}
				else
					xml.unknown("pasteAt");
				break;
			case Xml::Attribut:
			case Xml::TagEnd:
			default:
				break;
		}
	}
}

//---------------------------------------------------------
//   dropEvent
//---------------------------------------------------------

void EventCanvas::viewDropEvent(QDropEvent* event)
{
	QString text;
	if (event->source() == this)
	{
		printf("local DROP\n"); // REMOVE Tim
		//event->acceptProposedAction();
		//event->ignore();                     // TODO CHECK Tim.
		return;
	}
	if (event->mimeData()->hasFormat("text/x-oom-eventlist"))
	{
		text = QString(event->mimeData()->data("text/x-oom-eventlist"));

		int x = editor->rasterVal(event->pos().x());
		if (x < 0)
			x = 0;
		pasteAt(text, x);
		//event->accept();  // TODO
	}
	else
	{
		printf("cannot decode drop\n");
		//event->acceptProposedAction();
		//event->ignore();                     // TODO CHECK Tim.
	}
}

CItem* EventCanvas::getRightMostSelected()
{
        iCItem i, iRightmost;
        CItem* rightmost = NULL;

        // get a list of items that belong to the current part
        // since multiple parts have populated the _items list
        // we need to filter on the actual current Part!
        CItemList list = getItemlistForCurrentPart();

        //Get the rightmost selected note (if any)
        i = list.begin();
        while (i != list.end())
        {
                if (i->second->isSelected())
                {
                        iRightmost = i;
                        rightmost = i->second;
                }

                ++i;
        }

        return rightmost;
}

CItem* EventCanvas::getLeftMostSelected()
{
        iCItem i, iLeftmost;
        CItem* leftmost = NULL;

        // get a list of items that belong to the current part
        // since multiple parts have populated the _items list
        // we need to filter on the actual current Part!
        CItemList list = getItemlistForCurrentPart();

        if (list.size() > 0)
        {
                i = list.end();
                while (i != list.begin())
                {
                        --i;

                        if (i->second->isSelected())
                        {
                                iLeftmost = i;
                                leftmost = i->second;
                        }
                }
        }

        return leftmost;
}
