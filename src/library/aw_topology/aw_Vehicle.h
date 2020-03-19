/*--------------------------------------------------------------------
 * project ....: Darpa Urban Challenge 2007
 * authors ....: Team AnnieWAY
 * organization: Transregional Collaborative Research Center 28 /
 *               Cognitive Automobiles
 * creation ...: xx/xx/2007
 * revisions ..: $Id:$
---------------------------------------------------------------------*/
#ifndef AW_VEHICLE_H
#define AW_VEHICLE_H

#include <aw_CGAL.h>
#include <perception_interface.h>
#include <trajectory_points_interface.h>
#include <aw_RndfGraph.h>

  // for pose..TODO: remove
#include <aw_geometry.h>
#include <aw_timestamp.hpp>

namespace vlr {

class VehicleManager;
class Topology;
class Vehicle;

typedef int VehId;
typedef std::set<Vehicle> VehicleSet;
typedef std::set<VehId> VehicleIdSet;
typedef std::map<VehId, Vehicle> VehicleMap;

struct VehiclePose {
  kogmo_point_2D_t pose;
  Timestamp time;
  VehiclePose() : pose(), time() {};
  VehiclePose(kogmo_point_2D_t pose, Timestamp time) : pose(pose), time(time) {};
};

typedef std::map<VehId, VehiclePose> VehiclePoseMap;

//--------------------------------------------------------
//             Vehicle
//--------------------------------------------------------

class Vehicle
{
public:
	// default constructor, required to hold this in an stl container
	Vehicle();
	Vehicle( const PerceptionDynamicObstacle& vehicle, double offset_x, double offset_y);
	Vehicle( double x, double y, double yaw, double width, double length, double speed = 0. );

	~Vehicle();

    inline int32_t id() const {return id_;}
    inline double xMatchedFrom() const {return x_matched_from_;}
    inline double yMatchedFrom() const {return y_matched_from_;}
    inline double yawMatchedFrom() const {return yaw_matched_from_;}

    inline double xMatched() const {return x_matched_;}
    inline double yMatched() const {return y_matched_;}

    inline double speed() const {return speed_;}
    inline double width() const {return width_;}
    inline double length() const {return length_;}
    
    inline double turnSignal() const {return turn_signal_;}

    inline RoutePlanner::RndfEdge* edge() const {return edge_;}
    inline std::map<RoutePlanner::RndfEdge*, double>& edges() {return edges_;}
    inline const std::map<RoutePlanner::RndfEdge*, double>& edges() const {return edges_;}

    inline double matchingError() const {return matching_error_;}
    inline double distFromStart() const {return dist_from_start_;}
    inline double distToEnd() const {return dist_to_end_;}
    inline bool isBlocking() const {return is_blocking_;}
    inline void isBlocking(bool blocking) {is_blocking_ = blocking;}
    inline const VehicleManager* vehicleManager() const {return vman_;}
    inline void vehicleManager(VehicleManager* vman) {vman_ = vman;}
    inline const std::vector<vlr::MovingBox>& predictedTrajectory() const {return predicted_traj_;}

    void update( const PerceptionDynamicObstacle& vehicle, double offset_x, double offset_y);
	void update( const double x, const double y, const double yaw, const double speed );

	// "skip_annotation" means: nothing is annotated in the graph. this
	// is currently done for the ego vehicle.
	void match_to_graph( std::map<int, RoutePlanner::RndfEdge*>&, bool skip_graph_annotation = false );
	void match_to_mission_graph( std::map<int, RoutePlanner::RndfEdge*>, bool skip_graph_annotation = false );
	void rematch_in_intersection( bool skip_graph_annotation = false );

	// hacked functions for the site visit
	bool isAtStopline() const;
	bool isOnIntersection(RoutePlanner::RndfGraph* graph = NULL) const;
	bool isOnIntersection(double min_offset) const;
	bool isOnStopLane() const;

