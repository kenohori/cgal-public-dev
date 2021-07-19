// Copyright (c) 2021 GeometryFactory SARL (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Antonio Gomes, Dmitry Anisimov
//

#ifndef CGAL_BARYCENTRIC_INTERNAL_UTILS_3_H
#define CGAL_BARYCENTRIC_INTERNAL_UTILS_3_H

// #include <CGAL/license/Barycentric_coordinates_3.h>

// STL includes
#include <tuple>

// Internal includes
#include <CGAL/boost/graph/helpers.h>
#include <CGAL/convexity_check_3.h>
#include <CGAL/Barycentric_coordinates_2/Triangle_coordinates_2.h>

namespace CGAL{
namespace Barycentric_coordinates{
namespace internal{

enum class Edge_case {

  EXTERIOR = 0, // exterior part of the polygon
  BOUNDARY = 1, // boundary part of the polygon
  INTERIOR = 2  // interior part of the polygon
};

//Default sqrt
template<class Traits>
class Default_sqrt{
    typedef typename Traits::FT FT;

public:
    FT operator()(const FT &value) const{
        return FT(CGAL::sqrt(CGAL::to_double(value)));
    }
};

BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(Has_nested_type_Sqrt, Sqrt, false)

// Case: do_not_use_default = false.
template<class Traits, bool do_not_use_default = Has_nested_type_Sqrt<Traits>::value>
    class Get_sqrt
{
public:
    typedef Default_sqrt<Traits> Sqrt;

    static Sqrt sqrt_object(const Traits&)
    {
        return Sqrt();
    }
};

// Case: do_not_use_default = true.
template<class Traits>
    class Get_sqrt<Traits, true>
{
public:
    typedef typename Traits::Sqrt Sqrt;

    static Sqrt sqrt_object(const Traits &traits)
    {
        return traits.sqrt_object();
    }
};

// Get default values.
  template<typename OutputIterator>
  void get_default(
    const std::size_t n, OutputIterator output) {

    for (std::size_t i = 0; i < n; ++i) {
      *(output++) = 0;
    }
  }

  template<typename FT>
  FT get_tolerance() {
    return FT(1) / FT(10000000000);
  }

// Compute barycentric coordinates in the space.
  template<
  typename OutputIterator,
  typename GeomTraits>
  OutputIterator tetrahedron_coordinates_impl(
    const typename GeomTraits::Point_3& p0,
    const typename GeomTraits::Point_3& p1,
    const typename GeomTraits::Point_3& p2,
    const typename GeomTraits::Point_3& p3,
    const typename GeomTraits::Point_3& query,
    OutputIterator coordinates,
    const GeomTraits& traits) {

    // Number type.
    using FT = typename GeomTraits::FT;

    // Functions.
    const auto volume_3 = traits.compute_volume_3_object();
    const FT total_volume = volume_3(p0, p1, p2, p3);

    CGAL_precondition(total_volume != FT(0));
    if (total_volume == FT(0)) {
      get_default(4, coordinates);
      return coordinates;
    }

    // Compute some related sub-volumes.
    const FT V1 = volume_3(p1, p3, p2, query);
    const FT V2 = volume_3(p2, p3, p0, query);
    const FT V3 = volume_3(p3, p1, p0, query);

    // Compute the inverted total volume of the tetrahedron.
    CGAL_assertion(total_volume != FT(0));
    const FT inverted_total_volume = FT(1) / total_volume;

    // Compute coordinates.
    const FT b0 = V1 * inverted_total_volume;
    const FT b1 = V2 * inverted_total_volume;
    const FT b2 = V3 * inverted_total_volume;
    const FT b3 = FT(1) - b0 - b1 - b2;

    // Return coordinates.
    *(coordinates++) = b0;
    *(coordinates++) = b1;
    *(coordinates++) = b2;
    *(coordinates++) = b3;

    return coordinates;
  }

  // Compute normal vector of the face (not normalized).
  template<
    typename Face,
    typename VertexToPointMap,
    typename PolygonMesh,
    typename GeomTraits>
  typename GeomTraits::Vector_3 get_face_normal(
    const Face& face,
    const VertexToPointMap& vertex_to_point_map,
    const PolygonMesh& polygon_mesh,
    const GeomTraits& traits){

    using Point_3 = typename GeomTraits::Point_3;
    using Vector_3 = typename GeomTraits::Vector_3;
    const auto& cross_3 = traits.construct_cross_product_vector_3_object();

    const auto hedge = halfedge(face, polygon_mesh);
    const auto vertices = vertices_around_face(hedge, polygon_mesh);
    CGAL_precondition(vertices.size() >= 3);

    auto vertex = vertices.begin();
    const Point_3& point1 = get(vertex_to_point_map, *vertex); ++vertex;
    const Point_3& point2 = get(vertex_to_point_map, *vertex); ++vertex;
    const Point_3& point3 = get(vertex_to_point_map, *vertex);

    const Vector_3 u = point2 - point1;
    const Vector_3 v = point3 - point1;
    const Vector_3 face_normal = cross_3(u, v);

    return face_normal;
  }

