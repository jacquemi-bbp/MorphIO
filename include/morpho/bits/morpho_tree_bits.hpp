#ifndef MORPHO_TREE_BITS_HPP
#define MORPHO_TREE_BITS_HPP

#include "../morpho_tree.hpp"

namespace morpho{

namespace {

inline void soma_gravity_center(const mat_points & raw_points,
                                                             point & center,
                                                             double & distance){
    namespace geo = hadoken::geometry::cartesian;
    distance =0;
    center = { 0, 0, 0 };

    for(std::size_t i = 0; i < raw_points.size1(); ++i){
        for(std::size_t j = 0; j < 3; ++j){
            center(j) += raw_points(i,j);
        }
    }


    // compute gravity center
    center /= raw_points.size1();

    // compute average distance from gravity center
    for(std::size_t i = 0; i < raw_points.size1(); ++i){
        const point p( raw_points(i,0),
                              raw_points(i,1),
                              raw_points(i,2) );

        distance += geo::distance(p, center);
    }

    distance /= raw_points.size1();
}


// return the tangeante of a 3 points linestring in point 2
inline vector get_tangente_axis(const point & p1, const point & p2, const point & p3){
    namespace geo = hadoken::geometry::cartesian;
    vector v1 = p2 - p1, v2 = p3 - p2;

    return v1 + v2;
}


}


linestring branch::get_linestring() const{
    namespace geo = hadoken::geometry::cartesian;
    linestring res;
    res.reserve(get_size()+1);

    // if not first point, add the parent last point
    if(_parent != nullptr){
        if(_parent->get_type() == branch_type::soma){
            auto & soma = static_cast<branch_soma&>(*_parent);
            auto sphere = soma.get_sphere();
            geo::append(res, sphere.get_center());

        }else{
            geo::append(res, _parent->get_point(_parent->get_size()-1));
        }
    }

    for(std::size_t i =0; i < get_size(); ++i){
        geo::append(res, get_point(i));
    }
    return res;


}




circle_pipe branch::get_circle_pipe() const{
    namespace geo = hadoken::geometry::cartesian;
    circle_pipe res;
    res.reserve(get_size());

    if(get_size() < 1)
        return res;

    // if not first point, add the parent last point
    if(_parent != nullptr){
        // if our parent is the soma, add a circle based on the soma sphere, oriented
        if(_parent->get_type() == branch_type::soma){
            auto & soma = static_cast<branch_soma&>(*_parent);
            auto sphere = soma.get_sphere();
            auto center = sphere.get_center();
            auto radius = sphere.get_radius();
            auto axis = center - get_point(0);
            res.emplace_back(geo::circle3d(center, radius, axis));

        }else{
            circle_pipe parent_circle_pipe = _parent->get_circle_pipe();
            if(parent_circle_pipe.size() < 1){
                throw std::runtime_error("Invalid parent circle pipe, requires at least parent to have circle pipe >= 1 circle element");
            }
            res.push_back(parent_circle_pipe.back());
        }
    }else{
        throw std::runtime_error("Unable to compute circle pipe without parent informations");
    }

    for(std::size_t i =0; i < get_size(); ++i){
        const auto  & prev_center = res.back().get_center();
        auto center = get_point(i);
        vector axis;

        // compute the axis based on the tangente if we are not the last point
        if(i < get_size() -1){
            axis = get_tangente_axis(prev_center, center, get_point(i+1));
        }else{
            axis = prev_center - center;
        }

        auto radius = _distances[i];

        if(prev_center.close_to(center)){
            namespace fmt = hadoken::format;
            fmt::scat(std::cerr, "WARNING: skip point, Duplicated point in morphology detected ", prev_center, " and ",
                                            center, " in branch ", get_id(), ",  on point id ", i, "\n");
            continue;
        }

        res.emplace_back(geo::circle3d(center, radius, axis));
    }
    return res;


}


sphere branch_soma::get_sphere() const{
    namespace fmt = hadoken::format;
    switch(get_size()){
        case 0:
            throw std::runtime_error(fmt::scat("invalid branch ", get_id(), " : null size "));
        case 1:
            return sphere(get_point(0), get_distances()[0]);
        default:{
            double radius;
            point center;
            soma_gravity_center(get_points(), center, radius);
            return sphere(center, radius);
        }
    }
}

} //morpho

#endif // MORPHO_TREE_BITS_HPP

