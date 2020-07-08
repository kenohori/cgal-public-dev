// Copyright (c) 2020 GeometryFactory (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Andreas Fabri, Ayush Saraswat


#ifndef CGAL_EMBREE_AABB_TREE_H
#define CGAL_EMBREE_AABB_TREE_H

#include <CGAL/license/Embree.h>
#include <embree3/rtcore.h>

#include <vector>

namespace CGAL {
namespace Embree {


  // AF: This is what you had called SM

template <typename TriangleMesh>
struct Triangle_mesh_geometry {
  typedef typename TriangleMesh::Face_index Face_index;
  typedef std::pair<Face_index,TriangleMesh*> Primitive_id;

  const TriangleMesh* surfaceMesh;
  RTCGeometry rtc_geometry;
  unsigned int rtc_geomID;

  Triangle_mesh_geometry(const TriangleMesh& tm)
    : surfaceMesh(&tm)
  {}

  static void bound_function(const struct RTCBoundsFunctionArguments* args)
  {
    // AF: move your code
    RTCBounds* bounds_o = args->bounds_o;
    unsigned int primID = args->primID;

    // Ayush: how should we get the Point?
    std::vector<Point> FacePoints;

    Face_index fd(primID);
    // Ayush: static member function trying to access non static variable surfaceMesh
    Triangle_mesh_geometry* self = (Triangle_mesh_geometry*) args->geometryUserPtr;

    typename TriangleMesh::Halfedge_index hf = surfaceMesh->halfedge(fd);
    for(typename TriangleMesh::Halfedge_index hi : halfedges_around_face(hf, *surfaceMesh)){
        typename TriangleMesh::Vertex_index vi = target(hi, *surfaceMesh);
        Point data = surfaceMesh->point(vi);
        FacePoints.push_back(data);
    }
    bounds_o->lower_x = std::min({FacePoints[0].x(), FacePoints[1].x(), FacePoints[2].x()});
    bounds_o->lower_y = std::min({FacePoints[0].y(), FacePoints[1].y(), FacePoints[2].y()});
    bounds_o->lower_z = std::min({FacePoints[0].z(), FacePoints[1].z(), FacePoints[2].z()});
    bounds_o->upper_x = std::max({FacePoints[0].x(), FacePoints[1].x(), FacePoints[2].x()});
    bounds_o->upper_y = std::max({FacePoints[0].y(), FacePoints[1].y(), FacePoints[2].y()});
    bounds_o->upper_z = std::max({FacePoints[0].z(), FacePoints[1].z(), FacePoints[2].z()});

  }

  static void intersection_function(const RTCIntersectFunctionNArguments* args)
  {
    // AF: move your code
    int* valid = args->valid;
    struct RTCRayHit* rayhit = (RTCRayHit*)args->rayhit;
    unsigned int primID = args->primID;

    assert(args->N == 1);
    // Ayush: we need to acces point, triangle, vector, ray for a specific kernel. should we pass a kernel in the template?
    std::vector<Point> FacePoints;
    if (!valid[0]) return;

    Face_index fd(primID);
    // Ayush: static member function trying to access non static variable surfaceMesh
    TriangleMesh::Halfedge_index hf = surfaceMesh->halfedge(fd);
    for(TriangleMesh::Halfedge_index hi : halfedges_around_face(hf, *surfaceMesh)){
        TriangleMesh::Vertex_index vi = target(hi, *surfaceMesh);
        Point data = surfaceMesh->point(vi);
        FacePoints.push_back(data);
    }
    Triangle face(FacePoints[0], FacePoints[1], FacePoints[2]);

    Vector rayDirection(rayhit->ray.dir_x, rayhit->ray.dir_y, rayhit->ray.dir_z);
    Point rayOrgin(rayhit->ray.org_x, rayhit->ray.org_y, rayhit->ray.org_z);
    Ray ray(rayOrgin, rayDirection);

    auto v = CGAL::intersection(ray, face);
    if(v){
        rayhit->hit.geomID = rtc_geomID;
        rayhit->hit.primID = primID;
        if (const Point *intersectionPoint = boost::get<Point>(&*v) ){
            float _distance = sqrt(CGAL::squared_distance(rayOrgin, *intersectionPoint));
            rayhit->ray.tfar = _distance;
        }
    }
  }

