// Copyright (c) 2018  Liangliang Nan
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
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0+
//
// Author(s)     : Liangliang Nan

#ifndef CGAL_POLYGONAL_SURFACE_RECONSTRUCTION_CANDIDATE_CONFIDENCES_H
#define CGAL_POLYGONAL_SURFACE_RECONSTRUCTION_CANDIDATE_CONFIDENCES_H


#include <CGAL/Surface_mesh.h>
#include <CGAL/compute_average_spacing.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Point_set_with_segments.h>
#include <CGAL/algo/alpha_shape_mesh.h>
#include <CGAL/algo/hypothesis.h>
#include <CGAL/algo/parameters.h>
#include <CGAL/assertions.h>


// Concurrency
#ifdef CGAL_LINKED_WITH_TBB
typedef CGAL::Parallel_tag Concurrency_tag;
#else
typedef CGAL::Sequential_tag Concurrency_tag;
#endif


namespace CGAL {


	/** \ingroup PkgPolygonalSurfaceReconstruction
	*
	*	Compute the confidences of the candidate faces.
	*/

	template <typename Kernel>
	class Candidate_confidences
	{
	private:
		typedef typename Kernel::FT						FT;
		typedef typename Kernel::Point_3				Point;
		typedef typename Kernel::Point_2				Point2;
		typedef typename Kernel::Vector_3				Vector;
		typedef typename Kernel::Line_3					Line;
		typedef typename Kernel::Segment_3				Segment;
		typedef typename Kernel::Plane_3				Plane;
		typedef CGAL::Polygon_2<Kernel>					Polygon;
		typedef CGAL::Planar_segment<Kernel>			Planar_segment;
		typedef CGAL::Point_set_with_segments<Kernel>	Point_set;

		typedef CGAL::Surface_mesh<Point>				Mesh;
		typedef typename Mesh::Face_index				Face_descriptor;
		typedef typename Mesh::Edge_index				Edge_descriptor;
		typedef typename Mesh::Vertex_index				Vertex_descriptor;

		typedef typename Mesh::Halfedge_index			Halfedge_descriptor;

	public:
		Candidate_confidences() {}
		~Candidate_confidences() {}

		/// Compute the confidence values for each face
		/// - supporting point number:	stored as property 'f:num_supporting_points' 
		/// - face area:				stored as property 'f:face_area' 
		/// - covered area:				stored as property 'f:covered_area' 
		void compute(const Point_set& point_set, Mesh& mesh);

	private:
		// returns the indices of the supporting point for 'face'
		std::vector<std::size_t> supporting_points(Face_descriptor face, const Mesh& mesh, const Point_set& point_set);

		inline FT triangle_area(const Point& p1, const Point& p2, const Point& p3) const {
			const Vector& orth = CGAL::cross_product(Vector(p1, p2), Vector(p1, p3));
			return std::sqrt(orth.squared_length()) * FT(0.5);
		}

		FT face_area(Face_descriptor f, const Mesh& mesh) const {
			FT result(0);

			const typename Mesh::template Property_map<Vertex_descriptor, Point>& coords = mesh.points();

			Halfedge_around_face_circulator<Mesh> cir(mesh.halfedge(f), mesh), done(cir);
			Halfedge_descriptor p_hd = *cir;
			Vertex_descriptor p_vd = mesh.target(p_hd);
			const Point& p = coords[p_vd];
			++cir;

			do {
				Halfedge_descriptor q_hd = *cir;
				Vertex_descriptor q_vd = mesh.target(q_hd);
				const Point& q = coords[q_vd];

				Halfedge_descriptor r_hd = mesh.next(q_hd);
				Vertex_descriptor r_vd = mesh.target(r_hd);
				const Point& r = coords[r_vd];

				result += triangle_area(p, q, r);

				++cir;
			} while (cir != done);

			return result;
		}
	};


	//////////////////////////////////////////////////////////////////////////

	// implementation

