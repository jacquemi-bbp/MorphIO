/*
 * Copyright (C) 2015 Adrien Devresse <adrien.devresse@epfl.ch>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#include "mesh_exporter.hpp"

#include <algorithm>
#include <vector>
#include <cmath>


#include <hadoken/format/format.hpp>

#include <morpho/morpho_h5_v1.hpp>

using namespace std;
namespace fmt = hadoken::format;


namespace morpho{

const std::string gmsh_header =
"/***************************************************************\n"
" * gmsh file generated by morpho-tool\n"
"****************************************************************/\n\n";


std::size_t gmsh_abstract_file::add_point(const gmsh_point &point){
    gmsh_point new_point(point);
    new_point.id = _points.size()+1;
   //std::cout << "points " << geo::get_x(point.coords ) << " " << geo::get_y(point.coords ) << " " << geo::get_z(point.coords ) << std::endl;
    return _points.insert(new_point).first->id;
}

std::size_t gmsh_abstract_file::find_point(const gmsh_point &point){
    auto it = _points.find(point);
    if(it == _points.end()){
        throw std::out_of_range(fmt::scat("Impossible to find point ", geo::get_x(point.coords), " ",
                                          geo::get_y(point.coords), " ", geo::get_z(point.coords), " in list of morphology points"));
    }
    return it->id;
}

std::size_t gmsh_abstract_file::add_segment(const gmsh_segment & s){
    gmsh_segment segment(s);
    add_point(segment.point1);
    add_point(segment.point2);
    segment.id = create_id_line_element();

    return _segments.insert(segment).first->id;
}

std::size_t gmsh_abstract_file::add_circle(const gmsh_circle &c){
    gmsh_circle my_circle(c);
    add_point(my_circle.center);
    add_point(my_circle.point1);
    add_point(my_circle.point2);
    my_circle.id = create_id_line_element();
    return _circles.insert(my_circle).first->id;
}


std::size_t gmsh_abstract_file::add_line_loop(const gmsh_line_loop &l){
    gmsh_line_loop loop(l);
    loop.id = create_id_line_element();
    return _line_loop.insert(loop).first->id;
}

std::size_t gmsh_abstract_file::add_volume(const gmsh_volume &v){
    gmsh_volume volume(v);
    volume.id = _volumes.size() +1;
    return _volumes.insert(volume).first->id;
}


template<typename Value, typename Set>
std::vector<Value> sort_by_id(const Set & s){

    std::vector<Value> all_objects;
    all_objects.reserve(s.size());

    // reorder all points by id, for geo file lisibility
    std::copy(s.begin(), s.end(), std::back_inserter(all_objects));

    std::sort(all_objects.begin(), all_objects.end(), [](const Value & p1, const Value & p2){
        return (p1.id < p2.id);
    });

    return all_objects;
}


std::vector<gmsh_point> gmsh_abstract_file::get_all_points() const{
    return sort_by_id<gmsh_point>(_points);
}

std::vector<gmsh_segment> gmsh_abstract_file::get_all_segments() const{
    return sort_by_id<gmsh_segment>(_segments);
}

std::vector<gmsh_circle> gmsh_abstract_file::get_all_circles() const{
    return sort_by_id<gmsh_circle>(_circles);
}


std::vector<gmsh_line_loop> gmsh_abstract_file::get_all_line_loops() const{
    return sort_by_id<gmsh_line_loop>(_line_loop);
}


std::vector<gmsh_volume> gmsh_abstract_file::get_all_volumes() const{
    return sort_by_id<gmsh_volume>(_volumes);
}

// if point around 0 -> 0
double clean_coordinate(double val){
    if(hadoken::math::almost_equal(val, 0.0)){
        return 0;
    }
    return val;
}


