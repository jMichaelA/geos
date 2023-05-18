
//
// Test Suite for geos::triangulate::Voronoi
//
// tut
#include <tut/tut.hpp>
// geos
#include <geos/triangulate/quadedge/QuadEdge.h>
#include <geos/triangulate/quadedge/QuadEdgeSubdivision.h>
#include <geos/triangulate/IncrementalDelaunayTriangulator.h>
#include <geos/triangulate/VoronoiDiagramBuilder.h>

#include <geos/io/WKTWriter.h>
#include <geos/io/WKTReader.h>
#include <geos/io/WKBReader.h>
#include <geos/geom/GeometryCollection.h>
#include <geos/geom/GeometryFactory.h>

#include <geos/geom/CoordinateSequence.h>
//#include <stdio.h>
#include <iostream>
using namespace geos::triangulate;
using namespace geos::triangulate::quadedge;
using namespace geos::geom;
using namespace geos::io;

namespace tut {
//
// Test Group
//

// dummy data, not used
struct test_voronoidiag_data {
    test_voronoidiag_data()
    {
    }
};

typedef test_group<test_voronoidiag_data> group;
typedef group::object object;

group test_voronoidiag_group("geos::triangulate::Voronoi");

std::unique_ptr<Geometry>
readTextOrHex(const char* wkt)
{
    WKTReader wktreader;
    WKBReader wkbreader;
    std::string wktstr(wkt);

    if (wktstr == "") {
        return nullptr;
    }

    if ("01" == wktstr.substr(0, 2) || "00" == wktstr.substr(0, 2))
    {
        std::stringstream is(wkt);
        return wkbreader.readHEX(is);
    }
    else
        return wktreader.read(wktstr);
}

//helper function for running triangulation
void
runVoronoi(const char* sitesWkt, const char* expectedWkt, const double tolerance, const bool onlyEdges = false, const bool checkOrder = false)
{
    WKTWriter writer;
    geos::triangulate::VoronoiDiagramBuilder builder;
    std::unique_ptr<Geometry> sites(readTextOrHex(sitesWkt));
    std::unique_ptr<Geometry> expected(readTextOrHex(expectedWkt));
    std::unique_ptr<Geometry> results;
    const GeometryFactory& geomFact(*GeometryFactory::getDefaultInstance());
    builder.setSites(*sites);
    builder.setOrdered(checkOrder);

    //set Tolerance:
    builder.setTolerance(tolerance);
    if (onlyEdges) {
        results = builder.getDiagramEdges(geomFact);
    } else {
        results = builder.getDiagram(geomFact);
    }

    if (checkOrder) {
        if(sites->getGeometryTypeId() == GEOS_MULTIPOINT) {
            for (size_t i = 0; i < sites->getNumGeometries(); i++) {
                ensure("order is preserved", results->getGeometryN(i)->contains(sites->getGeometryN(i)) );
            }
        }
        if(sites->getGeometryTypeId() == GEOS_LINESTRING) {
            const auto& line = static_cast<const LineString&>(*sites);
            for (size_t i = 0; i < line.getNumPoints(); i++) {
                auto pt = line.getPointN(i);
                ensure("order is preserved", results->getGeometryN(i)->contains(pt.get()) );
            }
        }
    }

    results->normalize();
    expected->normalize();

    ensure_equals(static_cast<std::size_t>(results->getCoordinateDimension()),
                  static_cast<std::size_t>(expected->getCoordinateDimension()));
    bool eq = results->equalsExact(expected.get(), 1e-7);
    if(! eq) {
        writer.setTrim(true);
        std::cout << std::endl;
        std::cout << " Expected: " << writer.write(expected.get()) << std::endl;
        std::cout << " Obtained: " << writer.write(results.get()) << std::endl;
    }
    ensure(eq);

}

// Test Cases
// Basic Tests.
template<>
template<>
void object::test<1>
()
{
    using geos::geom::CoordinateSequence;
    Coordinate a(180, 300);
    Coordinate b(300, 290);
    Coordinate c(230, 330);
    Coordinate d(244, 284);

    geos::triangulate::VoronoiDiagramBuilder builder;
    CoordinateSequence seq;
    seq.add(a);
    seq.add(b);
    seq.add(c);
    seq.add(d);

    builder.setSites(seq);

    //getting the subdiv()
    std::unique_ptr<QuadEdgeSubdivision> subdiv = builder.getSubdivision();

    ensure_equals(subdiv->getTolerance(), 0);
    //-- disable this test, since it depends on Subdiv frame size and so is brittle
    //ensure_equals(subdiv->getEnvelope().toString(), "Env[-3540:4020,-3436:4050]");

}

// 1 - Case with a single point
template<>
template<>
void object::test<2>
()
{
    const char* wkt = "MULTIPOINT ((150 200))";
    const char* expected = "GEOMETRYCOLLECTION EMPTY";
    runVoronoi(wkt, expected, 0);
}

template<>
template<>
void object::test<3>
()
{
    const char* wkt = "MULTIPOINT ((150 200), (180 270), (275 163))";

    const char* expected =
        "GEOMETRYCOLLECTION (POLYGON ((25 38, 25 295, 221.20588235294116 210.91176470588235, 170.024 38, 25 38)), POLYGON ((400 369.6542056074766, 400 38, 170.024 38, 221.20588235294116 210.91176470588235, 400 369.6542056074766)), POLYGON ((25 295, 25 395, 400 395, 400 369.6542056074766, 221.20588235294116 210.91176470588235, 25 295)))";

    runVoronoi(wkt, expected, 0);
}

template<>
template<>
void object::test<4>
()
{
    const char* wkt = "MULTIPOINT ((280 300), (420 330), (380 230), (320 160))";

    const char* expected =
        "GEOMETRYCOLLECTION (POLYGON ((110 175.71428571428572, 110 500, 310.35714285714283 500, 353.515625 298.59375, 306.875 231.96428571428572, 110 175.71428571428572)), POLYGON ((590 204, 590 -10, 589.1666666666666 -10, 306.875 231.96428571428572, 353.515625 298.59375, 590 204)), POLYGON ((110 -10, 110 175.71428571428572, 306.875 231.96428571428572, 589.1666666666666 -10, 110 -10)), POLYGON ((310.35714285714283 500, 590 500, 590 204, 353.515625 298.59375, 310.35714285714283 500)))";
    runVoronoi(wkt, expected, 0);
}

template<>
template<>
void object::test<5>
()
{
    const char* wkt = "MULTIPOINT ((320 170), (366 246), (530 230), (530 300), (455 277), (490 160))";

    const char* expected =
        "GEOMETRYCOLLECTION (POLYGON ((110 -50, 110 349.02631578947364, 405.31091180866963 170.28550074738416, 392.35294117647055 -50, 110 -50)), POLYGON ((740 63.57142857142859, 740 -50, 392.35294117647055 -50, 405.31091180866963 170.28550074738416, 429.9147677857019 205.76082797008175, 470.12061711079946 217.7882187938289, 740 63.57142857142859)), POLYGON ((110 349.02631578947364, 110 510, 323.9438202247191 510, 429.9147677857019 205.76082797008175, 405.31091180866963 170.28550074738416, 110 349.02631578947364)),  POLYGON ((323.9438202247191 510, 424.57333333333327 510, 499.70666666666665 265, 470.12061711079946 217.7882187938289, 429.9147677857019 205.76082797008175, 323.9438202247191 510)),POLYGON ((740 265, 740 63.57142857142859, 470.12061711079946 217.7882187938289, 499.70666666666665 265, 740 265)), POLYGON ((424.57333333333327 510, 740 510, 740 265, 499.70666666666665 265, 424.57333333333327 510)))";
    runVoronoi(wkt, expected, 0);
}

//6. A little larger number of points
template<>
template<>
void object::test<6>
()
{
    const char* wkt =
        "MULTIPOINT ((280 200), (406 285), (580 280), (550 190), (370 190), (360 90), (480 110), (440 160), (450 180), (480 180), (460 160), (360 210), (360 220), (370 210), (375 227))";

    const char* expected =
        "GEOMETRYCOLLECTION (POLYGON ((-20 -102.27272727272727, -20 585, 111.9484126984127 585, 293.54906542056074 315.803738317757, 318.75 215, 323.2352941176471 179.11764705882354, 319.3956043956044 144.56043956043956, -20 -102.27272727272727)), POLYGON ((365 200, 365 215, 369.40909090909093 219.4090909090909, 414.2119205298013 206.2317880794702, 411.875 200, 365 200)), POLYGON ((365 215, 365 200, 323.2352941176471 179.11764705882354, 318.75 215, 365 215)), POLYGON ((-20 -210, -20 -102.27272727272727, 319.3956043956044 144.56043956043956, 388.972602739726 137.60273972602738, 419.55882352941177 102.64705882352942, 471.66666666666674 -210, -20 -210)), POLYGON ((319.3956043956044 144.56043956043956, 323.2352941176471 179.11764705882354, 365 200, 411.875 200, 410.29411764705884 187.35294117647058, 388.972602739726 137.60273972602738, 319.3956043956044 144.56043956043956)), POLYGON ((410.29411764705884 187.35294117647058, 411.875 200, 414.2119205298013 206.2317880794702, 431.62536593766146 234.01920096435336, 465 248.0047619047619, 465 175, 450 167.5, 410.29411764705884 187.35294117647058)), POLYGON ((365 215, 318.75 215, 293.54906542056074 315.803738317757, 339.6500765696784 283.1784073506891, 369.40909090909093 219.4090909090909, 365 215)), POLYGON ((111.9484126984127 585, 501.69252873563215 585, 492.5670391061452 267.4329608938547, 465 248.0047619047619, 431.62536593766146 234.01920096435336, 339.6500765696784 283.1784073506891, 293.54906542056074 315.803738317757, 111.9484126984127 585)),  POLYGON ((369.40909090909093 219.4090909090909, 339.6500765696784 283.1784073506891, 431.62536593766146 234.01920096435336, 414.2119205298013 206.2317880794702, 369.40909090909093 219.4090909090909)), POLYGON ((388.972602739726 137.60273972602738, 410.29411764705884 187.35294117647058, 450 167.5, 450 127, 419.55882352941177 102.64705882352942, 388.972602739726 137.60273972602738)), POLYGON ((465 175, 465 248.0047619047619, 492.5670391061452 267.4329608938547, 505 255, 520.7142857142857 145, 495 145, 465 175)),POLYGON ((880 -169.375, 880 -210, 471.66666666666674 -210, 419.55882352941177 102.64705882352942, 450 127, 495 145, 520.7142857142857 145, 880 -169.375)), POLYGON ((450 167.5, 465 175, 495 145, 450 127, 450 167.5)), POLYGON ((501.69252873563215 585, 880 585, 880 130.00000000000006, 505 255, 492.5670391061452 267.4329608938547, 501.69252873563215 585)), POLYGON ((880 130.00000000000006, 880 -169.375, 520.7142857142857 145, 505 255, 880 130.00000000000006)))";

    runVoronoi(wkt, expected, 0);
}

//Test with case of Tolerance:
template<>
template<>
void object::test<7>
()
{
    const char* wkt =
        "MULTIPOINT ((100 200), (105 202), (110 200), (140 230), (210 240), (220 190), (170 170), (170 260), (213 245), (220 190))";

    const char* expected =
        "GEOMETRYCOLLECTION (POLYGON ((-20 50, -20 380, -3.75 380, 105 235, 105 115, 77.14285714285714 50, -20 50)), POLYGON ((247 50, 77.14285714285714 50, 105 115, 145 195, 178.33333333333334 211.66666666666666, 183.51851851851853 208.7037037037037, 247 50)), POLYGON ((-3.75 380, 20.000000000000007 380, 176.66666666666666 223.33333333333334, 178.33333333333334 211.66666666666666, 145 195, 105 235, -3.75 380)), POLYGON ((105 115, 105 235, 145 195, 105 115)), POLYGON ((20.000000000000007 380, 255 380, 176.66666666666666 223.33333333333334, 20.000000000000007 380)), POLYGON ((255 380, 340 380, 340 240, 183.51851851851853 208.7037037037037, 178.33333333333334 211.66666666666666, 176.66666666666666 223.33333333333334, 255 380)), POLYGON ((340 240, 340 50, 247 50, 183.51851851851853 208.7037037037037, 340 240)))";

    runVoronoi(wkt, expected, 6);
}

template<>
template<>
void object::test<8>
()
{
    const char* wkt = "MULTIPOINT ((170 270), (177 275), (190 230), (230 250), (210 290), (240 280), (240 250))";

    const char* expected =
        "GEOMETRYCOLLECTION (POLYGON ((100 210, 100 360, 150 360, 200 260, 100 210)), POLYGON ((150 360, 250 360, 220 270, 200 260, 150 360)), POLYGON ((100 160, 100 210, 200 260, 235 190, 247 160, 100 160)), POLYGON ((220 270, 235 265, 235 190, 200 260, 220 270)), POLYGON ((250 360, 310 360, 310 265, 235 265, 220 270, 250 360)), POLYGON ((310 265, 310 160, 247 160, 235 190, 235 265, 310 265)))";

    runVoronoi(wkt, expected, 10);
}

//Taking tolerance very very high
template<>
template<>
void object::test<9>
()
{
    const char* wkt = "MULTIPOINT ((155 271), (150 360), (260 360), (271 265), (280 260), (270 370), (154 354), (150 260))";

    const char* expected =
        "GEOMETRYCOLLECTION (POLYGON ((20 130, 20 310, 205 310, 215 299, 215 130, 20 130)), POLYGON ((205 500, 410 500, 410 338, 215 299, 205 310, 205 500)), POLYGON ((20 310, 20 500, 205 500, 205 310, 20 310)), POLYGON ((410 338, 410 130, 215 130, 215 299, 410 338)))";

    runVoronoi(wkt, expected, 100);
}

//Validity issue and DD calculation
template<>
template<>
void object::test<10>
()
{
    const char* wkt = "0104000080170000000101000080EC51B81E11A20741EC51B81E85A51C415C8FC2F528DC354001010000801F85EB5114A207415C8FC2F585A51C417B14AE47E1BA3540010100008085EB51B818A20741A8C64B3786A51C413E0AD7A3709D35400101000080000000001BA20741FED478E984A51C413E0AD7A3709D3540010100008085EB51B818A20741FED478E984A51C413E0AD7A3709D354001010000800AD7A37016A20741FED478E984A51C413E0AD7A3709D35400101000080000000001BA2074154E3A59B83A51C413E0AD7A3709D3540010100008085EB51B818A2074154E3A59B83A51C413E0AD7A3709D354001010000800AD7A37016A2074154E3A59B83A51C413E0AD7A3709D35400101000080000000001BA20741AAF1D24D82A51C413E0AD7A3709D3540010100008085EB51B818A20741AAF1D24D82A51C413E0AD7A3709D35400101000080F6285C8F12A20741EC51B81E88A51C414160E5D022DB354001010000802222222210A2074152B81EC586A51C414160E5D022DB354001010000804F1BE8B40DA2074152B81EC586A51C414160E5D022DB354001010000807B14AE470BA2074152B81EC586A51C414160E5D022DB354001010000802222222210A20741B81E856B85A51C414160E5D022DB354001010000804F1BE8B40DA20741B81E856B85A51C414160E5D022DB354001010000807B14AE470BA20741B81E856B85A51C414160E5D022DB35400101000080A70D74DA08A20741B81E856B85A51C414160E5D022DB35400101000080D4063A6D06A20741B81E856B85A51C414160E5D022DB354001010000807B14AE470BA207411F85EB1184A51C414160E5D022DB35400101000080A70D74DA08A207411F85EB1184A51C414160E5D022DB35400101000080D4063A6D06A207411F85EB1184A51C414160E5D022DB3540";

    const char* expected =
        "GEOMETRYCOLLECTION (POLYGON ((193602.7711332133 469345.898980198, 193604.4418848486 469348.6016666667, 193605.9466666667 469348.6016666667, 193605.9466666667 469347.7638144172, 193603.2325 469345.391, 193602.9475 469345.391, 193602.8169643859 469345.5051185583, 193602.7711332133 469345.898980198)), POLYGON ((193601.8569454336 469344.9208636514, 193601.865 469344.9666851849, 193602.2037556305 469345.52375, 193602.2585544897 469345.5401343054, 193602.4642389841 469345.148354316, 193602.4577210526 469345.065, 193602.0579897448 469344.3617689955, 193601.8569454336 469344.9208636514)), POLYGON ((193601.2583333333 469342.0043333333, 193601.2583333333 469345.18625, 193601.5616666667 469345.18625, 193601.8569454336 469344.9208636514, 193602.0579897448 469344.3617689955, 193602.0222507725 469343.9301164729, 193601.5161595486 469342.0043333333, 193601.2583333333 469342.0043333333)), POLYGON ((193600.8486366758 469348.6016666667, 193604.4418848486 469348.6016666667, 193602.7711332133 469345.898980198, 193602.3274661311 469345.7182269423, 193601.865 469346.1338755144, 193601.5616666667 469346.6791265432, 193600.8486366758 469348.6016666667)), POLYGON ((193599.3943641253 469348.6016666667, 193600.8486366758 469348.6016666667, 193601.5616666667 469346.6791265432, 193601.5616666667 469345.52375, 193601.2583333333 469345.52375, 193600.955 469345.7963755144, 193599.3943641253 469348.6016666667)), POLYGON ((193602.4577210526 469345.065, 193602.4642389841 469345.148354316, 193602.8169643859 469345.5051185583, 193602.9475 469345.391, 193602.9475 469345.065, 193602.4577210526 469345.065)), POLYGON ((193602.2585544897 469345.5401343054, 193602.3274661311 469345.7182269423, 193602.7711332133 469345.898980198, 193602.8169643859 469345.5051185583, 193602.4642389841 469345.148354316, 193602.2585544897 469345.5401343054)), POLYGON ((193602.0222507725 469343.9301164729, 193602.0579897448 469344.3617689955, 193602.4577210526 469345.065, 193602.9475 469345.065, 193602.9475 469344.739, 193602.0222507725 469343.9301164729)), POLYGON ((193601.865 469345.52375, 193601.865 469346.1338755144, 193602.3274661311 469345.7182269423, 193602.2585544897 469345.5401343054, 193602.2037556305 469345.52375, 193601.865 469345.52375)), POLYGON ((193601.5616666667 469345.18625, 193601.5616666667 469345.52375, 193601.865 469345.52375, 193601.865 469344.9666851849, 193601.8569454336 469344.9208636514, 193601.5616666667 469345.18625)), POLYGON ((193601.5161595486 469342.0043333333, 193602.0222507725 469343.9301164729, 193602.9475 469344.739, 193603.2325 469344.739, 193603.2325 469342.0043333333, 193601.5161595486 469342.0043333333)), POLYGON ((193598.2316666667 469345.18625, 193598.2316666667 469348.6016666667, 193599.3943641253 469348.6016666667, 193600.955 469345.7963755144, 193600.955 469345.18625, 193598.2316666667 469345.18625)), POLYGON ((193603.2325 469345.065, 193603.2325 469345.391, 193605.9466666667 469347.7638144172, 193605.9466666667 469345.065, 193603.2325 469345.065)), POLYGON ((193603.2325 469344.739, 193603.2325 469345.065, 193605.9466666667 469345.065, 193605.9466666667 469344.739, 193603.2325 469344.739)), POLYGON ((193603.2325 469342.0043333333, 193603.2325 469344.739, 193605.9466666667 469344.739, 193605.9466666667 469342.0043333333, 193603.2325 469342.0043333333)), POLYGON ((193602.9475 469345.065, 193602.9475 469345.391, 193603.2325 469345.391, 193603.2325 469345.065, 193602.9475 469345.065)), POLYGON ((193602.9475 469344.739, 193602.9475 469345.065, 193603.2325 469345.065, 193603.2325 469344.739, 193602.9475 469344.739)), POLYGON ((193601.5616666667 469345.52375, 193601.5616666667 469346.6791265432, 193601.865 469346.1338755144, 193601.865 469345.52375, 193601.5616666667 469345.52375)), POLYGON ((193601.2583333333 469345.18625, 193601.2583333333 469345.52375, 193601.5616666667 469345.52375, 193601.5616666667 469345.18625, 193601.2583333333 469345.18625)), POLYGON ((193600.955 469345.18625, 193600.955 469345.7963755144, 193601.2583333333 469345.52375, 193601.2583333333 469345.18625, 193600.955 469345.18625)), POLYGON ((193600.955 469342.0043333333, 193600.955 469345.18625, 193601.2583333333 469345.18625, 193601.2583333333 469342.0043333333, 193600.955 469342.0043333333)), POLYGON ((193598.2316666667 469342.0043333333, 193598.2316666667 469345.18625, 193600.955 469345.18625, 193600.955 469342.0043333333, 193598.2316666667 469342.0043333333)), POLYGON ((193601.865 469344.9666851849, 193601.865 469345.52375, 193602.2037556305 469345.52375, 193601.865 469344.9666851849)))";

    runVoronoi(wkt, expected, 0);
}

// Consistency in the return value for edges
template<>
template<>
void object::test<11>
()
{
    const char* wkt1 = "LINESTRING (10 10, 20 20)";
    const char* expected1 = "MULTILINESTRING ((0 30, 30 0))";
    const char* wkt2 = "LINESTRING (10 10, 20 20, 30 30)";
    const char* expected2 = "MULTILINESTRING ((0 50, 50 0), (-10 40, 40 -10))";

    runVoronoi(wkt1, expected1, 0, true);
    runVoronoi(wkt2, expected2, 0, true);
}

// Robustness
// https://trac.osgeo.org/geos/ticket/769
template<>
template<>
void object::test<12>
()
{
    const char* wkb = "01040000000700000001010000000f8b33e3d97742c038c453588d0423c001010000001171d6d1b45d42c06adc1693e78c22c001010000001c8b33e3d97742c062c453588d0423c00101000000afa5c71fda7742c04b93c61d8e0423c00101000000b0cddcb4b57942c026476887d7b122c00101000000e0678421dc7642c0f7736021e1fb22c00101000000e32fd565018d42c0c7ea1222167c22c0";
    const char* expected = "01070000000700000001030000000100000006000000B5EED3F94DBC42C07FEF17D2E3BE21C083C782E8F67042C07FEF17D2E3BE21C01B8E1E20C77342C0081A51527C3D22C024A6D3C6BC9042C0B4B89DBBCEE322C0B5EED3F94DBC42C0328D5BABDB4F23C0B5EED3F94DBC42C07FEF17D2E3BE21C001030000000100000008000000B5EED3F94DBC42C0938EC16DC0C123C0B5EED3F94DBC42C0328D5BABDB4F23C024A6D3C6BC9042C0B4B89DBBCEE322C058723E09808842C061F41FEFD8E022C089AA93ED8D7942C09ADC10D431FC22C07F54F7A7C37742C039F77D71FB0423C0257EDE4F4F5142C0938EC16DC0C123C0B5EED3F94DBC42C0938EC16DC0C123C0010300000001000000070000003FB2D73D682E42C07FEF17D2E3BE21C03FB2D73D682E42C0EF02E944CD9B23C0C1681026B63B42C0F535EA64496D23C00CFA9663F66742C08140ABD7CECC22C01B8E1E20C77342C0081A51527C3D22C083C782E8F67042C07FEF17D2E3BE21C03FB2D73D682E42C07FEF17D2E3BE21C00103000000010000000500000024A6D3C6BC9042C0B4B89DBBCEE322C01B8E1E20C77342C0081A51527C3D22C00CFA9663F66742C08140ABD7CECC22C058723E09808842C061F41FEFD8E022C024A6D3C6BC9042C0B4B89DBBCEE322C0010300000001000000060000000CFA9663F66742C08140ABD7CECC22C0C1681026B63B42C0F535EA64496D23C0DF39DD8C877942C0127BCE7D3DFC22C089AA93ED8D7942C09ADC10D431FC22C058723E09808842C061F41FEFD8E022C00CFA9663F66742C08140ABD7CECC22C0010300000001000000070000003FB2D73D682E42C0938EC16DC0C123C0257EDE4F4F5142C0938EC16DC0C123C07F54F7A7C37742C039F77D71FB0423C0DF39DD8C877942C0127BCE7D3DFC22C0C1681026B63B42C0F535EA64496D23C03FB2D73D682E42C0EF02E944CD9B23C03FB2D73D682E42C0938EC16DC0C123C0010300000001000000040000007F54F7A7C37742C039F77D71FB0423C089AA93ED8D7942C09ADC10D431FC22C0DF39DD8C877942C0127BCE7D3DFC22C07F54F7A7C37742C039F77D71FB0423C0";

    runVoronoi(wkb, expected, 0, false);
}


// Polygon order reflects component order in input
template<>
template<>
void object::test<13>
()
{
    const char *wkt1 = "MULTIPOINT ((280 300), (420 330), (380 230), (320 160))";
    const char *wkt2 = "LINESTRING (280 300, 380 230, 420 330, 320 160)";
    const char *wkt3 = "MULTILINESTRING ((320 160, 280 300), (380 230, 420 330))";

    // We check result component order independently of how it's specified in WKT,
    // so the three input arrangements can all use the same expected value.
    const char *expected =
            "GEOMETRYCOLLECTION (POLYGON ((110 175.71428571428572, 110 500, 310.35714285714283 500, 353.515625 298.59375, 306.875 231.96428571428572, 110 175.71428571428572)), POLYGON ((590 204, 590 -10, 589.1666666666666 -10, 306.875 231.96428571428572, 353.515625 298.59375, 590 204)), POLYGON ((110 -10, 110 175.71428571428572, 306.875 231.96428571428572, 589.1666666666666 -10, 110 -10)), POLYGON ((310.35714285714283 500, 590 500, 590 204, 353.515625 298.59375, 310.35714285714283 500)))";
    runVoronoi(wkt1, expected, 0, false, true);
    runVoronoi(wkt2, expected, 0, false, true);
    runVoronoi(wkt3, expected, 0, false, true);
}

// Error thrown when trying to get ordered diagram from input with repeated points
template<>
template<>
void object::test<14>
()
{
    const char* wkt = "MULTIPOINT ((0 0), (1 1), (1 1), (2 2))";

    try {
        runVoronoi(wkt, "", 0, false, true);
        fail();
    } catch (const geos::util::GEOSException & e) {
    }
}

// empty input
// https://github.com/libgeos/geos/issues/795
template<>
template<>
void object::test<15>
()
{
    const char* wkt = "POLYGON EMPTY";
    const char* expected = "GEOMETRYCOLLECTION EMPTY";

    runVoronoi(wkt, expected, 0, false, false);
}

} // namespace tut
