﻿// Copyright (c) 2017 GeometryFactory (France).
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

#ifndef CGAL_POLYLINE_TRACING_UNIFORM_TRACER_H
#define CGAL_POLYLINE_TRACING_UNIFORM_TRACER_H

#include <CGAL/Polyline_tracing/Dictionary.h>
#include <CGAL/Polyline_tracing/Motorcycle.h>
#include <CGAL/Polyline_tracing/internal/robust_intersections.h>

#include <CGAL/assertions.h>
#include <CGAL/boost/graph/helpers.h>
#include <CGAL/Origin.h>
#include <CGAL/Polygon_mesh_processing/locate.h>

#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>

#include <utility>

namespace CGAL {

namespace Polyline_tracing {

template<typename MotorcycleGraphTraits, typename Tracer_visitor>
class Motorcycle;

template<typename MotorcycleGraphTraits>
class Uniform_direction_tracer_visitor
{
  typedef Uniform_direction_tracer_visitor<MotorcycleGraphTraits>     Self;

public:
  typedef MotorcycleGraphTraits                               Geom_traits;
  typedef typename Geom_traits::Triangle_mesh                 Triangle_mesh;

  typedef typename Geom_traits::FT                            FT;
  typedef typename Geom_traits::Point_d                       Point;
  typedef typename Geom_traits::Segment_d                     Segment;
  typedef typename Geom_traits::Vector_d                      Vector;
  typedef typename Geom_traits::Ray_d                         Ray;

  typedef Dictionary<Geom_traits>                             Dictionary;
  typedef Dictionary_entry<Geom_traits>                       Dictionary_entry;
  typedef typename Dictionary::DEC_it                         DEC_it;

  typedef typename Geom_traits::Face_location                 Face_location;

  typedef Motorcycle<Geom_traits, Self>                       Motorcycle;

  typedef typename Geom_traits::vertex_descriptor             vertex_descriptor;
  typedef typename Geom_traits::halfedge_descriptor           halfedge_descriptor;
  typedef typename Geom_traits::face_descriptor               face_descriptor;

  // - bool: whether we have found a destination or not
  // - DEC_it: the source of the path (might be different from mc.current_position()
  //           if on the border)
  // - DEC_it: the destination
  // - FT: the time at the destination
  // - bool: is the destination final
  typedef boost::tuple<bool, DEC_it, DEC_it, FT, bool>        result_type;

  // Constructor
  Uniform_direction_tracer_visitor(const Geom_traits& gt = Geom_traits()) : gt(gt) { }

  // Functions
  result_type compute_next_destination(const DEC_it start_point,
                                       const face_descriptor fd, const Motorcycle& mc,
                                       Dictionary& points, const Triangle_mesh& mesh) const;