void gmsh_abstract_file::add_bounding_box(){
    double minp[3] = {1e+6, 1e+6, 1e+6};
    double maxp[3] = {-1e+6, -1e+6, -1e+6};

    /// Get bounding box coordinates based on min and max point coordinates
    auto all_points = get_all_points();
    for(auto p = all_points.begin(); p != all_points.end(); ++p){
        double x = clean_coordinate(geo::get_x(p->coords));
        if (x < minp[0])
            minp[0] = x;
        if (x > maxp[0])
            maxp[0] = x;

        double y = clean_coordinate(geo::get_y(p->coords));
        if (y < minp[1])
            minp[1] = y;
        if (y > maxp[1])
            maxp[1] = y;

        double z = clean_coordinate(geo::get_z(p->coords));
        if (z < minp[2])
            minp[2] = z;
        if (z > maxp[2])
            maxp[2] = z;
    }

    /// Set the offset of a bounding box such that the neuron doesn't immediately touch the bounding face
    const double offset = 20.;
    for (int i = 0; i < 3; ++i){
       minp[i] -= offset;
       maxp[i] += offset;
    }

    double csize = 100.;
    std::vector<gmsh_point> pnts;
    gmsh_point pnt0(geo::point3d(minp[0], minp[1], minp[2]), csize);
    pnt0.setPhysical(true);
    add_point(pnt0);
    pnts.push_back(pnt0);
    gmsh_point pnt1(geo::point3d(maxp[0], minp[1], minp[2]), csize);
    pnt1.setPhysical(true);
    add_point(pnt1);
    pnts.push_back(pnt1);
    gmsh_point pnt2(geo::point3d(maxp[0], maxp[1], minp[2]), csize);
    pnt2.setPhysical(true);
    add_point(pnt2);
    pnts.push_back(pnt2);
    gmsh_point pnt3(geo::point3d(minp[0], maxp[1], minp[2]), csize);
    pnt3.setPhysical(true);
    add_point(pnt3);

    pnts.push_back(pnt3);
    gmsh_point pnt4(geo::point3d(minp[0], minp[1], maxp[2]), csize);
    pnt4.setPhysical(true);
    add_point(pnt4);
    pnts.push_back(pnt4);
    gmsh_point pnt5(geo::point3d(maxp[0], minp[1], maxp[2]), csize);
    pnt5.setPhysical(true);
    add_point(pnt5);
    pnts.push_back(pnt5);
    gmsh_point pnt6(geo::point3d(maxp[0], maxp[1], maxp[2]), csize);
    pnt6.setPhysical(true);
    add_point(pnt6);
    pnts.push_back(pnt6);
    gmsh_point pnt7(geo::point3d(minp[0], maxp[1], maxp[2]), csize);
    pnt7.setPhysical(true);
    add_point(pnt7);
    pnts.push_back(pnt7);

    /// Create segments (lines, edges)
    std::vector<int64_t> seg_ids;
    for (int i = 0; i < 4; ++i) {
        gmsh_segment seg(pnts[i], pnts[(i+1)%4]);
        seg.setPhysical(true);
        seg_ids.push_back(add_segment(seg));
    }
    for (int i = 0; i < 4; ++i) {
        gmsh_segment seg(pnts[i], pnts[i+4]);
        seg.setPhysical(true);
        seg_ids.push_back(add_segment(seg));
    }
    for (int i = 0; i < 4; ++i) {
        gmsh_segment seg(pnts[i+4], pnts[(i+1)%4+4]);
        seg.setPhysical(true);
        seg_ids.push_back(add_segment(seg));
    }

    /// Create line loops
    std::vector<size_t> lloop_ids;
///from seg_ids
    gmsh_line_loop lloop0({seg_ids[0], seg_ids[1], seg_ids[2], seg_ids[3]});
    lloop0.setPhysical(true);
    lloop0.setRuled(true);
    lloop_ids.push_back(add_line_loop(lloop0));
    gmsh_line_loop lloop1({seg_ids[3], seg_ids[4], -seg_ids[11], -seg_ids[7]});
    lloop1.setPhysical(true);
    lloop1.setRuled(true);
    lloop_ids.push_back(add_line_loop(lloop1));
    gmsh_line_loop lloop2({-seg_ids[0], seg_ids[4], seg_ids[8], -seg_ids[5]});
    lloop2.setPhysical(true);
    lloop2.setRuled(true);
    lloop_ids.push_back(add_line_loop(lloop2));
    gmsh_line_loop lloop3({-seg_ids[1], seg_ids[5], seg_ids[9], -seg_ids[6]});
    lloop3.setPhysical(true);
    lloop3.setRuled(true);
    lloop_ids.push_back(add_line_loop(lloop3));
    gmsh_line_loop lloop4({seg_ids[2], seg_ids[7], -seg_ids[10], -seg_ids[6]});
    lloop4.setPhysical(true);
    lloop4.setRuled(true);
    lloop_ids.push_back(add_line_loop(lloop4));
    gmsh_line_loop lloop5({seg_ids[8], seg_ids[9], seg_ids[10], seg_ids[11]});
    lloop5.setPhysical(true);
    lloop5.setRuled(true);
    lloop_ids.push_back(add_line_loop(lloop5));

    /// Create region
    gmsh_volume vol(lloop_ids);
    vol.setPhysical(true);
    add_volume(vol);
}



template<typename CollectionElements>
inline void exportPhysicalsElements(const CollectionElements & elements, const std::string & name,
                                    const std::string & tag, std::ostream & out){
    bool first = true;
    for(auto p = elements.begin(); p != elements.end(); ++p){
        if(p->isPhysical){
            if(first){
                fmt::scat(out, "Physical ", name, "(\"", tag, "\") = { ", p->id);
                first = false;
            }else{
                fmt::scat(out, ", ", p->id);
            }
        }

    }

    if(first == false){
        fmt::scat(out, " };\n");
    }
}

