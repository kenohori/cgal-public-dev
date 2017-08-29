// Copyright (c) 2017 GeometryFactory (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Author(s)     : Mael Rouxel-Labbé

#ifndef CGAL_POLYLINE_TRACING_MOTORCYCLE_PRIORITY_QUEUE_ENTRY_H
#define CGAL_POLYLINE_TRACING_MOTORCYCLE_PRIORITY_QUEUE_ENTRY_H

#include <CGAL/Polyline_tracing/Dictionary.h>
#include <CGAL/Polyline_tracing/Motorcycle.h>

#include <utility>

namespace CGAL {

namespace Polyline_tracing {

template<typename K>
class Motorcycle_priority_queue_entry
{
  typedef Motorcycle_priority_queue_entry<K>               Self;

public:
  typedef typename K::FT                                   FT;

  typedef typename Dictionary<K>::DEC_it                   DEC_it;
  typedef Motorcycle<K>                                    Motorcycle;

  const DEC_it dictionary_entry() const { return m->targets().begin()->first; }
  FT time() const { return m->targets().begin()->second; }
  Motorcycle& motorcycle() { return *m; }

  Motorcycle_priority_queue_entry(Motorcycle& mc);

  friend bool operator<(const Self& lhs, const Self& rhs) {
    return lhs.time() < rhs.time();
  }

private:
  Motorcycle* m;
};

template<typename K>
Motorcycle_priority_queue_entry<K>::
Motorcycle_priority_queue_entry(Motorcycle& mc)
  : m(&mc)
{
  CGAL_precondition(!m->targets().empty());
}

} // namespace Polyline_tracing

} // namespace CGAL

#endif // CGAL_POLYLINE_TRACING_MOTORCYCLE_PRIORITY_QUEUE_ENTRY_H