	template <typename Kernel>
	std::vector<std::size_t> Candidate_confidences<Kernel>::supporting_points(Face_descriptor face, const Mesh& mesh, const Point_set& point_set) {
		std::vector<std::size_t> indices;

		if (face == Mesh::null_face())
			return indices;

		// the supporting planar segment of each face
		typename Mesh::template Property_map<Face_descriptor, Planar_segment*> face_supporting_segments =
			mesh.template property_map<Face_descriptor, Planar_segment*>("f:supp_segment").first;

		Planar_segment* segment = face_supporting_segments[face];
		if (segment == nullptr)
			return indices;

		// the supporting plane of each face
		typename Mesh::template Property_map<Face_descriptor, const Plane*> face_supporting_planes =
			mesh.template property_map<Face_descriptor, const Plane*>("f:supp_plane").first;
		// we do everything by projecting the point onto the face's supporting plane
		const Plane* supporting_plane = face_supporting_planes[face];
		CGAL_assertion(supporting_plane == segment->supporting_plane());

		Polygon plg; // the projection of the face onto it supporting plane
		const typename Mesh::template Property_map<Vertex_descriptor, Point>& coords = mesh.points();
		Halfedge_around_face_circulator<Mesh> cir(mesh.halfedge(face), mesh), done(cir);
		do {
			Halfedge_descriptor hd = *cir;
			Vertex_descriptor vd = mesh.target(hd);
			const Point& p = coords[vd];
			const Point2& q = supporting_plane->to_2d(p);

			// remove duplicated vertices
			// the last point in the polygon
			if (!plg.is_empty()) {
				const Point2& r = plg[plg.size() - 1];
				if (CGAL::squared_distance(q, r) < CGAL::snap_squared_distance_threshold<FT>())
					continue;
			}
			plg.push_back(q);

			++cir;
		} while (cir != done);

		if (plg.size() < 3 || !plg.is_simple())
			return indices;

		const typename Point_set::Point_map& points = point_set.point_map();
		for (std::size_t i = 0; i < segment->size(); ++i) {
			std::size_t idx = segment->at(i);
			const Point& p = points[idx];
			if (plg.bounded_side(supporting_plane->to_2d(p)) == CGAL::ON_BOUNDED_SIDE)
				indices.push_back(idx);
		}

		return indices;
	}

	template <typename Kernel>
	void Candidate_confidences<Kernel>::compute(const Point_set& point_set, Mesh& mesh) {
		const unsigned int K = 6;

		const typename Point_set::Point_map& points = point_set.point_map();
		FT avg_spacing = compute_average_spacing<Concurrency_tag>(points, K);

		// the number of supporting points of each face
		typename Mesh::template Property_map<Face_descriptor, std::size_t> face_num_supporting_points =
			mesh.template add_property_map<Face_descriptor, std::size_t>("f:num_supporting_points").first;

		// the area of each face
		typename Mesh::template Property_map<Face_descriptor, FT> face_areas =
			mesh.template add_property_map<Face_descriptor, FT>("f:face_area").first;

		// the point covered area of each face
		typename Mesh::template Property_map<Face_descriptor, FT> face_covered_areas =
			mesh.template add_property_map<Face_descriptor, FT>("f:covered_area").first;

		// the supporting plane of each face
		typename Mesh::template Property_map<Face_descriptor, const Plane*> face_supporting_planes =
			mesh.template property_map<Face_descriptor, const Plane*>("f:supp_plane").first;

		FT degenerate_face_area_threshold = CGAL::snap_squared_distance_threshold<FT>() * CGAL::snap_squared_distance_threshold<FT>();

		BOOST_FOREACH(Face_descriptor f, mesh.faces()) {
			const Plane* supporting_plane = face_supporting_planes[f];
			// face area
			FT area = face_area(f, mesh);
			face_areas[f] = area;

			if (area > degenerate_face_area_threshold) {
				const std::vector<std::size_t>& indices = supporting_points(f, mesh, point_set);
				face_num_supporting_points[f] = indices.size();

				std::vector<Point> pts;
				for (std::size_t i = 0; i < indices.size(); ++i) {
					std::size_t idx = indices[i];
					const Point& p = points[idx];
					pts.push_back(p);
				}

				FT covered_area(0);
				Alpha_shape_mesh<Kernel> alpha_mesh(pts.begin(), pts.end(), *supporting_plane);
				Mesh covering_mesh;
				FT radius = avg_spacing * FT(5.0);
				if (alpha_mesh.extract_mesh(radius * radius, covering_mesh)) {
					// we cannot use the area of the 3D faces, because the alpha shape mesh is 
					// not perfectly planar
					const typename Mesh::template Property_map<Vertex_descriptor, Point>& coords = covering_mesh.points();
					BOOST_FOREACH(Face_descriptor face, covering_mesh.faces()) {
						// we have to use the projected version
						Polygon plg; // the projection of the face onto it supporting plane
						Halfedge_around_face_circulator<Mesh> cir(covering_mesh.halfedge(face), covering_mesh), done(cir);
						do {
							Halfedge_descriptor hd = *cir;
							Vertex_descriptor vd = covering_mesh.target(hd);
							const Point& p = coords[vd];
							const Point2& q = supporting_plane->to_2d(p);
							plg.push_back(q);
							++cir;
						} while (cir != done);
						covered_area += std::abs(plg.area());
					}
				}

				face_covered_areas[f] = covered_area;
				if (covered_area > area)
					face_covered_areas[f] = area;
			}
			else { // for tiny faces, we can simple assign zero supporting points
				face_num_supporting_points[f] = 0;
				face_covered_areas[f] = FT(0.0);
			}
		}
	}

} //namespace CGAL


#endif // CGAL_POLYGONAL_SURFACE_RECONSTRUCTION_CANDIDATE_CONFIDENCES_H