void gmsh_abstract_file::export_points_to_stream(ostream &out){
    out << "\n";
    out << "// export morphology points \n";

    /// Setting the scaling factor
    fmt::scat(out, "h=1;\n");
    auto all_points = get_all_points();
    for(auto p = all_points.begin(); p != all_points.end(); ++p){
        fmt::scat(out,
                  "Point(", p->id,") = {", clean_coordinate(geo::get_x(p->coords)),", ", clean_coordinate(geo::get_y(p->coords)), ", ", clean_coordinate(geo::get_z(p->coords)), "};\n");
//                  "Point(", p->id,") = {", clean_coordinate(geo::get_x(p->coords)),", ", clean_coordinate(geo::get_y(p->coords)), ", ", clean_coordinate(geo::get_z(p->coords)), ", ", p->diameter, "*h};\n");
    }

    out << "\n";
    exportPhysicalsElements(all_points, "Point", "Points", out);
    out << "\n";
}


void gmsh_abstract_file::export_points_to_stream_dmg(ostream &out){
    auto all_points = get_all_points();
    for(auto p = all_points.begin(); p != all_points.end(); ++p){
        if(p->isPhysical){
            fmt::scat(out,
                  p->id," ", clean_coordinate(geo::get_x(p->coords))," ", clean_coordinate(geo::get_y(p->coords)), " ", clean_coordinate(geo::get_z(p->coords)), "\n");
        }
    }
}


template<typename Collection, typename AbstractFile >
void export_segments_single(const Collection & all_segments, AbstractFile & f, ostream & out){
    out << "// export morphology segments  \n";


    for(auto p = all_segments.begin(); p != all_segments.end(); ++p){
        fmt::scat(out,
                  "Line(", p->id,") = {" , f.find_point(p->point1),", ", f.find_point(p->point2),"};\n");
    }

    out << "\n";
    exportPhysicalsElements(all_segments, "Line", "Segments", out);
}


template<typename Collection, typename AbstractFile >
void export_segments_packed(const Collection & all_segments, AbstractFile & f, ostream & out){
    out << "// export morphology segments packed \n";
    
    struct packed_line{
        packed_line(int my_id) : id(my_id), isPhysical(true){}
        
        std::size_t id;
        
        bool isPhysical;
    };

    std::vector<Collection> packed_segments;
    std::vector<packed_line> packed_lines;

    for(auto p = all_segments.begin(); p != all_segments.end(); ++p){

        if(packed_segments.size() > 0){
            Collection & back_group = packed_segments.back();   
            
            if(back_group.size() > 0 
                && back_group.back().point2 == p->point1
                && back_group.back().branch_id == p->branch_id){
                back_group.push_back(*p);
                continue;
            }
        }
        // create a new polyline
        std::size_t packed_line_id = packed_lines.size() +1;        
        Collection new_back;
        new_back.push_back(*p);
        new_back.back().id = packed_line_id;
        packed_segments.emplace_back(std::move(new_back));
        packed_lines.push_back(packed_line(packed_line_id));

    }
    
    for( const auto & elem_group : packed_segments){
        
        const auto & first_elem = elem_group.front();
        
        fmt::scat(out,
                  "Line(", first_elem.id,") = {" , f.find_point(first_elem.point1));
        
        for(auto p = elem_group.begin(); p  <  elem_group.end(); ++p){
            fmt::scat(out, ", ", f.find_point(p->point2));
        };
        
        fmt::scat(out, "};\n");
        
    }

    out << "\n";
    exportPhysicalsElements(packed_lines, "Line", "Segments", out);
}



void gmsh_abstract_file::export_segments_to_stream(ostream &out, bool packed){
    out << "\n";
    auto all_segments = get_all_segments();

    if(packed){
        export_segments_packed(all_segments, *this, out);
    }else{
        export_segments_single(all_segments, *this, out);
    }
    out << "\n";
}


void gmsh_abstract_file::export_segments_to_stream_dmg(ostream &out){
    auto all_segments = get_all_segments();
    for(auto p = all_segments.begin(); p != all_segments.end(); ++p){
        if(p->isPhysical){
            fmt::scat(out,
                  p->id," ", find_point(p->point1), " ", find_point(p->point2),"\n");
        }
    }
}


void gmsh_abstract_file::export_circle_to_stream(ostream &out){
    out << "\n";
    out << "// export morphology arc-circle \n";

    auto all_circles = get_all_circles();

    for(auto p = all_circles.begin(); p != all_circles.end(); ++p){
        fmt::scat(out,
                  "Circle(", p->id,") = {" , find_point(p->point1),", ", find_point(p->center), ", ", find_point(p->point2),"};\n");
    }

    out << "\n";
    exportPhysicalsElements(all_circles, "Line", "Circles", out);
    out << "\n";
}


