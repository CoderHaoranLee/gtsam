/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/*
 * @brief Unit tests for serialization of library classes
 * @author Frank Dellaert
 * @author Alex Cunningham
 */

/* ************************************************************************* */
// Serialization testing code.
/* ************************************************************************* */

#include <fstream>
#include <sstream>
#include <string>

// includes for standard serialization types
#include <boost/serialization/optional.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/list.hpp>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

// whether to print the serialized text to stdout
const bool verbose = false;

// Templated round-trip serialization
template<class T>
void roundtrip(const T& input, T& output) {
	// Serialize
	std::ostringstream out_archive_stream;
	{
		boost::archive::text_oarchive out_archive(out_archive_stream);
		out_archive << input;
	}

	// Convert to string
	std::string serialized = out_archive_stream.str();
	if (verbose) std::cout << serialized << std::endl << std::endl;

	// De-serialize
	{
		std::istringstream in_archive_stream(serialized);
		boost::archive::text_iarchive in_archive(in_archive_stream);
		in_archive >> output;
	}
}

// This version requires equality operator
template<class T>
bool equality(const T& input = T()) {
	T output;
	roundtrip<T>(input,output);
	return input==output;
}

// This version requires equals
template<class T>
bool equalsObj(const T& input = T()) {
	T output;
	roundtrip<T>(input,output);
	return input.equals(output);
}

// De-referenced version for pointers
template<class T>
bool equalsDereferenced(const T& input) {
	T output;
	roundtrip<T>(input,output);
	return input->equals(*output);
}

/* ************************************************************************* */
// Templated round-trip serialization using XML
template<class T>
void roundtripXML(const T& input, T& output) {
	// Serialize
	std::string serialized;
	{
		std::ostringstream out_archive_stream;
		boost::archive::xml_oarchive out_archive(out_archive_stream);
		out_archive << boost::serialization::make_nvp("data", input);

		// Convert to string
		serialized = out_archive_stream.str();
	}
	if (verbose) std::cout << serialized << std::endl << std::endl;

	// De-serialize
	std::istringstream in_archive_stream(serialized);
	boost::archive::xml_iarchive in_archive(in_archive_stream);
	in_archive >> boost::serialization::make_nvp("data", output);
}

// This version requires equality operator
template<class T>
bool equalityXML(const T& input = T()) {
	T output;
	roundtripXML<T>(input,output);
	return input==output;
}

// This version requires equals
template<class T>
bool equalsXML(const T& input = T()) {
	T output;
	roundtripXML<T>(input,output);
	return input.equals(output);
}

// This version is for pointers
template<class T>
bool equalsDereferencedXML(const T& input = T()) {
	T output;
	roundtripXML<T>(input,output);
	return input->equals(*output);
}

/* ************************************************************************* */
// Actual Tests
/* ************************************************************************* */

#include <gtsam/geometry/Point2.h>
#include <gtsam/geometry/Pose2.h>
#include <gtsam/geometry/Rot2.h>
#include <gtsam/geometry/Point3.h>
#include <gtsam/geometry/Pose3.h>
#include <gtsam/geometry/Rot3.h>
#include <gtsam/geometry/Cal3_S2.h>

//#include <gtsam/linear/VectorValues.h>
//#include <gtsam/linear/GaussianConditional.h>
//#include <gtsam/inference/SymbolicConditional.h>

#include <gtsam/slam/planarSLAM.h>

#include <CppUnitLite/TestHarness.h>

using namespace std;
using namespace gtsam;

/* ************************************************************************* */
TEST (Serialization, text_geometry) {
	EXPECT(equalsObj<gtsam::Point2>(Point2(1.0, 2.0)));
	EXPECT(equalsObj<gtsam::Pose2>(Pose2(1.0, 2.0, 0.3)));
	EXPECT(equalsObj<gtsam::Rot2>(Rot2::fromDegrees(30.0)));

	Point3 pt3(1.0, 2.0, 3.0);
	Rot3 rt3 = Rot3::RzRyRx(1.0, 3.0, 2.0);
	EXPECT(equalsObj<gtsam::Point3>(pt3));
	EXPECT(equalsObj<gtsam::Rot3>(rt3));
	EXPECT(equalsObj<gtsam::Pose3>(Pose3(rt3, pt3)));
}