  void insert_primitives()
  {
    rtcSetGeometryUserPrimitiveCount(rtc_geometry, surfaceMesh.number_of_faces());
    rtcSetGeometryUserData(rtc_geometry, this);

    // AF: For the next two you have to find out how to write
    // the function pointer for a static member function

    // Ayush: for a static member function, we can directly pass a pointer, so the below should work fine.
    // https://isocpp.org/wiki/faq/pointers-to-members

    rtcSetGeometryBoundsFunction(rtc_geometry, bound_function, nullptr);
    rtcSetGeometryIntersectFunction(rtc_geometry, intersection_function);
    rtcCommitGeometry(rtc_geometry);

    rtcReleaseGeometry(rtc_geometry);
  }

  Primitive_id primitive_id(unsigned int primID) const
  {
    return std::make_pair(Face_index(primID),surfaceMesh);
  }
};


/**
 * \ingroup PkgEmbreeRef
 * This class...
 */


//  AF:  Geometry is the above class
// AF: For GeomTraits you take a kernel, that is Simple_cartesian
template <typename Geometry, typename GeomTraits>
class AABB_tree {

  typedef typename Geometry::Primitive_id Primitive_id;

  RTCDevice device;
  RTCScene scene;

  // AF: As the ray intersection returns a geomID we need a mapping
  std::unordered_map<unsigned int, Geometry> id2geometry;
  std::list<Geometry> geometries;

  void errorFunction(void* userPtr, enum RTCError error, const char* str)
  {
    std::cout<<"error "<<error<<": "<<str<<std::endl;
  }

  RTCDevice initializeDevice()
  {
    RTCDevice device = rtcNewDevice(NULL);

    if (!device)
      printf("error %d: cannot create device\n", rtcGetDeviceError(NULL));

    rtcSetDeviceErrorFunction(device, errorFunction, NULL);
    return device;
  }

  AABB_tree()
  {
    device = initializeDevice();
    scene = rtcNewScene(device);
  }



  /// T is the surface mesh
  template<typename T>
  void insert (const T& t)
  {
    geometries.push_back(Geometry(t));
    const Geometry geometry = geometries.back();
    geometry.rtc_geometry = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_USER);
    geometry.rtc_geomID = rtcAttachGeometry(scene, geometry.rtc_geometry);
    geometry.insert_primitives();
    rtcCommitScene(scene);
  }

  template<typename Ray>
  boost::optional<Primitive_id>
  first_intersected_primitive(const Ray& query) const
  {
    // AF: no idea where the next two lines should go

    // Ayush: they are fine here.
    struct RTCIntersectContext context;
    rtcInitIntersectContext(&context);

    struct RTCRayHit rayhit;

    // AF initialize rayhit
    rayhit.ray.org_x =  query.source().x(); /*POINT.X*/
    rayhit.ray.org_y =  query.source().y(); /*POINT.Y*/
    rayhit.ray.org_z =  query.source().z(); /*POINT.Z*/

    rayhit.ray.dir_x = query.direction().x()/ sqrt(pow(query.direction().x(), 2) + pow(query.direction().y(), 2) + pow(query.direction().z(), 2));
    rayhit.ray.dir_y = query.direction().y()/ sqrt(pow(query.direction().x(), 2) + pow(query.direction().y(), 2) + pow(query.direction().z(), 2));
    rayhit.ray.dir_z = query.direction().z()/ sqrt(pow(query.direction().x(), 2) + pow(query.direction().y(), 2) + pow(query.direction().z(), 2));

    rayhit.ray.tnear = 0;
    rayhit.ray.tfar = std::numeric_limits<float>::infinity();
    rayhit.ray.mask = 0;
    rayhit.ray.flags = 0;

    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    rayhit.hit.primID = RTC_INVALID_GEOMETRY_ID;

    rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

    rtcIntersect1(scene, &context, &rayhit);

    unsigned int rtc_geomID = rayhit.hit.geomID;
    if(rtc_geomID == RTC_INVALID_GEOMETRY_ID){
      return boost::none;
    }
    typename Geometry geometry = id2geometry[rtc_geomID];

    return boost::make_optional(geometry.primitive(rayhit.hit.primID));
  }


};

} // namespace Embree
} // namespace CGAL
#endif // CGAL_EMBREE_AABB_TREE_H
