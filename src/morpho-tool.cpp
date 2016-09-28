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
#include <iostream>
#include <exception>
#include <fstream>
#include <string>
#include <vector>
#include <utility>

#include <boost/program_options.hpp>

#include <boost/geometry.hpp>

#include <hadoken/format/format.hpp>

#include <morpho/morpho_h5_v1.hpp>

#include "mesh_exporter.hpp"

using namespace std;
using namespace morpho;

namespace fmt = hadoken::format;
namespace po = boost::program_options;

std::string version(){
    return std::string( MORPHO_VERSION_MAJOR "." MORPHO_VERSION_MINOR );
}

po::parsed_options parse_args(int argc, char** argv,
                         po::variables_map & res,
                         std::string & help_msg
                             ){
    po::options_description general("Commands:\n"
                                    "\t\t\n"
                                    "  export gmsh [morphology-file] [geo-file]:\texport morphology file to .geo file format\n"
                                    "\n\n"
                                    "Options");
    general.add_options()
        ("help", "produce a help message")
        ("version", "output the version number")
        ("point-cloud", "gmsh: export to a point cloud")
        ("wireframe", "gmsh: export to a wired morphology (default)")        
        ("command", po::value<std::string>(), "command to execute")
        ("subargs", po::value<std::vector<std::string> >(), "Arguments for command");
        ;

    po::positional_options_description pos;
    pos.add("command", 1)
       .add("subargs", -1)
            ;

    auto opts = po::command_line_parser(argc, argv )
            .options(general)
            .positional(pos)
            .allow_unregistered()
            .run();
    po::store(opts, res);
    if(res.count("help")){
        fmt::scat(std::cout, general, "\n");
        exit(0);
    }
    help_msg = fmt::scat(general);
    return opts;
}



void export_morpho_to_mesh(const std::string & filename_morpho, const std::string & filename_geo,
                          po::variables_map & options){
    gmsh_exporter exporter(filename_morpho, filename_geo);

    if(options.count("point-cloud")){
        exporter.export_to_point_cloud();
 
    }else{
        exporter.export_to_wireframe();
    }
    fmt::scat(std::cout, "\nconvert ", filename_morpho, " to gmsh file format.... ", filename_geo, "\n\n");    
}



int main(int argc, char** argv){
    po::variables_map options;
    std::string help_string;
    try{
        auto parsed_options = parse_args(argc, argv, options, help_string);

        if(options.count("version")){
            fmt::scat(std::cout, "version: ", version(), "\n");
            exit(0);
        }

        if(options.count("command") && options.count("subargs") ){
            std::string command = options["command"].as<std::string>();
            std::vector<std::string> subargs = po::collect_unrecognized(parsed_options.options, po::include_positional);
            if(command == "export" 
                && subargs.size() == 4 
                && subargs[1] == "gmsh"){
                export_morpho_to_mesh(subargs[2], subargs[3], options);
                return 0;
            };
        }

        fmt::scat(std::cout, "\nWrong command usage, see --help for details\n\n");
    }catch(std::exception & e){
        fmt::scat(std::cerr,
                  argv[0], "\n",
                "Error ", e.what(), "\n");
        exit(-1);
    }
    return 0;
}
