/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file triangulation.h
 * @brief Functions for triangulation
 * @date July 31, 2013
 * @author Chris Beall
 */

#pragma once

#include <gtsam/slam/TriangulationFactor.h>
#include <gtsam/slam/PriorFactor.h>
#include <gtsam/nonlinear/NonlinearFactorGraph.h>
#include <gtsam/inference/Symbol.h>

namespace gtsam {

/// Exception thrown by triangulateDLT when SVD returns rank < 3
class TriangulationUnderconstrainedException: public std::runtime_error {
public:
  TriangulationUnderconstrainedException() :
      std::runtime_error("Triangulation Underconstrained Exception.") {
  }
};

/// Exception thrown by triangulateDLT when landmark is behind one or more of the cameras
class TriangulationCheiralityException: public std::runtime_error {
public:
  TriangulationCheiralityException() :
      std::runtime_error(
          "Triangulation Cheirality Exception: The resulting landmark is behind one or more cameras.") {
  }
};

/**
 * DLT triangulation: See Hartley and Zisserman, 2nd Ed., page 312
 * @param projection_matrices Projection matrices (K*P^-1)
 * @param measurements 2D measurements
 * @param rank_tol SVD rank tolerance
 * @return Triangulated point, in homogeneous coordinates
 */
GTSAM_EXPORT Vector4 triangulateHomogeneousDLT(
    const std::vector<Matrix34>& projection_matrices,
    const std::vector<Point2>& measurements, double rank_tol = 1e-9);

/**
 * DLT triangulation: See Hartley and Zisserman, 2nd Ed., page 312
 * @param projection_matrices Projection matrices (K*P^-1)
 * @param measurements 2D measurements
 * @param rank_tol SVD rank tolerance
 * @return Triangulated Point3
 */
GTSAM_EXPORT Point3 triangulateDLT(
    const std::vector<Matrix34>& projection_matrices,
    const std::vector<Point2>& measurements, double rank_tol = 1e-9);

///
/**
 * Create a factor graph with projection factors from poses and one calibration
 * @param poses Camera poses
 * @param sharedCal shared pointer to single calibration object
 * @param measurements 2D measurements
 * @param landmarkKey to refer to landmark
 * @param initialEstimate
 * @return graph and initial values
 */
template<class CALIBRATION>
std::pair<NonlinearFactorGraph, Values> triangulationGraph(
    const std::vector<Pose3>& poses, boost::shared_ptr<CALIBRATION> sharedCal,
    const std::vector<Point2>& measurements, Key landmarkKey,
    const Point3& initialEstimate) {
  Values values;
  values.insert(landmarkKey, initialEstimate); // Initial landmark value
  NonlinearFactorGraph graph;
  static SharedNoiseModel unit2(noiseModel::Unit::Create(2));
  static SharedNoiseModel prior_model(noiseModel::Isotropic::Sigma(6, 1e-6));
  for (size_t i = 0; i < measurements.size(); i++) {
    const Pose3& pose_i = poses[i];
    PinholeCamera<CALIBRATION> camera_i(pose_i, *sharedCal);
    graph.push_back(TriangulationFactor<CALIBRATION> //
        (camera_i, measurements[i], unit2, landmarkKey));
  }
  return std::make_pair(graph, values);
}

/**
 * Create a factor graph with projection factors from pinhole cameras
 * (each camera has a pose and calibration)
 * @param cameras pinhole cameras
 * @param measurements 2D measurements
 * @param landmarkKey to refer to landmark
 * @param initialEstimate
 * @return graph and initial values
 */
template<class CAMERA>
std::pair<NonlinearFactorGraph, Values> triangulationGraph(
    const std::vector<CAMERA>& cameras,
    const std::vector<Point2>& measurements, Key landmarkKey,
    const Point3& initialEstimate) {
  Values values;
  values.insert(landmarkKey, initialEstimate); // Initial landmark value
  NonlinearFactorGraph graph;
  static SharedNoiseModel unit2(noiseModel::Unit::Create(2));
  static SharedNoiseModel prior_model(noiseModel::Isotropic::Sigma(6, 1e-6));
  for (size_t i = 0; i < measurements.size(); i++) {
    const CAMERA& camera_i = cameras[i];
    graph.push_back(TriangulationFactor<typename CAMERA::CalibrationType> //
        (camera_i, measurements[i], unit2, landmarkKey));
  }
  return std::make_pair(graph, values);
}

///
/**
 * Optimize for triangulation
 * @param graph nonlinear factors for projection
 * @param values initial values
 * @param landmarkKey to refer to landmark
 * @return refined Point3
 */
GTSAM_EXPORT Point3 optimize(const NonlinearFactorGraph& graph,
    const Values& values, Key landmarkKey);

/**
 * Given an initial estimate , refine a point using measurements in several cameras
 * @param poses Camera poses
 * @param sharedCal shared pointer to single calibration object
 * @param measurements 2D measurements
 * @param initialEstimate
 * @return refined Point3
 */
template<class CALIBRATION>
Point3 triangulateNonlinear(const std::vector<Pose3>& poses,
    boost::shared_ptr<CALIBRATION> sharedCal,
    const std::vector<Point2>& measurements, const Point3& initialEstimate) {

  // Create a factor graph and initial values
  Values values;
  NonlinearFactorGraph graph;
  boost::tie(graph, values) = triangulationGraph(poses, sharedCal, measurements,
      Symbol('p', 0), initialEstimate);

  return optimize(graph, values, Symbol('p', 0));
}

/**
 * Given an initial estimate , refine a point using measurements in several cameras
 * @param cameras pinhole cameras
 * @param measurements 2D measurements
 * @param initialEstimate
 * @return refined Point3
 */
template<class CAMERA>
Point3 triangulateNonlinear(
    const std::vector<CAMERA>& cameras,
    const std::vector<Point2>& measurements, const Point3& initialEstimate) {

  // Create a factor graph and initial values
  Values values;
  NonlinearFactorGraph graph;
  boost::tie(graph, values) = triangulationGraph(cameras, measurements,
      Symbol('p', 0), initialEstimate);

  return optimize(graph, values, Symbol('p', 0));
}

/**
 * Create a 3*4 camera projection matrix from calibration and pose.
 * Functor for partial application on calibration
 * @param pose The camera pose
 * @param cal  The calibration
 * @return Returns a Matrix34
 */
template<class CALIBRATION>
struct CameraProjectionMatrix {
  CameraProjectionMatrix(const CALIBRATION& calibration) :
      K_(calibration.K()) {
  }
  Matrix34 operator()(const Pose3& pose) const {
    return K_ * (pose.inverse().matrix()).block<3, 4>(0, 0);
  }
private:
  const Matrix3 K_;
};

/**
 * Function to triangulate 3D landmark point from an arbitrary number
 * of poses (at least 2) using the DLT. The function checks that the
 * resulting point lies in front of all cameras, but has no other checks
 * to verify the quality of the triangulation.
 * @param poses A vector of camera poses
 * @param sharedCal shared pointer to single calibration object
 * @param measurements A vector of camera measurements
 * @param rank_tol rank tolerance, default 1e-9
 * @param optimize Flag to turn on nonlinear refinement of triangulation
 * @return Returns a Point3
 */
template<class CALIBRATION>
Point3 triangulatePoint3(const std::vector<Pose3>& poses,
    boost::shared_ptr<CALIBRATION> sharedCal,
    const std::vector<Point2>& measurements, double rank_tol = 1e-9,
    bool optimize = false) {

  assert(poses.size() == measurements.size());
  if (poses.size() < 2)
    throw(TriangulationUnderconstrainedException());

  // construct projection matrices from poses & calibration
  std::vector<Matrix34> projection_matrices;
  CameraProjectionMatrix<CALIBRATION> createP(*sharedCal); // partially apply
  BOOST_FOREACH(const Pose3& pose, poses)
    projection_matrices.push_back(createP(pose));

  // Triangulate linearly
  Point3 point = triangulateDLT(projection_matrices, measurements, rank_tol);

  // The n refine using non-linear optimization
  if (optimize)
    point = triangulateNonlinear(poses, sharedCal, measurements, point);

#ifdef GTSAM_THROW_CHEIRALITY_EXCEPTION
  // verify that the triangulated point lies infront of all cameras
  BOOST_FOREACH(const Pose3& pose, poses) {
    const Point3& p_local = pose.transform_to(point);
    if (p_local.z() <= 0)
      throw(TriangulationCheiralityException());
  }
#endif

  return point;
}

/**
 * Function to triangulate 3D landmark point from an arbitrary number
 * of poses (at least 2) using the DLT. This function is similar to the one
 * above, except that each camera has its own calibration. The function
 * checks that the resulting point lies in front of all cameras, but has
 * no other checks to verify the quality of the triangulation.
 * @param cameras pinhole cameras
 * @param measurements A vector of camera measurements
 * @param rank_tol rank tolerance, default 1e-9
 * @param optimize Flag to turn on nonlinear refinement of triangulation
 * @return Returns a Point3
 */
template<class CAMERA>
Point3 triangulatePoint3(
    const std::vector<CAMERA>& cameras,
    const std::vector<Point2>& measurements, double rank_tol = 1e-9,
    bool optimize = false) {

  size_t m = cameras.size();
  assert(measurements.size() == m);

  if (m < 2)
    throw(TriangulationUnderconstrainedException());

  // construct projection matrices from poses & calibration
  typedef PinholeCamera<typename CAMERA::CalibrationType> Camera;
  std::vector<Matrix34> projection_matrices;
  BOOST_FOREACH(const Camera& camera, cameras)
    projection_matrices.push_back(
        CameraProjectionMatrix<typename CAMERA::CalibrationType>(camera.calibration())(
            camera.pose()));
  Point3 point = triangulateDLT(projection_matrices, measurements, rank_tol);

  // The n refine using non-linear optimization
  if (optimize)
    point = triangulateNonlinear(cameras, measurements, point);

#ifdef GTSAM_THROW_CHEIRALITY_EXCEPTION
  // verify that the triangulated point lies infront of all cameras
  BOOST_FOREACH(const Camera& camera, cameras) {
    const Point3& p_local = camera.pose().transform_to(point);
    if (p_local.z() <= 0)
      throw(TriangulationCheiralityException());
  }
#endif

  return point;
}

} // \namespace gtsam
