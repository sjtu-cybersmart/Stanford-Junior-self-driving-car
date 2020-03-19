/*--------------------------------------------------------------------
 * project ....: Darpa Urban Challenge 2007
 * file .......: rndf_lane_quadtree.cpp
 * authors ....: Benjamin Pitzer
 * organization: Stanford University
 * creation ...: 02/15/2007
 * revisions ..: $Id: rndf_lane_quadtree.cpp,v 1.4 2007-04-16 04:51:22 pitzer Exp $
---------------------------------------------------------------------*/
#define DEBUG_LEVEL 0

//== INCLUDES =================================================================
#include "geometry_2d.h"
#include "rndf_lane_quadtree.h"

//== NAMESPACES ===============================================================
using namespace std;

namespace dgc {

//== IMPLEMENTATION ==========================================================
rndf_lane_quadtree::
rndf_lane_quadtree(
  rndf_file*            rndf, 
  double                min_radius, 
  int                   min_elements) 
  : m_rndf(rndf), m_tile_id(1)
{
  sla::Vec2d bb_min(1e16);
  sla::Vec2d bb_max(-1e16);
  int i, j, k, l;
  element_list_t elements;
  element_p element;  
  int num_segments,num_lanes,num_waypoints,num_exits;
  
  // calculate bounding box and create elements
  num_segments = rndf->num_segments();
  for(i = 0; i < num_segments; i++) {
    num_lanes = rndf->segment(i)->num_lanes();
    for(j = 0; j < num_lanes; j++) {
      num_waypoints = rndf->segment(i)->lane(j)->num_waypoints();
      for(k = 0; k < num_waypoints; k++) {
        // calculate bounding box
        bb_min.x[0] = min(bb_min.x[0],rndf->segment(i)->lane(j)->waypoint(k)->utm_x());
        bb_max.x[0] = max(bb_max.x[0],rndf->segment(i)->lane(j)->waypoint(k)->utm_x());
        bb_min.x[1] = min(bb_min.x[1],rndf->segment(i)->lane(j)->waypoint(k)->utm_y());
        bb_max.x[1] = max(bb_max.x[1],rndf->segment(i)->lane(j)->waypoint(k)->utm_y());
        
        // create elements
        if(k<num_waypoints-1) {
          element = (element_p)malloc(sizeof(element_t));
          element->s = rndf->segment(i);
          element->l = rndf->segment(i)->lane(j);
          element->w1 = rndf->segment(i)->lane(j)->waypoint(k);
          element->w2 = rndf->segment(i)->lane(j)->waypoint(k+1);
          elements.push_back(element);
        } // k<num_waypoints-1
        
        // add exiting waypoints as new segments
        num_exits = rndf->segment(i)->lane(j)->waypoint(k)->num_exits();
        for(l = 0; l < num_exits; l++) {
          element = (element_p)malloc(sizeof(element_t));
          element->s = rndf->segment(i);
          element->l = rndf->segment(i)->lane(j);
          element->w1 = rndf->segment(i)->lane(j)->waypoint(k);
          element->w2 = rndf->segment(i)->lane(j)->waypoint(k)->exit(l);
          elements.push_back(element);
        }
      } // k
    } // j
  } // i
      
  // set members 
  m_center = bb_min+(bb_max-bb_min)/2.0;
  m_radius = (bb_max-bb_min).max();
  m_parent_index = 0;
  m_parent_tree = NULL;
  
  // initialize quadtree
  initialize(elements,min_radius,min_elements);
}

rndf_lane_quadtree::
rndf_lane_quadtree(
  rndf_file*            rndf, 
  sla::Vec2d&           center,  
  double                radius, 
  double                min_radius, 
  int                   min_elements)
  : m_rndf(rndf), m_center(center), m_radius(radius), m_tile_id(1)
{
  int i, j, k;
  element_list_t elements;
  element_p element;  
  int num_segments,num_lanes,num_waypoints;
  
  // calculate bounding box and create elements
  num_segments = rndf->num_segments();
  for(i = 0; i < num_segments; i++) {
    num_lanes = rndf->segment(i)->num_lanes();
    for(j = 0; j < num_lanes; j++) {
      num_waypoints = rndf->segment(i)->lane(j)->num_waypoints();
      for(k = 0; k < num_waypoints; k++) {
        // create elements
        if(k<num_waypoints-1) {
          element = (element_p)malloc(sizeof(element_t));
          element->s = rndf->segment(i);
          element->l = rndf->segment(i)->lane(j);
          element->w1 = rndf->segment(i)->lane(j)->waypoint(k);
          element->w2 = rndf->segment(i)->lane(j)->waypoint(k+1);
          elements.push_back(element);
        } // k<num_waypoints-1
      } // k
    } // j
  } // i
      
  // set members 
  m_parent_index = 0;
  m_parent_tree = NULL;
  
  // initialize quadtree
  initialize(elements,min_radius,min_elements);
}

rndf_lane_quadtree::
rndf_lane_quadtree(
  rndf_file*            rndf, 
  sla::Vec2d&           center, 
  double                radius, 
  const element_list_t& elements,
  double                min_radius, 
  int                   min_elements, 
  int                   parent_index, 
  rndf_lane_quadtree*   parent_tree)
  : m_rndf(rndf), m_center(center), m_radius(radius),
    m_parent_index(parent_index), m_parent_tree(parent_tree)
{
  if(m_parent_tree)
    m_tile_id = m_parent_tree->tile_id()*10+parent_index;
  else
    m_tile_id=1;
    
  initialize(elements,min_radius,min_elements);
}

rndf_lane_quadtree::
~rndf_lane_quadtree()
{
  element_p element;
  if (m_is_leaf) {
    element_list_t::const_iterator it, it_end;
    for(it=m_elements.begin(), it_end=m_elements.end(); it!=it_end; ++it) {
      element = *it;
      if(element) {
          // TODO: keep a list of elements in all trees. This is easier to to free
          //       because this doesn't work! 
//        free(element);
//        element=NULL;
      }
    }
    return;
  }
  for (int i=0; i<4; i++) {
    if (m_child[i]) 
      delete m_child[i];
  }
}

void rndf_lane_quadtree::
initialize(const element_list_t& elements, double min_radius, int min_elements)
{
  bool subdivided=false;
  double element_x[2];
  double element_y[2];
  double center_x = m_center(0);
  double center_y = m_center(1);
  
  m_elements.clear();
  m_is_leaf = false;

  // check whether we need to subdivide more
  if ((m_radius < 2.0*min_radius) || elements.size()==1) {
    m_is_leaf = true;
    m_elements = elements;
    m_child[0] = m_child[1] = m_child[2] = m_child[3] = NULL;
    return;
  }

  // if so, try for triangle and each child whether they intersect
  double half_radius = m_radius*.5;
  sla::Vec2d child_center;

  element_list_t xp, xm; // x plus, x minus
  element_list_t yp, ym; // y plus, y minus
  element_list_t uk;     // unknown (elements going across splits)
  xp.reserve(elements.size());
  xm.reserve(elements.size());
  uk.reserve(elements.size());

  // split about x axis
  int cnt;
  element_list_t::const_iterator it, it_end;
  for(it=elements.begin(), it_end=elements.end(); it!=it_end; ++it)
  {
    element_x[0] = (*it)->w1->utm_x();
    element_x[1] = (*it)->w2->utm_x();
    
    cnt = ((element_x[0] >= center_x) + (element_x[1] >= center_x));
    if (cnt == 2) {
      xp.push_back(*it); 
    } else if (cnt == 0) {
      xm.push_back(*it); 
    } else {      
      uk.push_back(*it);
    }  
  }
 
  // split about y axis
  element_list_t &xv = xm;
  for (int i=0; i<2; i++) {
    if (i) xv = xp;
    else   xv = xm;
    yp.clear(); yp.reserve(xv.size());
    ym.clear(); ym.reserve(xv.size());

    for(it=xv.begin(), it_end=xv.end(); it!=it_end; ++it) 
    {
      element_y[0] = (*it)->w1->utm_y();
      element_y[1] = (*it)->w2->utm_y();
    
      cnt = ((element_y[0] >= center_y) + (element_y[1] >= center_y));
      if (cnt == 2) {
        yp.push_back(*it);
      } else if (cnt == 0) {
        ym.push_back(*it);
      } else {
        uk.push_back(*it);
      }  
    }

    // check the unknowns
    child_center(0) = m_center(0) + (i ? half_radius : -half_radius);
    child_center(1) = m_center(1) - half_radius;
    element_list_t::const_iterator it1, it1_end;
    for(it=uk.begin(), it_end=uk.end(); it!=it_end; ++it) 
    {
      if(element_inside_quad(child_center,half_radius,*it))
        ym.push_back(*it);
    }

    // create the children, if needed
    int k = i;
    if (ym.size()>0) {    
      m_child[k] = new rndf_lane_quadtree(m_rndf, child_center, half_radius, 
                                          ym, min_radius, min_elements, 
                                          k, this);
      subdivided = true;
    } else {
      m_child[k] = NULL;
    }
    
    // create the children, if needed
    child_center(1) = m_center(1) + half_radius;
    k += 2;
    for(it=uk.begin(), it_end=uk.end(); it!=it_end; ++it) 
    {
      if(element_inside_quad(child_center,half_radius,*it))
        yp.push_back(*it);
    }

    if (yp.size()>0) {
      m_child[k] = new rndf_lane_quadtree(m_rndf,child_center,half_radius, 
                                          yp, min_radius, min_elements, 
                                          k, this);
      subdivided = true;
    } else {
      m_child[k] = NULL;
    }
  }

  // no subdivision made
  if(!subdivided)
  {
    m_elements = elements;
    m_is_leaf = true;
    m_child[0] = m_child[1] = m_child[2] = m_child[3] = NULL;
  }
}

bool rndf_lane_quadtree::
element_inside_quad(const sla::Vec2d& center, double radius, element_p element)
{
  sla::Vec2d t1(element->w2->utm_x(),element->w2->utm_y());
  sla::Vec2d t2(element->w1->utm_x(),element->w1->utm_y());
  bool outside = line_outside_of_rect(center, radius*2.0, t1, t2);
  return (!outside);
}

bool rndf_lane_quadtree::
closer_to_element(const sla::Vec2d& p, element_p element, 
                  double& distance, sla::Vec2d& cp) 
{
  sla::Vec2d a(element->w1->utm_x(),element->w1->utm_y());
  sla::Vec2d b(element->w2->utm_x(),element->w2->utm_y());
  return closer_on_line(p,a,b,distance,cp);
}

bool rndf_lane_quadtree::
search_nearest_neighbor(const sla::Vec2d& p, element_p& element, double& distance, sla::Vec2d& closest_point)
{
  // is this a leaf node?
  if (m_is_leaf) {
    // look for a new closest distance
    element_list_t::const_iterator it, it_end;
    for(it=m_elements.begin(), it_end=m_elements.end(); it!=it_end; ++it) 
    {
      // check if distance to this element is smaller
      if(closer_to_element(p,*it,distance,closest_point))
        element = *it;
    }
    return circle_within_bounds(p, sqrt(distance), m_center, m_radius);
  }

  // which child contains p?
  int iChild = 2*(m_center(1)<p(1)) + (m_center(0)<p(0));
  // check that child first
  if (m_child[iChild] && m_child[iChild]->search_nearest_neighbor(p, element, distance, closest_point)) return true;

  // now see if the other children need to be checked
  for (int i=0; i<4; i++) {
    if (i==iChild) continue;
    if (m_child[i]) {
      if(bounds_overlap_circle(p,sqrt(distance),m_child[i]->m_center, m_child[i]->m_radius)) {
        if (m_child[i]->search_nearest_neighbor(p, element, distance, closest_point)) return true;
      }
    }
  }
  return circle_within_bounds(p, sqrt(distance), m_center, m_radius);
}

void rndf_lane_quadtree::
get_tiles(std::vector<sla::Vec2d> &points, std::vector<int> &edges, std::vector<unsigned long long> &tile_ids) 
{
  if (m_is_leaf) {
    int s = points.size();
    points.push_back(m_center + sla::Vec2d(-m_radius,-m_radius));//0
    points.push_back(m_center + sla::Vec2d( m_radius,-m_radius));//1
    points.push_back(m_center + sla::Vec2d( m_radius, m_radius));//2
    points.push_back(m_center + sla::Vec2d(-m_radius, m_radius));//3
    
    edges.push_back(s+0);edges.push_back(s+1);
    edges.push_back(s+1);edges.push_back(s+2);
    edges.push_back(s+2);edges.push_back(s+3);
    edges.push_back(s+3);edges.push_back(s+0);
    
    tile_ids.push_back(m_tile_id);
    return;
  }
  for (int i=0; i<4; i++) {
    if (m_child[i]) 
      m_child[i]->get_tiles(points,edges,tile_ids);
  }
}

void rndf_lane_quadtree::
get_elements(element_list_t& elements) 
{
  if (m_is_leaf) {
    element_list_t::const_iterator it, it_end;
    for(it=m_elements.begin(), it_end=m_elements.end(); it!=it_end; ++it)
      elements.push_back(*it);
    return;
  }
  for (int i=0; i<4; i++) {
    if (m_child[i]) 
      m_child[i]->get_elements(elements);
  }
}

  
//=============================================================================
} // namespace dgc
//=============================================================================
