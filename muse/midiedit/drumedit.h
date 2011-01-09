//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: drumedit.h,v 1.9.2.7 2009/11/16 11:29:33 lunar_shuttle Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __DRUM_EDIT_H__
#define __DRUM_EDIT_H__

#include <values.h>
#include "midieditor.h"
#include "noteinfo.h"
#include "cobject.h"
#include "tools.h"
#include "header.h"
#include "shortcuts.h"
#include "event.h"

class QCloseEvent;
class QLabel;
class QMenu;
class QKeyEvent;
class QResizeEvent;
class QToolButton;
class QWidget;

class MidiPart;
class DrumCanvas;
class ScrollScale;
class ScoreConfig;
class MTScale;
class Splitter;
class PartList;
class Toolbar1;
class CtrlCanvas;
class Xml;
class DList;
class Header;
class CtrlEdit;
class Part;
class SNode;

//---------------------------------------------------------
//   DrumEdit
//---------------------------------------------------------

class DrumEdit : public MidiEditor {
      Event selEvent;
      MidiPart* selPart;
      int selTick;
      QMenu* menuEdit, *menuFunctions, *menuFile, *menuSelect;

      NoteInfo* info;
      QToolButton* srec;
      QToolButton* midiin;
      EditToolBar* tools2;

      Toolbar1* toolbar;
      Splitter* split1;
      Splitter* split2;
      QWidget* split1w1;
      DList* dlist;
      Header* header;
      QToolBar* tools;

      static int _quantInit, _rasterInit;
      static int _widthInit, _heightInit;
      static int _dlistWidthInit, _dcanvasWidthInit;

      static int _toInit; //Used in function dialog for applying modification to selection

      QAction *loadAction, *saveAction, *resetAction;
      QAction *cutAction, *copyAction, *pasteAction, *deleteAction;
      QAction *fixedAction, *veloAction;
      QAction *sallAction, *snoneAction, *invAction, *inAction , *outAction;
      QAction *prevAction, *nextAction;

      Q_OBJECT
      void initShortcuts();

      virtual void closeEvent(QCloseEvent*);
      QWidget* genToolbar(QWidget* parent);
      virtual void resizeEvent(QResizeEvent*);
      virtual void keyPressEvent(QKeyEvent*);
      int _to;//TODO: Make this work
      void setHeaderToolTips();
      void setHeaderWhatsThis();

   private slots:
      void setRaster(int);
      void setQuant(int);
      void noteinfoChanged(NoteInfo::ValType type, int val);
      //CtrlEdit* addCtrl();
      void cmd(int);
      void clipboardChanged(); // enable/disable "Paste"
      void selectionChanged(); // enable/disable "Copy" & "Paste"
      void load();
      void save();
      void reset();
      void setTime(unsigned);
      void follow(int);
      void newCanvasWidth(int);
      void configChanged();
      void songChanged1(int);

   public slots:
      void setSelection(int, Event&, Part*);
      void soloChanged(bool);       // called by Solo button
      void execDeliveredScript(int);
      void execUserScript(int);
      CtrlEdit* addCtrl();
      void removeCtrl(CtrlEdit* ctrl);
      
      virtual void updateHScrollRange();
   signals:
      void deleted(unsigned long);

   public:
      DrumEdit(PartList*, QWidget* parent = 0, const char* name = 0, unsigned initPos = MAXINT);
      virtual ~DrumEdit();
      virtual void readStatus(Xml&);
      virtual void writeStatus(int, Xml&) const;
      static void readConfiguration(Xml& xml);
      static void writeConfiguration(int, Xml&);
      };

#endif