void gmsh_abstract_file::export_circle_to_stream_dmg(ostream &out){

    auto all_circles = get_all_circles();

    for(auto p = all_circles.begin(); p != all_circles.end(); ++p){
        if(p->isPhysical){
            fmt::scat(out,
                  p->id, " " , find_point(p->point1), " ", find_point(p->point2),"\n");
        }
    }
}


void gmsh_abstract_file::export_line_loop_to_stream(ostream &out){
    out << "\n";
    out << "// export line_looop \n";

    auto all_loop = get_all_line_loops();

    for(auto p = all_loop.begin(); p != all_loop.end(); ++p){
      //  std::cout << " size line loop " << p->ids.size() << std::endl;
        fmt::scat(out, "Line Loop(", p->id,") = {");
        std::string delimiter = "";
        for(auto & id_lines : p->ids){
            fmt::scat(out, delimiter,id_lines);
            delimiter.assign(", ");
        }
        fmt::scat(out, "};\n");
        if(p->isRuled){
            fmt::scat(out,
                      "Ruled Surface(", p->id,") = {", p->id,"};\n");
        }
    }

    out << "\n";
    exportPhysicalsElements(all_loop, "Surface", "Surfaces", out);
    out << "\n";
}


void gmsh_abstract_file::export_line_loop_to_stream_dmg(ostream &out){

    auto all_loop = get_all_line_loops();
    for(auto p = all_loop.begin(); p != all_loop.end(); ++p){
        if(!p->isPhysical)
            continue;

        fmt::scat(out, p->id, " 1\n", " ", p->ids.size(), "\n");
        for(auto & id : p->ids) {
            fmt::scat(out, "  ", std::abs(id));
            if (id > 0)
                fmt::scat(out, " 1\n");
            else
                fmt::scat(out, " 0\n");
        }
    }
}


void gmsh_abstract_file::export_volume_to_stream(ostream &out){
    out << "\n";
    out << "// export volumes \n";

    auto all_volumes = get_all_volumes();

    // export 1 surface loop == 1 volume
    for(auto p = all_volumes.begin(); p != all_volumes.end(); ++p){
        fmt::scat(out, "Surface Loop(", p->id,") = {");
        std::string delimiter = "";
        for(auto & id: p->ids){
            fmt::scat(out, delimiter,id);
            delimiter.assign(", ");
        }
        fmt::scat(out, "};\n");
        fmt::scat(out,
                  "Volume(", p->id,") = {", p->id,"};\n");
        if(p->isPhysical){
            fmt::scat(out,
                  "Physical Volume(", p->id,") = {", p->id,"};\n");
        }
    }

    out << "\n\n";
}


void gmsh_abstract_file::export_volume_to_stream_dmg(ostream &out){

    auto all_volumes = get_all_volumes();
//int nreg = 0;
    for(auto p = all_volumes.begin(); p != all_volumes.end(); ++p){
        if(!p->isPhysical)
            continue;
//++nreg;
        fmt::scat(out, p->id, " 1\n", " ", p->ids.size(), "\n");
        for(auto & id: p->ids){
            fmt::scat(out, "  ", id);
            if (id > 0)
                fmt::scat(out, " 1\n");
            else
                fmt::scat(out, " 0\n");
        }
    }
//printf("Number of regions is %d\n", nreg);
}


// Line id need to be unique for all line elements
//
std::size_t gmsh_abstract_file::create_id_line_element(){
    return _segments.size() + _circles.size() + _line_loop.size() + 1;
}

gmsh_exporter::gmsh_exporter(const std::string & morphology_filename, const std::string & mesh_filename, exporter_flags my_flags) :
    geo_stream(mesh_filename),
    dmg_stream(),
    reader(morphology_filename),
    flags(my_flags)
{
    if (is_dmg_enabled()) {
        std::string dmg_filename(mesh_filename);
        dmg_filename.erase(dmg_filename.find_last_of('.'), std::string::npos);
        dmg_filename.append(".dmg");
        dmg_stream.open(dmg_filename);
    }

}

gmsh_exporter::gmsh_exporter(std::vector<morpho_tree> && trees, const std::string & mesh_filename, exporter_flags my_flags) :
    geo_stream(mesh_filename),
    dmg_stream(),
    reader("default.h5"),
    flags(my_flags),
    morphotrees(std::move(trees))
{
    if (is_dmg_enabled()) {
        std::string dmg_filename(mesh_filename);
        dmg_filename.erase(dmg_filename.find_last_of('.'), std::string::npos);
        dmg_filename.append(".dmg");
        dmg_stream.open(dmg_filename);
    }

}

