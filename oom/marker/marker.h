//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: marker.h,v 1.2 2003/12/15 11:41:00 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MARKER_H__
#define __MARKER_H__

#include <map>

#include "xml.h"
#include "utils.h"
#include "pos.h"

class QString;

//---------------------------------------------------------
//   Marker
//---------------------------------------------------------

class Marker : public Pos {
	  qint64 m_id;
      QString _name;
      bool _current;

   public:
      Marker() 
	  : m_id(create_id())
	  , _current(false)
	  {}
      Marker(const QString& s, bool cur = false)
      : m_id(create_id()), _name(s), _current(cur) {}
      void read(Xml&);
      const QString name() const     { return _name; }
      void setName(const QString& s) { _name = s;    }
      bool current() const           { return _current; }
      void setCurrent(bool f)        { _current = f; }
	  qint64 id() const  {return m_id;}
};

//---------------------------------------------------------
//   MarkerList
//---------------------------------------------------------

class MarkerList : public std::multimap<unsigned, Marker, std::less<unsigned> > {
   public:
      Marker* add(const Marker& m);
      Marker* add(const QString& s, int t, bool lck);
      void write(int, Xml&) const;
      void remove(Marker*);
      };

typedef std::multimap<unsigned, Marker, std::less<unsigned> >::iterator iMarker;
typedef std::multimap<unsigned, Marker, std::less<unsigned> >::const_iterator ciMarker;

#endif