  //Compute cotangent of dihedral angle between two faces
  template<typename GeomTraits>
  typename GeomTraits::FT cot_dihedral_angle(
    const typename GeomTraits::Vector_3& vec_1,
    const typename GeomTraits::Vector_3& vec_2,
    const GeomTraits& traits){

    using FT = typename GeomTraits::FT;
    const auto& dot_3 = traits.compute_scalar_product_3_object();
    const auto& cross_3 = traits.construct_cross_product_vector_3_object();
    const auto& sqrt(Get_sqrt<GeomTraits>::sqrt_object(traits));

    assert(vec_1.squared_length() != FT(0));
    assert(vec_2.squared_length() != FT(0));

    const FT approximate_dot_3 = dot_3(vec_1, vec_2);

    const FT approximate_cross_3_length = sqrt(cross_3(vec_1, vec_2).squared_length());

    assert(approximate_cross_3_length != FT(0));

    return approximate_dot_3/approximate_cross_3_length;
  }

  template<
    typename VertexRange,
    typename VertexToPointMap,
    typename PolygonMesh,
    typename OutIterator,
    typename GeomTraits>
  OutIterator boundary_coordinates_3(
    VertexRange vertex,
    const VertexToPointMap& vertex_to_point_map,
    const PolygonMesh& polygon_mesh,
    const typename GeomTraits::Point_3& query,
    OutIterator coordinates,
    const GeomTraits& traits){

    using Point_3 = typename GeomTraits::Point_3;
    using Vector_3 = typename GeomTraits::Vector_3;
    using FT = typename GeomTraits::FT;
    using Plane_3 = typename GeomTraits::Plane_3;
    using Point_2 = typename GeomTraits::Point_2;

    const auto v0 = *vertex; vertex++;
    const auto v1 = *vertex; vertex++;
    const auto v2 = *vertex;

    const FT tol = get_tolerance<FT>();

    const Plane_3 plane_face(get(vertex_to_point_map, v0),
     get(vertex_to_point_map, v1), get(vertex_to_point_map, v2));
    const Point_2 query_2d = plane_face.to_2d(query);
    const Point_2 v0_2d = plane_face.to_2d(get(vertex_to_point_map, v0));
    const Point_2 v1_2d = plane_face.to_2d(get(vertex_to_point_map, v1));
    const Point_2 v2_2d = plane_face.to_2d(get(vertex_to_point_map, v2));

    const std::array<FT, 3> coordinates_2d = compute_triangle_coordinates_2(
      v0_2d, v1_2d, v2_2d, query_2d, traits);

    const auto vd = vertices(polygon_mesh);
    for(const auto& v : vd){

      if(v == v0)
        *coordinates = CGAL::abs(coordinates_2d[0]) < tol? 0: coordinates_2d[0];
      else if(v == v1)
        *coordinates = CGAL::abs(coordinates_2d[1]) < tol? 0: coordinates_2d[1];
      else if(v == v2)
        *coordinates = CGAL::abs(coordinates_2d[2]) < tol? 0: coordinates_2d[2];
      else
        *coordinates = FT(0);

      coordinates++;
    }
    return coordinates;
  }

  // Determine if the query point is on the interior, exterior or boundary
  template<
    typename VertexToPointMap,
    typename PolygonMesh,
    typename OutIterator,
    typename GeomTraits>
  Edge_case locate_wrt_polyhedron(
    const VertexToPointMap& vertex_to_point_map,
    const PolygonMesh& polygon_mesh,
    const typename GeomTraits::Point_3& query,
    OutIterator coordinates,
    const GeomTraits& traits){

    using Point_3 = typename GeomTraits::Point_3;
    using Vector_3 = typename GeomTraits::Vector_3;
    using FT = typename GeomTraits::FT;
    const auto& dot_3 = traits.compute_scalar_product_3_object();
    const auto& construct_vector_3 = traits.construct_vector_3_object();
    const auto& sqrt(Get_sqrt<GeomTraits>::sqrt_object(traits));

    const FT tol = get_tolerance<FT>();
    auto face_range = faces(polygon_mesh);

    for(auto& face : face_range){

      const auto hedge = halfedge(face, polygon_mesh);
      const auto vertices_face = vertices_around_face(hedge, polygon_mesh);
      CGAL_precondition(vertices_face.size() >= 3);

      auto vertex = vertices_face.begin();
      const auto vertex_val = get(vertex_to_point_map, *vertex);

      // Vector connecting query point to vertex;
      const Vector_3 query_vertex = construct_vector_3(query, vertex_val);

      // Calculate normals of faces
      Vector_3 face_normal_i = get_face_normal(
        face, vertex_to_point_map, polygon_mesh, traits);
      face_normal_i = face_normal_i / sqrt(face_normal_i.squared_length());

      // Distance of query to face
      const FT perp_dist_i = dot_3(query_vertex, face_normal_i);

      // Verify location of query point;
      if(CGAL::abs(perp_dist_i) < tol){

        boundary_coordinates_3(vertex, vertex_to_point_map, polygon_mesh, query, coordinates, traits);
        return Edge_case::BOUNDARY;
      }
      else if(perp_dist_i < 0)
        return Edge_case::EXTERIOR;
    }

    //Default case
    return Edge_case::INTERIOR;
  }

}
}
}

#endif