bool gmsh_exporter::is_dmg_enabled() const{
    return flags & exporter_write_dmg;
}

bool gmsh_exporter::is_bbox_enabled() const{
    return flags & exporter_bounding_box;
}

bool gmsh_exporter::is_packed() const{
    return flags & exporter_packed;
}

void gmsh_exporter::export_to_point_cloud(){
    serialize_header();
    serialize_points_raw();
}


void gmsh_exporter::export_to_wireframe(){
    serialize_header();

    if (morphotrees.size()<1){
      fmt::scat(std::cout, "load morphology tree ", reader.get_filename(), "\n");
      morpho_tree tree = reader.create_morpho_tree();
      morphotrees.push_back(std::move(tree));
    }

    gmsh_abstract_file vfile;
    fmt::scat(std::cout, "convert morphology tree to gmsh set of wireframe geometries", "\n");
    for(unsigned int i=0;i<morphotrees.size();i++){
      construct_gmsh_vfile_lines(morphotrees[i], morphotrees[i].get_branch(0), vfile);
    }

    if (is_bbox_enabled()) {
       fmt::scat(std::cout, "Adding bounding box", "\n");
       vfile.add_bounding_box();
    }

    fmt::scat(std::cout, "export gmsh objects to output file", "\n");
    vfile.export_points_to_stream(geo_stream);
    
    vfile.export_segments_to_stream(geo_stream, is_packed());

    if (is_bbox_enabled()) {
        vfile.export_line_loop_to_stream(geo_stream);
        vfile.export_volume_to_stream(geo_stream);

        /// Insert lines in the cube for geo file
        auto all_segments = vfile.get_all_segments();
        size_t seg_id_beg = all_segments[0].id;
        size_t seg_id_end = all_segments[all_segments.size()-13].id;

        auto all_volumes = vfile.get_all_volumes();
        size_t vol_id_end = all_volumes[all_volumes.size()-1].id;

        fmt::scat(geo_stream, "For s In {", seg_id_beg, ":", seg_id_end, "}\n  Line{s} In Volume{", vol_id_end, "};\nEndFor");
    }

    if (is_dmg_enabled()) {
        fmt::scat(std::cout, "export gmsh geometry objects to dmg file format", "\n");

        std::size_t ndim[4] = {0, 0, 0, 0};
        auto all_points = vfile.get_all_points();
        for(auto p = all_points.begin(); p != all_points.end(); ++p)
           if(p->isPhysical)
               ndim[0]++;

        auto all_segments = vfile.get_all_segments();
        for(auto p = all_segments.begin(); p != all_segments.end(); ++p)
           if(p->isPhysical)
               ndim[1]++;

        if (is_bbox_enabled()){
            ndim[2] = 6;
            ndim[3] = 1;
        }

        /// Write the header of dmg information
        fmt::scat(dmg_stream, ndim[3], " ", ndim[2], " ", ndim[1], " ", ndim[0], "\n0 0 0\n0 0 0\n");

        vfile.export_points_to_stream_dmg(dmg_stream);
        vfile.export_segments_to_stream_dmg(dmg_stream);

        if (is_bbox_enabled()) {
            vfile.export_line_loop_to_stream_dmg(dmg_stream);
            vfile.export_volume_to_stream_dmg(dmg_stream);
        }
    }
}


void gmsh_exporter::export_to_3d_object(){
    serialize_header();

    fmt::scat(std::cout, "load morphology tree ", reader.get_filename(), "\n");
    morpho_tree tree = reader.create_morpho_tree();


    gmsh_abstract_file vfile;
    fmt::scat(std::cout, "convert morphology tree to gmsh set of 3D geometries", "\n");
    construct_gmsh_3d_object(tree, tree.get_branch(0), vfile);

    fmt::scat(std::cout, "export gmsh objects to output file", "\n");


    vfile.export_points_to_stream(geo_stream);
    vfile.export_segments_to_stream(geo_stream);
    vfile.export_circle_to_stream(geo_stream);
    vfile.export_line_loop_to_stream(geo_stream);
    vfile.export_volume_to_stream(geo_stream);

    if (is_dmg_enabled()) {
        fmt::scat(std::cout, "export gmsh geometry objects to dmg file format", "\n");
        std::size_t ndim[4] = {0, 0, 0, 0};
        auto all_points = vfile.get_all_points();
        for(auto p = all_points.begin(); p != all_points.end(); ++p)
           if(p->isPhysical)
               ndim[0]++;

        auto all_segments = vfile.get_all_segments();
        for(auto p = all_segments.begin(); p != all_segments.end(); ++p)
           if(p->isPhysical)
               ndim[1]++;

        auto all_circles = vfile.get_all_circles();
        for(auto p = all_circles.begin(); p != all_circles.end(); ++p) {
            if(p->isPhysical)
                ndim[1]++;
        }

        auto all_line_loops = vfile.get_all_line_loops();
        for(auto p = all_line_loops.begin(); p != all_line_loops.end(); ++p)
           if(p->isPhysical)
               ndim[2]++;

        auto all_volumes = vfile.get_all_volumes();
        for(auto p = all_volumes.begin(); p != all_volumes.end(); ++p)
           if(p->isPhysical)
               ndim[3]++;

        /// Write the header of dmg information
        fmt::scat(dmg_stream, ndim[3], " ", ndim[2], " ", ndim[1], " ", ndim[0], "\n0 0 0\n0 0 0\n");

        vfile.export_points_to_stream_dmg(dmg_stream);
        vfile.export_segments_to_stream_dmg(dmg_stream);
        vfile.export_circle_to_stream_dmg(dmg_stream);
        vfile.export_line_loop_to_stream_dmg(dmg_stream);
        vfile.export_volume_to_stream_dmg(dmg_stream);
    }
}