  result_type operator()(vertex_descriptor vd, const Motorcycle& mc,
                         Dictionary& points, const Triangle_mesh& mesh) const;
  result_type operator()(halfedge_descriptor hd, const Motorcycle& mc,
                         Dictionary& points, const Triangle_mesh& mesh) const;
  result_type operator()(face_descriptor fd, const Motorcycle& mc,
                         Dictionary& points, const Triangle_mesh& mesh) const;

private:
  Geom_traits gt;
};

// -----------------------------------------------------------------------------

template<typename MotorcycleGraphTraits>
typename Uniform_direction_tracer_visitor<MotorcycleGraphTraits>::result_type
Uniform_direction_tracer_visitor<MotorcycleGraphTraits>::
compute_next_destination(const DEC_it start_point,
                         const face_descriptor fd, const Motorcycle& mc,
                         Dictionary& points, const Triangle_mesh& mesh) const
{
  CGAL_precondition(start_point->location().first == fd);
  CGAL_precondition(num_vertices(mesh) != 0);

  // @todo add named parameters ?
  typedef typename property_map_selector<Triangle_mesh, boost::vertex_point_t>::const_type VertexPointMap;
  VertexPointMap vpmap = get_const_property_map(vertex_point, mesh);

  // @todo do I really need to look at all the halfedges? why not simply break
  // as soon as it's a valid destination with time different from current_time?
  Point farthest_destination;
  FT time_at_farthest_destination = mc.current_time(); // minimum allowed time value
  Vector mc_dir = *(mc.direction());

  Ray r(start_point->point(), mc_dir);

  typedef CGAL::Halfedge_around_face_circulator<Triangle_mesh>  Halfedge_around_facet_circulator;
  Halfedge_around_facet_circulator hcir_begin(halfedge(fd, mesh), mesh);
  Halfedge_around_facet_circulator hcir = hcir_begin;
  do
  {
    halfedge_descriptor hd = *hcir++;

    Segment s(get(vpmap, source(hd, mesh)), get(vpmap, target(hd, mesh)));
    std::cout << "ray: " << r << std::endl << "segment: " << s << std::endl;

    if(gt.do_intersect_2_object()(r, s))
    {
      // returns a point because we ignore the degenerate configuration of the ray
      // and segment being aligned (the next halfedge will give us an intersection
      // at a vertex descriptor, which is the point we need)

      boost::optional<Point> res = internal::robust_intersection<Geom_traits>(r, s, gt);
      if(!res)
        continue;

      const Point new_destination = *res;

      // compute the time at destination
      FT time_at_new_destination = mc.current_time() +
        CGAL::sqrt(CGAL::squared_distance(start_point->point(), new_destination)) / mc.speed();

      std::cout << "new potential destination: " << new_destination
                << " a time: " << time_at_new_destination << std::endl;

      if(time_at_new_destination > time_at_farthest_destination)
      {
        farthest_destination = new_destination;
        time_at_farthest_destination = time_at_new_destination;
      }
    }
  } while (hcir != hcir_begin);

  CGAL_postcondition(time_at_farthest_destination >= mc.current_time());
  if(time_at_farthest_destination == mc.current_time())
  {
    std::cerr << "Warning: motorcycle has no interesting intersection "
              << "with the border of the face: " << fd << std::endl;

    // Since we have already dealt with the case of a null direction, if the current
    // position is the new destination, then the direction is pointing outside
    // of this face.
    CGAL_assertion(*(mc.direction()) != CGAL::NULL_VECTOR);
    return boost::make_tuple(false, DEC_it(), DEC_it(), 0., false /*not final*/);
  }

  Face_location loc = CGAL::Polygon_mesh_processing::locate<Triangle_mesh>(
                        fd, farthest_destination, mesh);

  // A uniform tracer will trace until it reaches a boundary. It is important
  // that the location of this new destination reflects that it is on the boundary
  // (that is, one of its barycentric coordinates should be 0). To ensure that
  // it is the case, it is snapped to the closest halfedge (or even vertex).
  CGAL::Polygon_mesh_processing::internal::snap_location_to_border<Triangle_mesh>(loc);

  std::pair<DEC_it, bool> destination = points.insert(loc, farthest_destination);

#ifdef CGAL_MOTORCYCLE_GRAPH_VERBOSE
  std::cout << "new source at " << &*start_point
            << " p: " << start_point->point()
            << "loc: " << start_point->location().first
            << " bc: [" << start_point->location().second[0] << " "
                        << start_point->location().second[1] << " "
                        << start_point->location().second[2] << "] "
            << " time: " << mc.current_time() << std::endl;
  std::cout << "new destination at : " << &*destination.first
            << " p: " << destination.first->point()
            << "loc: " << destination.first->location().first
            << " bc: [" << destination.first->location().second[0] << " "
                        << destination.first->location().second[1] << " "
                        << destination.first->location().second[2] << "] "
            << " time: " << time_at_farthest_destination << std::endl;
#endif

  return boost::make_tuple(true, start_point, destination.first,
                           time_at_farthest_destination, false /*not final*/);
}

template<typename MotorcycleGraphTraits>
typename Uniform_direction_tracer_visitor<MotorcycleGraphTraits>::result_type
Uniform_direction_tracer_visitor<MotorcycleGraphTraits>::
operator()(vertex_descriptor vd, const Motorcycle& mc,
           Dictionary& points, const Triangle_mesh& mesh) const
{
  // check which face we should move into, find it, compute.
#ifdef CGAL_MOTORCYCLE_GRAPH_VERBOSE
  std::cout << " Uniform tracing from a point on the vertex " << vd;
  std::cout << " with direction: " << *(mc.direction()) << std::endl;
#endif

  // just to get rid of a degenerate case // @todo factorize this
  CGAL_precondition(bool(mc.direction())); // direction must be known
  if(*(mc.direction()) == CGAL::NULL_VECTOR)
  {
    std::cerr << "Warning: the motorcycle direction is null and "
              << "the next destination is thus the current position" << std::endl;

    return boost::make_tuple(true, mc.current_position(), mc.current_position(),
                             mc.current_time(), true /*final destination*/);
  }

  halfedge_descriptor hd = halfedge(vd, mesh);

  // loop the incident faces of vd
  typedef CGAL::Face_around_target_circulator<Triangle_mesh> face_around_target_circulator;
  face_around_target_circulator fatc(hd, mesh), done(fatc);
  do
  {
    face_descriptor fd = *fatc;
    if(fd == boost::graph_traits<Triangle_mesh>::null_face())
    {
      ++fatc;
      continue;
    }

    std::cout << "at face: " << fd << std::endl;

    // Compute the position of the motorcycle in the current face
    std::pair<DEC_it, bool> source_in_fd; // bool indicates whether it's a new point or not

    if(mc.current_position()->location().first == fd)
    {
      source_in_fd = std::make_pair(mc.current_position(), false);
    }
    else
    {
      Face_location loc_in_fd = CGAL::Polygon_mesh_processing::locate(
                                  mc.current_position()->location(), fd, mesh);

      // Insert into the dictionary, but keep an iterator to it so that it can
      // be removed quickly if this point is useless
      source_in_fd = points.insert(loc_in_fd, mc.current_position()->point());
    }

    result_type res = compute_next_destination(source_in_fd.first, fd, mc, points, mesh);

    // Since direction == NULL_VECTOR has been filtered in Tracer.h, the time
    // can only be null if the direction points outside
    if(res.template get<0>() && res.template get<3>() > 0)
      return res;

    // If 'source_in_fd' is a new point in the dictionary and 'fd' is not the face
    // in which the destination lies, then clean 'source_in_fd' off from the dictionary.
    if(source_in_fd.second)
    {
      // To make sure that indeed no motorcycle uses this point
      CGAL_assertion(source_in_fd.first->visiting_motorcycles().empty());
      points.erase(source_in_fd.first);
    }

    ++fatc;
  }
  while(fatc != done);

  // If we couldn't find a destination, then we must be on the border of the mesh
  // with a direction pointing out.
  CGAL_assertion(bool(is_border(vd, mesh)));
  return boost::make_tuple(false, mc.current_position(), mc.current_position(),
                           mc.current_time(), true /*final destination*/);
}

template<typename MotorcycleGraphTraits>
typename Uniform_direction_tracer_visitor<MotorcycleGraphTraits>::result_type
Uniform_direction_tracer_visitor<MotorcycleGraphTraits>::
operator()(halfedge_descriptor hd, const Motorcycle& mc,
           Dictionary& points, const Triangle_mesh& mesh) const
{
#ifdef CGAL_MOTORCYCLE_GRAPH_VERBOSE
  std::cout << " Uniform tracing from a point on the halfedge " << hd;
  std::cout << " with direction: " << *(mc.direction()) << std::endl;
#endif

  // just to get rid of a degenerate case
  CGAL_precondition(bool(mc.direction())); // direction must be known
  if(*(mc.direction()) == CGAL::NULL_VECTOR)
  {
    std::cerr << "Warning: the motorcycle direction is null and "
              << "the next destination is thus the current position" << std::endl;

    return boost::make_tuple(true, mc.current_position(), mc.current_position(),
                             mc.current_time(), true /*final destination*/);
  }

  // When we reach the border at the interior of a halfedge, the path continues
  // on the adjacent face.

  // Exception case: we are computing the first destination. In that case, first
  // try to find a valid destination on face(hd, mesh)
  if(mc.input_destination() == boost::none)
  {
    face_descriptor fd = face(hd, mesh);
    result_type res = compute_next_destination(mc.current_position(), fd, mc, points, mesh);

    // Since direction == NULL_VECTOR has been filtered in Tracer.h, the time
    // can only be null if the direction points to face(opposite(hd, mesh), mesh)
    // and not to face(hd, mesh)
    if(res.template get<0>() && res.template get<3>() > 0)
      return res;
  }

  // Check the other face incident to hd
  halfedge_descriptor opp_hd = opposite(hd, mesh);

  if(is_border(opp_hd, mesh))
  {
    // Source is on the border of the mesh and the direction is pointing out,
    return boost::make_tuple(false, mc.current_position(), mc.current_position(),
                             mc.current_time(), true /*final destination*/);
  }

  // Compute the position of the motorcycle in the opposite face
  face_descriptor opp_fd = face(opp_hd, mesh);
  CGAL_assertion(opp_fd != boost::graph_traits<Triangle_mesh>::null_face());
  Face_location opp_loc = CGAL::Polygon_mesh_processing::locate(mc.current_position()->location(),
                                                                opp_fd, mesh);

  // Insert the source seen from the opposite face in the dictionary
  std::pair<DEC_it, bool> source_in_next_face = points.insert(opp_loc,
                                                              mc.current_position()->point());

  result_type opp_res = compute_next_destination(source_in_next_face.first, opp_fd, mc, points, mesh);

  CGAL_assertion(opp_res.template get<0>());
  return opp_res;
}

template<typename MotorcycleGraphTraits>
typename Uniform_direction_tracer_visitor<MotorcycleGraphTraits>::result_type
Uniform_direction_tracer_visitor<MotorcycleGraphTraits>::
operator()(face_descriptor fd, const Motorcycle& mc,
           Dictionary& points, const Triangle_mesh& mesh) const
{
#ifdef CGAL_MOTORCYCLE_GRAPH_VERBOSE
  std::cout << " Uniform tracing from a point in the face " << fd;
  std::cout << " with direction: " << *(mc.direction()) << std::endl;
#endif

  // just to get rid of a degenerate case
  CGAL_precondition(bool(mc.direction())); // direction must be known
  if(*(mc.direction()) == CGAL::NULL_VECTOR)
  {
    std::cerr << "Warning: the motorcycle direction is null and "
              << "the next destination is thus the current position" << std::endl;

    return boost::make_tuple(true, mc.current_position(), mc.current_position(),
                             mc.current_time(), true /*final destination*/);
  }

  return compute_next_destination(mc.current_position(), fd, mc, points, mesh);
}

} // namespace Polyline_tracing

} // namespace CGAL

#endif // CGAL_POLYLINE_TRACING_UNIFORM_TRACER_H
