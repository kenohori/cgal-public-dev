// Copyright (c) 1997, 2012, 2019 INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
// Author(s)     : Mariette Yvinec, Claudia Werner

#ifndef CGAL_TRIANGULATION_ON_SPHERE_FACE_BASE_2_H
#define CGAL_TRIANGULATION_ON_SPHERE_FACE_BASE_2_H

#include <CGAL/license/Triangulation_on_sphere_2.h>

#include <CGAL/Triangulation_ds_face_base_2.h>

namespace CGAL {

template <typename Gt,
          typename Fb = Triangulation_ds_face_base_2<> >
class Triangulation_on_sphere_face_base_2
    : public Fb
{
public:
  typedef Gt                                                  Geom_traits;
  typedef typename Fb::Vertex_handle                          Vertex_handle;
  typedef typename Fb::Face_handle                            Face_handle;

  template < typename TDS2 >
  struct Rebind_TDS {
    typedef typename Fb::template Rebind_TDS<TDS2>::Other     Fb2;
    typedef Triangulation_on_sphere_face_base_2<Gt, Fb2>      Other;
  };

public:
  void set_in_conflict_flag(unsigned char f) { _in_conflict_flag = f; }
  unsigned char get_in_conflict_flag() const { return _in_conflict_flag; }

public:
  Triangulation_on_sphere_face_base_2()
    : Fb(), _ghost(false)
  {
    set_in_conflict_flag(0);
  }

  Triangulation_on_sphere_face_base_2(Vertex_handle v0,
                                      Vertex_handle v1,
                                      Vertex_handle v2)
    : Fb(v0, v1, v2), _ghost(false)
  {
    set_in_conflict_flag(0);
  }

  Triangulation_on_sphere_face_base_2(Vertex_handle v0,
                                      Vertex_handle v1,
                                      Vertex_handle v2,
                                      Face_handle n0,
                                      Face_handle n1,
                                      Face_handle n2)
    : Fb(v0, v1, v2, n0, n1, n2), _ghost(false)
  {
    set_in_conflict_flag(0);
  }

  bool is_ghost() const { return _ghost; }
  bool& ghost() { return _ghost; }

protected:
  bool _ghost;
  unsigned char _in_conflict_flag;
};

} // namespace CGAL

#endif //CGAL_TRIANGULATION_ON_SPHERE_FACE_BASE_2_H