void gmsh_exporter::serialize_header(){

    fmt::scat(geo_stream,
              gmsh_header,
              "// converted to GEO format from ", reader.get_filename(), "\n");

}


void gmsh_exporter::construct_gmsh_vfile_raw(gmsh_abstract_file & vfile){

    auto points = reader.get_points_raw();

    assert(points.size2() > 3);

    for(std::size_t row =0; row < points.size1(); ++row){
        gmsh_point point(geo::point3d( points(row, 0), points(row, 1), points(row, 2)), points(row, 3));
        vfile.add_point(point);
    }

}

void gmsh_exporter::construct_gmsh_vfile_lines(morpho_tree & tree, branch & current_branch, gmsh_abstract_file & vfile){

    const auto linestring = current_branch.get_linestring();
    if(linestring.size() > 1 && !(current_branch.get_type() == branch_type::soma && (flags & exporter_single_soma))){
        for(std::size_t i =0; i < (linestring.size()-1); ++i){
            auto dist = geo::distance(linestring[i], linestring[i+1]);
            gmsh_point p1(linestring[i], dist);
            p1.setPhysical(true);

            if (i < linestring.size()-2)
                dist = geo::distance(linestring[i+1], linestring[i+2]);
            gmsh_point p2(linestring[i+1], dist);
            p2.setPhysical(true);

            gmsh_segment segment(p1, p2);
            segment.setPhysical(true);
            segment.setBranchId(current_branch.get_id());
            vfile.add_segment(segment);
        }
    }

    const auto & childrens = current_branch.get_childrens();
    for( auto & c : childrens){
        branch & child_branch = tree.get_branch(c);
        construct_gmsh_vfile_lines(tree, child_branch, vfile);
    }
}



void create_gmsh_sphere(gmsh_abstract_file & vfile, const geo::sphere3d & sphere){
    // create 6 points of the sphere
    std::array<gmsh_point, 2> xpoints, ypoints, zpoints;
    xpoints[0] =  gmsh_point(sphere.get_center() - geo::point3d(sphere.get_radius(), 0 ,0 ));
    xpoints[1]  = gmsh_point(sphere.get_center() + geo::point3d(sphere.get_radius(), 0 ,0 ));
    ypoints[0]  = gmsh_point(sphere.get_center() - geo::point3d(0, sphere.get_radius() ,0 ));
    ypoints[1]  = gmsh_point(sphere.get_center() + geo::point3d(0, sphere.get_radius() ,0 ));
    zpoints[0] =  gmsh_point(sphere.get_center() - geo::point3d(0, 0, sphere.get_radius() ));
    zpoints[1]  = gmsh_point(sphere.get_center() + geo::point3d(0, 0, sphere.get_radius() ));

    // We need to cut the sphere in 12 arc circle to modelize it
    // and connect the arc together with line loops
    std::vector<std::size_t> line_loops;
    for(auto & x : xpoints){
        x.setPhysical(true);
        for(auto & y : ypoints){
            y.setPhysical(true);
            gmsh_point center(sphere.get_center());

            gmsh_circle xy_circle (center, x, y);
            xy_circle.setPhysical(true);
            std::size_t xy_circle_id= vfile.add_circle(xy_circle);

            for(auto & z : zpoints){
                z.setPhysical(true);
                gmsh_point center(sphere.get_center());

                gmsh_circle xz_circle(center, x, z);
                xz_circle.setPhysical(true);
                std::size_t xz_circle_id = vfile.add_circle(xz_circle);

                gmsh_circle yz_circle(center, y, z);
                yz_circle.setPhysical(true);
                std::size_t yz_circle_id = vfile.add_circle(yz_circle);

                gmsh_line_loop line_loop({ int64_t(xy_circle_id), int64_t(yz_circle_id), int64_t(-1* xz_circle_id) });
                line_loop.setPhysical(true);
                line_loop.setRuled(true);
                line_loops.push_back(vfile.add_line_loop(line_loop));
            }
        }

    }

    vfile.add_volume(gmsh_volume(line_loops));
}


