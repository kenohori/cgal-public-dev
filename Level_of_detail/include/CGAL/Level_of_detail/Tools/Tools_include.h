#ifndef CGAL_LEVEL_OF_DETAIL_TOOLS_INCLUDE_H
#define CGAL_LEVEL_OF_DETAIL_TOOLS_INCLUDE_H

#include <CGAL/Level_of_detail/Tools/Data/Semantic_data_splitter.h>
#include <CGAL/Level_of_detail/Tools/Data/Polygon_data_estimator.h>
#include <CGAL/Level_of_detail/Tools/Data/Kd_tree_with_data_creator.h>

#include <CGAL/Level_of_detail/Tools/Estimations/Parameters_estimator.h>
#include <CGAL/Level_of_detail/Tools/Estimations/Barycentre_estimator.h>
#include <CGAL/Level_of_detail/Tools/Estimations/End_points_estimator.h>
#include <CGAL/Level_of_detail/Tools/Estimations/Bounding_box_estimator.h>
#include <CGAL/Level_of_detail/Tools/Estimations/Average_spacing_estimator.h>
#include <CGAL/Level_of_detail/Tools/Estimations/Tree_based_lines_estimator.h>

#include <CGAL/Level_of_detail/Tools/Filtering/Grid_based_filtering.h>
#include <CGAL/Level_of_detail/Tools/Filtering/Alpha_shapes_filtering.h>

#include <CGAL/Level_of_detail/Tools/Fitting/Line_to_points_fitter.h>
#include <CGAL/Level_of_detail/Tools/Fitting/Plane_to_points_fitter.h>
#include <CGAL/Level_of_detail/Tools/Fitting/Segment_to_points_fitter.h>

#include <CGAL/Level_of_detail/Tools/Transformations/Translator.h>
#include <CGAL/Level_of_detail/Tools/Transformations/Points_to_line_projector.h>

#include <CGAL/Level_of_detail/Tools/Property_maps/Colours/Colour_property_map.h>
#include <CGAL/Level_of_detail/Tools/Property_maps/Colours/Buildings_from_facets_colour_property_map.h>
#include <CGAL/Level_of_detail/Tools/Property_maps/Colours/Visibility_from_facets_colour_property_map.h>

#include <CGAL/Level_of_detail/Tools/Property_maps/General/Dereference_property_map.h>
#include <CGAL/Level_of_detail/Tools/Property_maps/General/Semantic_element_property_map.h>
#include <CGAL/Level_of_detail/Tools/Property_maps/General/Estimated_normal_property_map_2.h>
#include <CGAL/Level_of_detail/Tools/Property_maps/General/Segment_from_region_property_map_2.h>

#include <CGAL/Level_of_detail/Tools/Property_maps/Points/Partition_point_property_map.h>
#include <CGAL/Level_of_detail/Tools/Property_maps/Points/Point_from_value_property_map_2.h>
#include <CGAL/Level_of_detail/Tools/Property_maps/Points/Point_from_reference_property_map_2.h>

#include <CGAL/Level_of_detail/Tools/Triangulations/Constrained_triangulation_creator.h>
#include <CGAL/Level_of_detail/Tools/Triangulations/Data_structures/Triangulation_face_info.h>
#include <CGAL/Level_of_detail/Tools/Triangulations/Data_structures/Triangulation_vertex_info.h>

#include <CGAL/Level_of_detail/Tools/Quality/Coverage.h>
#include <CGAL/Level_of_detail/Tools/Quality/Complexity.h>
#include <CGAL/Level_of_detail/Tools/Quality/Distortion.h>

#include <CGAL/Level_of_detail/Tools/Sorting/Scores_based_sorting.h>

#endif // CGAL_LEVEL_OF_DETAIL_TOOLS_INCLUDE_H