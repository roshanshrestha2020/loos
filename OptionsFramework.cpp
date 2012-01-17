/*
  This file is part of LOOS.

  LOOS (Lightweight Object-Oriented Structure library)
  Copyright (c) 2011, Tod D. Romo, Alan Grossfield
  Department of Biochemistry and Biophysics
  School of Medicine & Dentistry, University of Rochester

  This package (LOOS) is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation under version 3 of the License.

  This package is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/



#include <OptionsFramework.hpp>


namespace loos {
  namespace OptionsFramework {

    void BasicOptions::addGeneric(po::options_description& opts) {
      if (!full_help.empty())
        opts.add_options()("fullhelp", "More detailed help");

      opts.add_options()
        ("help,h", "Produce this message")
        ("verbosity,v", po::value<int>(&verbosity)->default_value(verbosity), "Verbosity of output (if available)");
    }

    std::string BasicOptions::print() const {
      std::ostringstream oss;
      oss << "verbosity=" << verbosity ;
      return(oss.str());
    }


    // Presumably, BasicOptions will be the first OptionsPackage in
    // the list of options.  This means we can catch the --fullhelp at
    // the check() stage.  If we try later (i.e. with
    // postConditions()), then another OptionsPackage may fail the
    // check (such as required args or a model name)
    bool BasicOptions::check(po::variables_map& map) {
      if (!full_help.empty()) {
        if (map.count("fullhelp")) {
          std::cout << full_help << std::endl;
          return(true);
        }
      }
      return(false);
    }


    void BasicOptions::setFullHelp(const std::string& s) {
      full_help = s;
    }

    // -------------------------------------------------------

    void OutputPrefix::addGeneric(po::options_description& opts) {
      opts.add_options()
        ("prefix,p", po::value<std::string>(&prefix)->default_value(prefix), label.c_str());
    }

    std::string OutputPrefix::print() const {
      std::ostringstream oss;
      oss << "prefix='" << prefix << "'";
      return(oss.str());
    }
      
    // -------------------------------------------------------

    void BasicSelection::addGeneric(po::options_description& opts) {
      opts.add_options()
        ("selection,s", po::value<std::string>(&selection)->default_value(selection), label.c_str());
    }

    std::string BasicSelection::print() const {
      std::ostringstream oss;
      oss << "selection='" << selection << "'";
      return(oss.str());
    }

    // -------------------------------------------------------

    void ModelWithCoords::addGeneric(po::options_description& opts) {
      std::string filetypes = "Model types:\n" + availableSystemFileTypes();

      opts.add_options()
        ("coordinates,c", po::value<std::string>(&coords_name)->default_value(coords_name), "File to use for coordinates")
        ("modeltype", po::value<std::string>(&model_type)->default_value(model_type), filetypes.c_str());
    }

    void ModelWithCoords::addHidden(po::options_description& opts) {
      opts.add_options()
        ("model", po::value<std::string>(&model_name), "Model Filename");
    }


    void ModelWithCoords::addPositional(po::positional_options_description& pos) {
      pos.add("model", 1);
    }


    bool ModelWithCoords::check(po::variables_map& map) {
      return(!map.count("model"));
    }

    bool ModelWithCoords::postConditions(po::variables_map& map) {
      if (map.count("modeltype")) {
        model_type = map["modeltype"].as<std::string>();
        model = loadStructureWithCoords(model_name, model_type, coords_name);
      } else
        model = loadStructureWithCoords(model_name, coords_name);

      return(true);
    }

    std::string ModelWithCoords::help() const { return("model"); }


    std::string ModelWithCoords::print() const {
      std::ostringstream oss;

      oss << boost::format("model='%s'") % model_name;
      if (!coords_name.empty())
        oss << boost::format(", coords='%s'") % coords_name;

      return(oss.str());
    }




    // -------------------------------------------------------


    void TwoModelsWithCoords::addGeneric(po::options_description& opts) {
      std::string optdesc1 = std::string("File to use for coordinates for") + desc1;
      std::string optdesc2 = std::string("File to use for coordinates for") + desc2;


      opts.add_options()
        ("coord1,c", po::value<std::string>(&coords1_name)->default_value(coords1_name), optdesc1.c_str())
        ("coord2,d", po::value<std::string>(&coords2_name)->default_value(coords2_name), optdesc2.c_str());
    }

    void TwoModelsWithCoords::addHidden(po::options_description& opts) {
      opts.add_options()
        ("model1", po::value<std::string>(&model1_name), desc1.c_str())
        ("model2", po::value<std::string>(&model2_name), desc2.c_str());
    }


    void TwoModelsWithCoords::addPositional(po::positional_options_description& pos) {
      pos.add("model1", 1);
      pos.add("model2", 1);
    }


    bool TwoModelsWithCoords::check(po::variables_map& map) {
      return(!(map.count("model1") && map.count("model2")));
    }

    bool TwoModelsWithCoords::postConditions(po::variables_map& map) {
      model1 = loadStructureWithCoords(model1_name, coords1_name);
      model2 = loadStructureWithCoords(model2_name, coords2_name);
      return(true);
    }

    std::string TwoModelsWithCoords::help() const {
      std::string msg = desc1 + " " + desc2;
      return(msg);
    }


    std::string TwoModelsWithCoords::print() const {
      std::ostringstream oss;

      oss << boost::format("model1='%s'") % model1_name;
      if (!coords1_name.empty())
        oss << boost::format(", coords1='%s'") % coords1_name;

      oss << boost::format("model2='%s'") % model2_name;
      if (!coords2_name.empty())
        oss << boost::format(", coords2='%s'") % coords2_name;

      return(oss.str());
    }




    // -------------------------------------------------------



    void BasicTrajectory::addGeneric(po::options_description& opts) {
      std::string modeltypes = "Model types:\n" + availableSystemFileTypes();
      std::string trajtypes = "Trajectory types:\n" + availableTrajectoryFileTypes();

      opts.add_options()
        ("skip,k", po::value<unsigned int>(&skip)->default_value(skip), "Number of frames to skip")
        ("modeltype", po::value<std::string>(&model_type)->default_value(model_type), modeltypes.c_str())
        ("trajtype", po::value<std::string>(&traj_type)->default_value(traj_type), trajtypes.c_str());
    };

    void BasicTrajectory::addHidden(po::options_description& opts) {
      opts.add_options()
        ("model", po::value<std::string>(&model_name), "Model filename")
        ("traj", po::value<std::string>(&traj_name), "Trajectory filename");
    }

    void BasicTrajectory::addPositional(po::positional_options_description& pos) {
      pos.add("model", 1);
      pos.add("traj", 1);
    }

    bool BasicTrajectory::check(po::variables_map& map) {
      return(! (map.count("model") && map.count("traj")));
    }

    bool BasicTrajectory::postConditions(po::variables_map& map) {
      if (model_type.empty())
        model = createSystem(model_name);
      else
        model = createSystem(model_name, model_type);

      if (traj_type.empty())
        trajectory = createTrajectory(traj_name, model);
      else
        trajectory = createTrajectory(traj_name, traj_type, model);

      if (skip > 0)
        trajectory->readFrame(skip-1);

      return(true);
    }
    
    std::string BasicTrajectory::help() const { return("model trajectory"); }
    std::string BasicTrajectory::print() const {
      std::ostringstream oss;
      oss << boost::format("model='%s', traj='%s', skip=%d") % model_name % traj_name % skip;
      return(oss.str());
    }

    
    // -------------------------------------------------------


    void TrajectoryWithFrameIndices::addGeneric(po::options_description& opts) {
      opts.add_options()
        ("skip,k", po::value<unsigned int>(&skip)->default_value(skip), "Number of frames to skip")
        ("stride,i", po::value<unsigned int>(&stride)->default_value(stride), "Take every ith frame")
        ("range,r", po::value<std::string>(&frame_index_spec), "Which frames to use (matlab style range, overrides stride and skip)");
    };

    void TrajectoryWithFrameIndices::addHidden(po::options_description& opts) {
      opts.add_options()
        ("model", po::value<std::string>(&model_name), "Model filename")
        ("traj", po::value<std::string>(&traj_name), "Trajectory filename");
    }

    void TrajectoryWithFrameIndices::addPositional(po::positional_options_description& pos) {
      pos.add("model", 1);
      pos.add("traj", 1);
    }

    bool TrajectoryWithFrameIndices::check(po::variables_map& map) {
      return(! (map.count("model") && map.count("traj")));
    }

    bool TrajectoryWithFrameIndices::postConditions(po::variables_map& map) {
      if (skip > 0 && !frame_index_spec.empty()) {
        std::cerr << "Error- you cannot specify both a skip and a frame range...I might get confused!\n";
        return(false);
      }

      model = createSystem(model_name);
      trajectory = createTrajectory(traj_name, model);
      
      return(true);
    }
    
    std::string TrajectoryWithFrameIndices::help() const { return("model trajectory"); }
    std::string TrajectoryWithFrameIndices::print() const {
      std::ostringstream oss;
      oss << boost::format("model='%s', traj='%s'") % model_name % traj_name;
      if (skip > 0)
        oss << ", skip=" << skip;
      else if (!frame_index_spec.empty())
        oss << ", range='" << frame_index_spec << "'";
      
      return(oss.str());
    }

    
    std::vector<uint> TrajectoryWithFrameIndices::frameList() const {
      return(assignTrajectoryFrames(trajectory, frame_index_spec, skip, stride));
    }



    // -------------------------------------------------------

    void RequiredArguments::addArgument(const std::string& name, const std::string& description) {
      StringPair arg(name, description);
      if (find(arguments.begin(), arguments.end(), arg) != arguments.end()) {
        std::ostringstream oss;
        oss << "Error- duplicate command line argument requested for '" << name << "'";
        throw(LOOSError(oss.str()));
      }
      
      arguments.push_back(arg);
    }
    
    void RequiredArguments::addVariableArguments(const std::string& name, const std::string& description) {
      if (vargs_set)
        throw(LOOSError("Multiple variable arguments requested"));
      
      variable_arguments = StringPair(name, description);
      vargs_set = true;
    }
    
    void RequiredArguments::addHidden(po::options_description& o) {
      for (std::vector<StringPair>::const_iterator i = arguments.begin(); i != arguments.end(); ++i)
        o.add_options()(i->first.c_str(), po::value<std::string>(), i->second.c_str());
      if (vargs_set)
        o.add_options()(variable_arguments.first.c_str(), po::value< std::vector<std::string> >(), variable_arguments.second.c_str());
    }
    
    void RequiredArguments::addPositional(po::positional_options_description& pos) {
      for (std::vector<StringPair>::const_iterator i = arguments.begin(); i != arguments.end(); ++i)
        pos.add(i->first.c_str(), 1);
      if (vargs_set)
        pos.add(variable_arguments.first.c_str(), -1);
    }
    
    
    bool RequiredArguments::check(po::variables_map& map) {
      for (std::vector<StringPair>::const_iterator i = arguments.begin(); i != arguments.end(); ++i)
        if (! map.count(i->first.c_str()) )
          return(true);
      
      if (vargs_set && !map.count(variable_arguments.first))
        return(true);
      
      return(false);
    }
    
    bool RequiredArguments::postConditions(po::variables_map& map) {
      held_map = map;
      return(true);
    }

    std::string RequiredArguments::value(const std::string& s) const {
      std::string result;
      if (held_map.count(s))
        result = held_map[s].as<std::string>();
      return(result);
    }

  
    std::vector<std::string> RequiredArguments::variableValues(const std::string& s) const {
      return(held_map[s].as< std::vector<std::string> >());
    }


    std::string RequiredArguments::help() const {
      std::string s;
      for (std::vector<StringPair>::const_iterator i = arguments.begin(); i != arguments.end(); ++i)
        s = s + (i == arguments.begin() ? "" : " ") + i->first;
      if (vargs_set)
        s = s + (s.empty() ? "" : " ") + variable_arguments.first + " [" + variable_arguments.first + " ...]";
      return(s);
    }

    std::string RequiredArguments::print() const {
      std::ostringstream oss;
      for (std::vector<StringPair>::const_iterator i = arguments.begin(); i != arguments.end(); ++i)
        oss << i->first << "='" << held_map[i->first].as<std::string>() << "',";
      
      if (vargs_set) {
        std::vector<std::string> v = variableValues(variable_arguments.first);
        oss << variable_arguments.first << "=(";
        oss << vectorAsStringWithCommas<std::string>(v);
        oss << ")";
      }

      return(oss.str());
    }


    // -------------------------------------------------------


    AggregateOptions& AggregateOptions::add(OptionsPackage* pack) {
      options.push_back(pack); return(*this);
    }


    void AggregateOptions::setupOptions() {
      for (vOpts::iterator i = options.begin(); i != options.end(); ++i)
        (*i)->addGeneric(generic);

      for (vOpts::iterator i = options.begin(); i != options.end(); ++i)
        (*i)->addHidden(hidden);

      command_line.add(generic).add(hidden);

      pos = po::positional_options_description();
      for (vOpts::iterator i = options.begin(); i != options.end(); ++i)
        (*i)->addPositional(pos);
    }

    void AggregateOptions::showHelp() {
      std::cout << "Usage- " << program_name << " [options] ";
      for (vOpts::iterator i = options.begin(); i != options.end(); ++i) {
        std::string help_text = (*i)->help();
        if (! help_text.empty())
          std::cout << (*i)->help() << " ";
      }
      std::cout << std::endl;
      std::cout << generic;
    }


    bool AggregateOptions::parse(int argc, char *argv[]) {
      if (program_name.empty())
        program_name = std::string(argv[0]);

      generic.add_options()
        ("config", po::value<std::string>(&config_name), "Options config file");

      setupOptions();
      bool show_help = false;

      try {
        po::store(po::command_line_parser(argc, argv).style(po::command_line_style::default_style ^ po::command_line_style::allow_guessing).
                  options(command_line).positional(pos).run(), vm);
        po::notify(vm);

        if (!config_name.empty()) {
          std::ifstream ifs(config_name.c_str());
          if (!ifs)
            throw(LOOSError("Cannot open options config file"));
          store(parse_config_file(ifs, command_line), vm);
          notify(vm);
        }

      }
      catch (std::exception& e) {
        std::cerr << "Error- " << e.what() << std::endl;
        show_help = true;
      }

      if (!show_help)
        show_help = vm.count("help");

      if (!show_help)
        for (vOpts::iterator i = options.begin(); i != options.end() && !show_help; ++i)
          show_help = (*i)->check(vm);

      if (show_help) {
        showHelp();
        return(false);
      }

      for (vOpts::iterator i = options.begin(); i != options.end(); ++i)
        if (!(*i)->postConditions(vm)) {
          showHelp();
          return(false);
        }

      return(true);
      
    }

    std::vector<std::string> AggregateOptions::print() const {
      std::vector<std::string> results;

      results.push_back(program_name);
    
      for (vOpts::const_iterator i = options.begin(); i != options.end(); ++i)
        results.push_back((*i)->print());

      return(results);
    }

  };
};