void check_points_on_circle(double radius, gmsh_point & center, std::array<gmsh_point, 4> & points){

    for(auto & p: points){
        auto new_radius = geo::distance(center.coords, p.coords);
        bool is_on_circle = hadoken::math::close_to_abs(radius, new_radius, 0.0001);
        if(is_on_circle == false){
            throw std::out_of_range(fmt::scat("Invalid circle generation point ", p.coords, " is not on circle of center ", center.coords,
                                              " radius ", radius, " != ", new_radius));
        }
    }
}

//
// look for a suitable axis vector for cross product
// if dot_product == 1, the vector are colinear and the cross product will be null
// consequently, looking for the minimum value of the dot product
geo::vector3d get_unit_vec(const geo::vector3d & p1){
    const std::array<geo::vector3d, 2> unit_vec = {{ geo::vector3d({ 1.0, 0, 0}), geo::vector3d({ 0, 1.0, 0 }) }};

    double prev_dot = std::numeric_limits<double>::max();
    geo::vector3d res;
    for(auto && u :  unit_vec){
        double dot_res = geo::dot_product(p1, u);
        if(dot_res < prev_dot){
            prev_dot = dot_res;
            res = u;
        }
    }

    return res;
}

void create_gmsh_circle(gmsh_abstract_file & vfile, const geo::circle3d & p1, gmsh_point & center, std::array<gmsh_point, 4> & points,
                        std::array<std::size_t, 4> & arc_ids){


    auto center_coords = p1.get_center();
    center = gmsh_point(center_coords);
    vfile.add_point(center);

    auto radius = p1.get_radius();
    auto axis = geo::normalize(p1.get_axis());
    auto unit_vec = get_unit_vec(axis);

    auto normal_vec = geo::cross_product(unit_vec, axis);
    normal_vec = geo::normalize(normal_vec);

    auto orig_vec = geo::cross_product(normal_vec, axis);
    orig_vec = geo::normalize(orig_vec);

    // create the 4 orthogonal points on the circle
    std::array<geo::point3d, 4> norm_vectors;
    norm_vectors[0] = normal_vec * radius;
    norm_vectors[1] = orig_vec * radius;
    norm_vectors[2] = normal_vec * radius * -1;
    norm_vectors[3] = orig_vec * radius * -1;


    for(std::size_t i= 0; i < norm_vectors.size(); ++i){
            points[i] = gmsh_point( center_coords + norm_vectors[i]);
            points[i].setPhysical(true);
            vfile.add_point(points[i]);
    };

    check_points_on_circle(radius, center, points);

    for(std::size_t i =0; i < points.size(); ++i){
        std::size_t next_id = (i < points.size() -1) ? (i+1) : 0;

        gmsh_circle arc(center, points[i], points[next_id]);
       // arc.setPhysical(true);
        arc_ids[i] = vfile.add_circle(arc);
    }
}

// create a gmsh_disk
// take a circle coordinate, add all necessary points to the vfile
// and return the ids of the surface of the disk
std::vector<std::size_t> create_gmsh_disk(gmsh_abstract_file & vfile, const geo::circle3d & p1,
                                          std::array<gmsh_point, 4> & circle1_points,
                                          std::array<std::size_t, 4> & arc1_ids, bool surface){
    std::vector<std::size_t> surfaces;
    surfaces.reserve(4);

   // create the circle with all exterior arcs
   gmsh_point center;
   create_gmsh_circle(vfile, p1, center, circle1_points, arc1_ids);

   // add the connecting lines circle -> center
   std::vector<std::size_t> radius_segment_ids;
   radius_segment_ids.reserve(4);

   for(auto & point : circle1_points){
        gmsh_segment radius_segment(point, center);
        radius_segment.setPhysical(true);
        const std::size_t id_segment = vfile.add_segment(radius_segment);
        radius_segment_ids.emplace_back(id_segment);
   }


    if(surface){
        for(std::size_t i =0; i < radius_segment_ids.size(); ++i){
            std::size_t next_id = (i < radius_segment_ids.size()-1) ? i+1 : 0;
            std::vector<std::int64_t> ids;
            ids.push_back(arc1_ids[i]);
            ids.push_back(radius_segment_ids[next_id]);
            ids.push_back(-radius_segment_ids[i]);

            gmsh_line_loop part_disk(ids);
            part_disk.setPhysical(true);
            part_disk.setRuled(true);
            const std::size_t part_disk_id = vfile.add_line_loop(part_disk);
            surfaces.push_back(part_disk_id);

        }
    }
    return surfaces;
}