/* ************************************************************************* */
TEST (Serialization, xml_geometry) {
	EXPECT(equalsXML<gtsam::Point2>(Point2(1.0, 2.0)));
	EXPECT(equalsXML<gtsam::Pose2>(Pose2(1.0, 2.0, 0.3)));
	EXPECT(equalsXML<gtsam::Rot2>(Rot2::fromDegrees(30.0)));

	Point3 pt3(1.0, 2.0, 3.0);
	Rot3 rt3 = Rot3::RzRyRx(1.0, 3.0, 2.0);
	EXPECT(equalsXML<gtsam::Point3>(pt3));
	EXPECT(equalsXML<gtsam::Rot3>(rt3));
	EXPECT(equalsXML<gtsam::Pose3>(Pose3(rt3, pt3)));

}

/* ************************************************************************* */
TEST (Serialization, text_linear) {
	// NOT WORKING:
//	EXPECT(equalsObj<VectorValues>());
//	EXPECT(equalsObj<GaussianConditional>());
}

/* ************************************************************************* */
TEST (Serialization, xml_linear) {
	// NOT WORKING:
//	EXPECT(equalsXML<VectorValues>());
//	EXPECT(equalsXML<GaussianConditional>());
}

/* ************************************************************************* */
TEST (Serialization, noiseModels) {
	SharedDiagonal diag3 = noiseModel::Diagonal::Sigmas(Vector_(3, 0.1, 0.2, 0.3));
	SharedGaussian model3 = noiseModel::Isotropic::Sigma(3, 0.3);

	EXPECT(equalsDereferenced<SharedDiagonal>(diag3));
	EXPECT(equalsDereferencedXML<SharedDiagonal>(diag3));

	// FAIL: Segfaults
//	EXPECT(equalsDereferenced<SharedGaussian>(model3));
//	EXPECT(equalsDereferencedXML<SharedGaussian>(model3));
}

/* ************************************************************************* */
TEST (Serialization, planar_system) {
	using namespace planarSLAM;

	Values values;
	values.insert(PointKey(3), Point2(1.0, 2.0));
	values.insert(PoseKey(4), Pose2(1.0, 2.0, 0.3));

	SharedGaussian model1 = noiseModel::Isotropic::Sigma(1, 0.3);
	SharedGaussian model2 = noiseModel::Isotropic::Sigma(2, 0.3);
	SharedGaussian model3 = noiseModel::Isotropic::Sigma(3, 0.3);
	Graph graph;
	graph.addBearing(PoseKey(3), PointKey(5), Rot2::fromDegrees(0.5), model1);
	graph.addRange(PoseKey(2), PointKey(9), 7.0, model1);
	graph.addBearingRange(PoseKey(2), PointKey(3), Rot2::fromDegrees(0.6), 2.0, model2);
	graph.addOdometry(PoseKey(2), PoseKey(3), Pose2(1.0, 2.0, 0.3), model3);

	// text
	EXPECT(equalsObj<PoseKey>(PoseKey(2)));
	EXPECT(equalsObj<PointKey>(PointKey(3)));
	EXPECT(equalsObj<Values>(values));
//	EXPECT(equalsObj<Graph>(graph));

	// xml
	EXPECT(equalsXML<PoseKey>(PoseKey(2)));
	EXPECT(equalsXML<PointKey>(PointKey(3)));
	EXPECT(equalsXML<Values>(values));
//	EXPECT(equalsXML<Graph>(graph));
}

/* ************************************************************************* */
int main() {
	TestResult tr;
	return TestRegistry::runAllTests(tr);
}
/* ************************************************************************* */