	// new generalized funcs
	bool isAtStopline(const RoutePlanner::RndfIntersection* intersection) const;
	bool isOnIntersection(const RoutePlanner::RndfIntersection* intersection) const;
//	bool isOnIntersection(const RoutePlanner::RndfIntersection* intersection, double min_offset) const;
//	bool isOnStopLane(const RoutePlanner::RndfIntersection* intersection) const;

	//! calculates the minimal distance of a vehicles bumpers to the given \a intersection
	/*! \remark the pos of the vehicle is assumed in the center of the vehicle */
	double distToIntersection(const RoutePlanner::RndfIntersection* intersection = NULL) const;

	//! calculates the minimal distance of a vehicles frontbumper to the next stopline of the given \a intersection
	/*! \remark the pos of the vehicle is assumed in the center of the vehicle */
	double distToStopLine(const RoutePlanner::RndfIntersection* intersection = NULL) const;

	//! returns the distance of the vehicle to the point of the matched edge
	double distToMatchedEdge() const;
	//! returns the distance of the vehicle to the \a edge
	double distToEdge(RoutePlanner::RndfEdge* edge) const;
	//! returns the delta angle of the vehicle to the matched edge out of [0, Pi]
	double angleToMatchedEdge() const;
	//! returns the delta angle of the vehicle to the \a edge out of [0, Pi]
	double angleToEdge(RoutePlanner::RndfEdge* edge) const;

	//! moves the position of the vehicle (does not recalulate matchings)
	void move(CGAL_Geometry::Vector_2 delta_v);

	//! test if two vehicles intersects
	bool intersects(const Vehicle& veh) const;

	//	GraphPlace getGraphPlace() const;

	void blockMatchedEdges(bool block_multi_matching_edges = true);



	CGAL_Geometry::Point_2 point() const {return Point_2( x_matched_from_, y_matched_from_); }

	class circle {
	public:
	  circle() {
	  }
	  circle(double x_, double y_, double r_) :
	    x(x_), y(y_), r(r_) {
	  }
	  virtual ~circle() {
	  }

	  double x, y, r;
	};

	void predict(double t0, double checked_horizon, double time_sample_res);

	 // Generates NUM_CIRCLES Circles with a diameter of WIDTH and centers along the car from the front to the back
	 static void createCircles(MovingBox& mb);

	 // Generates the circum circle around all circles of car2Circles()
	 static void createCircumCircle(MovingBox& mb);


private:
	double fwdDistToIntersection(const RoutePlanner::RndfIntersection* intersection, const RoutePlanner::RndfEdge& edge, double act_dist) const;
	double bwdDistToIntersection(const RoutePlanner::RndfIntersection* intersection, const RoutePlanner::RndfEdge& edge, double act_dist) const;

	double fwdDistToStopLine(const RoutePlanner::RndfIntersection* intersection, const RoutePlanner::RndfEdge& edge, double act_dist) const;
	double bwdDistToStopLine(const RoutePlanner::RndfIntersection* intersection, const RoutePlanner::RndfEdge& edge, double act_dist) const;

  RoutePlanner::RndfEdge* bestPredictedEdge(const RoutePlanner::TRndfEdgeSet& edges);
  RoutePlanner::RndfEdge* bestPredictedEdge(const std::map<RoutePlanner::RndfEdge*, double>& edges, double& dist_to_end);


private:
	int32_t id_;

	// position that was acquired by sensor system
	double x_matched_from_;
	double y_matched_from_;
	double yaw_matched_from_;

	double x_matched_;
	double y_matched_;

	double speed_;
	
	char turn_signal_;

	double width_, length_;
    RoutePlanner::RndfEdge* edge_;                     // result of map matching
    std::map<RoutePlanner::RndfEdge*, double> edges_;  // result of multi matching (used in intersections)

	double matching_error_;

	double dist_from_start_;
	double dist_to_end_;

	//! true if this vehicle is clasified as blockage
	bool is_blocking_;

        // link to VehicleManager
	VehicleManager* vman_;
  std::vector<vlr::MovingBox> predicted_traj_;
};

} // namespace vlr

#endif // AW_VEHICLE_H