std::vector<std::size_t> create_gmsh_pipe_surfaces(gmsh_abstract_file & vfile,
                                                   std::array<gmsh_point, 4> & disk1_points,
                                                   std::array<std::size_t, 4> & arc1_ids,
                                                   std::array<gmsh_point, 4> & disk2_points,
                                                   std::array<std::size_t, 4> & arc2_ids){
    std::vector<std::size_t> res;
    std::vector<std::int64_t> ids;
    ids.reserve(4);
    res.reserve(4);

    assert(disk1_points.size() == disk2_points.size());
    assert(arc1_ids.size() == arc2_ids.size());
    assert(disk1_points.size() == arc2_ids.size());

    for(std::size_t i =0; i < disk1_points.size(); ++i){
        std::size_t id = i;
        std::size_t next_id = (i < (disk1_points.size() -1)) ? i+1 : 0;
        ids.clear();

        std::size_t line_id1, line_id2;
        {
            gmsh_segment seg1(disk1_points[id], disk2_points[id]);
            seg1.setPhysical(true);
            line_id1 = vfile.add_segment(seg1);
        }

        {
            gmsh_segment seg2(disk1_points[next_id], disk2_points[next_id]);
            seg2.setPhysical(true);
            line_id2 = vfile.add_segment(seg2);
        }


        ids.emplace_back(arc1_ids[i]);
        ids.emplace_back(line_id2);
        ids.emplace_back(- arc2_ids[i]);
        ids.emplace_back( - line_id1 );

        gmsh_line_loop pipe_surface(ids);
        pipe_surface.setPhysical(true);
        pipe_surface.setRuled(true);
        res.emplace_back(vfile.add_line_loop(pipe_surface));
    }
    return res;
}


void create_gmsh_truncated_pipe(gmsh_abstract_file & vfile, const std::vector<geo::circle3d> & circles ){
    std::vector<std::size_t> volume_ids;


    for(std::size_t id =0; id < circles.size()-1; ++id){
        const std::size_t id_next = id +1;

        // create 6 points of the sphere
        std::array<gmsh_point, 4> circle1_points, circle2_points;
        std::array<std::size_t, 4> arc1_ids, arc2_ids;

       // first circle
        {
           const bool is_surface = (id == 0);
           std::vector<std::size_t> disk_surface_ids = create_gmsh_disk(vfile, circles[id],
                                                                        circle1_points, arc1_ids, is_surface);
           std::copy(disk_surface_ids.begin(), disk_surface_ids.end(), std::back_inserter(volume_ids));
        }

       // second circle
       {
            const bool is_surface = (id_next == circles.size()-1);
            std::vector<std::size_t> disk_surface_ids = create_gmsh_disk(vfile, circles[id_next],
                                                                         circle2_points, arc2_ids, is_surface);
            std::copy(disk_surface_ids.begin(), disk_surface_ids.end(), std::back_inserter(volume_ids));
        }
        // create connecting line loops


        auto pipe_surfaces_ids = create_gmsh_pipe_surfaces(vfile, circle1_points, arc1_ids, circle2_points, arc2_ids);
        volume_ids.insert(volume_ids.end(), pipe_surfaces_ids.begin(), pipe_surfaces_ids.end());
    }

    // create final volume
    gmsh_volume truncated_pipe(volume_ids);
    truncated_pipe.setPhysical(true);
    vfile.add_volume(truncated_pipe);
}



void gmsh_exporter::construct_gmsh_3d_object(morpho_tree & tree, branch & current_branch, gmsh_abstract_file & vfile){

    if(current_branch.get_type() == branch_type::soma){
        sphere soma_sphere = static_cast<branch_soma&>(current_branch).get_sphere();
        create_gmsh_sphere(vfile, soma_sphere);
    }else{

        auto & distance = current_branch.get_distances();
        std::size_t last_elem = distance.size()-1;

        create_gmsh_sphere(vfile, geo::sphere3d(current_branch.get_point(last_elem), distance(last_elem)));
    }

   const auto & childrens = current_branch.get_childrens();
    for( auto & c : childrens){
        branch & child_branch = tree.get_branch(c);
        auto pipe = child_branch.get_circle_pipe();

 /*       decltype(pipe) copy;
        std::copy_n(pipe.begin(), 4, std::back_inserter(copy));*/
        create_gmsh_truncated_pipe(vfile, pipe);
        construct_gmsh_3d_object(tree, child_branch, vfile);
    }
}



void gmsh_exporter::serialize_points_raw(){
    gmsh_abstract_file vfile;
    construct_gmsh_vfile_raw(vfile);
    vfile.export_points_to_stream(geo_stream);

    if (is_dmg_enabled())
        vfile.export_points_to_stream_dmg(dmg_stream);
}



} // morpho
