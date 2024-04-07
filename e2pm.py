
from platformdirs import *
from pathlib import Path
from pprint import pprint
import os
import json
import argparse
import uuid 
from jinja2 import Environment, PackageLoader, FileSystemLoader, select_autoescape
import shutil

import scg

class TargetType():
    # Only contains .hpp/.h
    HeaderOnly = "header-only"

    # Only contains .hpp/.h, .lib
    StaticLibrary = "static-library"

    # Only contains .hpp/.h, .lib, .dll (.pdb)
    SharedLibrary = "shared-library" 

    # Only contains .dll (.pdb)
    RuntimeLibrary = "runtime-library"

    # Only contains .exe (.pdb)
    Application = "application"



class PackageTarget():

    def __init__(self):

        self.name : str = ""

        # The type of this library
        self.type : str = TargetType.SharedLibrary

        # True if this package was built with reflection (it has reflection/db.bin)
        self.reflection : bool = False 

        # Names of dependencies, in the format <package>.<target>
        self.dependencies : list[str] = []

        # A dict of defines for shipping
        self.defines : dict[str, str] = {}

        # A dict of defines for development
        self.defines_devel : dict[str, str] = {}
        self.defines_profile : dict[str, str] = {}

        # If not empty, these are the include paths that should be used when using this target
        # These paths are relative to the package folder
        self.include_paths : list[str] = []

        # If not empty, these are the libraries that should be used when using this target
        # Note: ONLY VALID if type is SharedLibrary or StaticLibrary
        self.libraries : list[str] = []
        self.libraries_devel : list[str] = []
        self.libraries_profile : list[str] = []

        # If not empty, these are the binaries that should be used when using this target 
        # Note: ONLY VALID if type is SharedLibrary, RuntimeLibrary or Application
        self.binaries : list[str] = []
        self.binaries_devel : list[str] = []
        self.binaries_profile : list[str] = []

    def parse_from_json(self, json_obj):
        # first reset to sane defaults
        self.name = "core"
        self.type = TargetType.SharedLibrary
        self.reflection = False
        self.dependencies.clear()
        self.defines.clear()
        self.defines_devel.clear()
        self.defines_profile.clear()
        self.include_paths.clear()
        self.libraries.clear()
        self.libraries_devel.clear()
        self.libraries_profile.clear()
        self.binaries.clear()
        self.binaries_devel.clear()
        self.binaries_profile.clear()

        if "name" in json_obj:
            self.name = json_obj["name"]

        if "type" in json_obj:
            self.type = json_obj["type"]

        if "reflection" in json_obj:
            self.reflection = json_obj["reflection"]
        
        if "dependencies" in json_obj:
            self.dependencies = json_obj["dependencies"]

        if "defines" in json_obj:
            self.defines = json_obj["defines"]

        if "defines-devel" in json_obj:
            self.defines_devel = json_obj["defines-devel"]

        if "defines-profile" in json_obj:
            self.defines_profile = json_obj["defines-profile"]

        if "include-paths" in json_obj:
            self.include_paths = json_obj["include-paths"]

        if "libraries" in json_obj:
            self.libraries = json_obj["libraries"]

        if "libraries-devel" in json_obj:
            self.libraries_devel = json_obj["libraries-devel"]

        if "libraries-profile" in json_obj:
            self.libraries_profile = json_obj["libraries-profile"]

        if "binaries" in json_obj:
            self.binaries = json_obj["binaries"]

        if "binaries-devel" in json_obj:
            self.binaries_devel = json_obj["binaries-devel"]

        if "binaries-profile" in json_obj:
            self.binaries_profile = json_obj["binaries-profile"]

    def headers_relevant(self):
        return self.type in [TargetType.SharedLibrary, TargetType.StaticLibrary, TargetType.HeaderOnly]
    
    def libraries_relevant(self):
        return self.type in [TargetType.SharedLibrary, TargetType.StaticLibrary]

    def binaries_relevant(self):
        return self.type in [TargetType.SharedLibrary, TargetType.RuntimeLibrary, TargetType.Application]

# Represents an .e2p package, "code-ready-to-use(tm)"
class Package():
    def __init__(self, path:str):
        # Full path to package directory
        self.path : str = path
        self.valid : bool = False
        self.targets : list[PackageTarget] = []

        if not self.get_file().exists():
            return
        
        with open(self.get_file(), "r") as file:
            json_obj = json.load(file)

            #if not hasattr(json_obj, "targets"):
            if "targets" not in json_obj:
                return
            
            for target_json in json_obj["targets"]:
                newTarget : PackageTarget = PackageTarget()
                newTarget.parse_from_json(target_json)
                self.targets.append(newTarget)

            self.valid = True
            
    def get_folder(self) -> Path:
        return Path(self.path)
    
    def get_file(self) -> Path:
        return self.get_folder() / "package.e2p"
    
    def get_target(self, tgt_name:str) -> PackageTarget:
        for tgt in self.targets:
            if tgt.name == tgt_name:
                return tgt
        return None


class Repository():
    def __init__(self, path:str):
        self.path : str = path
    
    def get_folder(self) -> Path:
        return Path(self.path)
    
    def has_package(self, package_name:str) -> bool:
        return (self.get_folder() / package_name / "package.e2p").exists()

    def get_package(self, package_name:str) -> Package:
        return Package((self.get_folder() / package_name).resolve())

    def get_package_names(self):
        for entry in self.get_folder().iterdir():
            if entry.is_dir():
                if (entry / "package.e2p").exists():
                    yield entry.name

class ObjectEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, set):
            return list(obj)
        if isinstance(obj, object):
            return obj.__dict__
        return json.JSONEncoder.default(self, obj)

class Config():
    def __init__(self):
        self.repositories : list[str] = [ ]

class RuntimeContext():
    def __init__(self):
        cfg_path : Path = user_config_path("e2pm", "PRKL Interactive") / "config.json"
        cfg = Config()

        print("loading config: {}".format(cfg_path))

        #print("config folder: {}".format(cfg_path.parent))

        if cfg_path.exists():
            with open(cfg_path, "r") as file:
                tmp = json.load(file)
                if "repositories" in tmp:
                    cfg.repositories = tmp["repositories"]
        else:
            print("No configuration, so no repositories. Writing empty configuration and exiting..")
            cfg_path.parent.mkdir(parents=True, exist_ok=True)
            empty_cfg = Config()
            with open(cfg_path, "x") as file:
                json.dump(empty_cfg, file, indent=4, cls=ObjectEncoder)
            exit()

        if not cfg.repositories:
            print("No repositories, exiting..")
            exit()

        self.repositories : list[Repository] = [Repository(x) for x in cfg.repositories]

    def get_package(self, name:str) -> Package:
        for repo in self.repositories:
            pkg = repo.get_package(name)
            if not pkg:
                continue 

            return pkg

        return None 

    def get_target(self, name:str) -> PackageTarget:
        name_parts = name.split(".", 1)

        if len(name_parts) != 2:
            return None
        
        pkg_name : str = name_parts[0]
        tgt_name : str = name_parts[1]

        for repo in self.repositories:
            pkg = repo.get_package(pkg_name)
            if not pkg:
                continue 
            
            tgt = pkg.get_target(tgt_name)
            if not tgt:
                continue
            
            return tgt

        return None 
    
    def gather_dependencies(self, tgt_name:str) -> list[str]:      
        workload : list[str] = [tgt_name]
        returner : list[str] = []

        skip_list:set[str] = set()

        while workload:
            next:str = workload.pop(0)

            if next in skip_list:
                continue

            skip_list.add(next)

            tgt = self.get_target(next)
            if not tgt:
                print("could not find stated dependency {}".format(next) )
                continue 

            returner.append(next)
            
            for dep in tgt.dependencies:
                workload.append(dep)

        return returner


class SourceTargetBlock():
    def __init__(self):
        # Dependencies, if public these dependencies will be inherited as well as build deps, if private they are only build deps
        self.dependencies : list[str] = []

        # Defines, if public they will only be inherited, if private they will only be defined when building
        self.defines : dict[str,str] = {}

        # Defines, if public they will only be inherited, if private they will only be defined when building
        self.defines_devel : dict[str,str] = {}

        self.defines_profile : dict[str,str] = {}

        # Package-relative include paths, if private they will only be used when building, if public they will also be packaged and referenced when used
        self.include_paths : list[str] = []

    def parse_from_json(self, json_obj):
        self.dependencies.clear()
        self.defines.clear()
        self.defines_devel.clear()
        self.defines_profile.clear()
        self.include_paths.clear()

        if "dependencies" in json_obj:
            self.dependencies = json_obj["dependencies"]

        if "defines" in json_obj:
            self.defines = json_obj["defines"]

        if "defines-devel" in json_obj:
            self.defines_devel = json_obj["defines-devel"]

        if "defines-profile" in json_obj:
            self.defines_profile = json_obj["defines-profile"]

        if "include-paths" in json_obj:
            self.include_paths = json_obj["include-paths"]

class SourceTarget():

    def __init__(self):

        self.name : str = ""

        # The type of this library
        self.type : str = TargetType.SharedLibrary

        # Package relative paths with source files (.cpp, .c)
        self.source_paths : list[str] = []
        self.reflection : bool = False

        self.api_define : str = ""

        self.public : SourceTargetBlock = SourceTargetBlock()
        self.private : SourceTargetBlock = SourceTargetBlock()

    def parse_from_json(self, json_obj):
        # first reset to sane defaults
        self.name = "core"
        self.type = TargetType.SharedLibrary
        self.source_paths.clear()

        self.public = SourceTargetBlock()
        self.private = SourceTargetBlock()

        self.api_define : str = ""

        if "api" in json_obj:
            self.api_define = json_obj["api"]

        if "reflection" in json_obj:
            self.reflection = str(json_obj["reflection"]).lower() in ["enable", "enabled", "true", "yes", "1", "on"]

        if "name" in json_obj:
            self.name = json_obj["name"]

        if "type" in json_obj:
            self.type = json_obj["type"]
        
        if "source-paths" in json_obj:
            self.source_paths = json_obj["source-paths"]

        if "public" in json_obj:
            self.public.parse_from_json(json_obj["public"])

        if "private" in json_obj:
            self.private.parse_from_json(json_obj["private"])

    def headers_relevant(self):
        return self.type in [TargetType.SharedLibrary, TargetType.StaticLibrary, TargetType.HeaderOnly]
    
    def libraries_relevant(self):
        return self.type in [TargetType.SharedLibrary, TargetType.StaticLibrary]

    def binaries_relevant(self):
        return self.type in [TargetType.SharedLibrary, TargetType.RuntimeLibrary, TargetType.Application]

# Represents an .e2s source, i.e. sourcecode for a package
class SourceProject():
    def __init__(self, path:str):
        self.path : str = path
        self.name = self.get_file().stem
        self.valid : bool = False
        self.targets : list[SourceTarget] = []

        if not self.get_file().exists():
            return
        
        with open(self.get_file(), "r") as file:
            json_obj = json.load(file)

            if "targets" not in json_obj:
                return
            
            for target_json in json_obj["targets"]:
                newTarget : SourceTarget = SourceTarget()
                newTarget.parse_from_json(target_json)
                self.targets.append(newTarget)

            self.valid = True

    def get_folder(self) -> Path:
        return self.get_file().parent
    
    def get_file(self) -> Path:
        return Path(self.path).resolve()

def make_guid():
    return "{" + str(uuid.uuid4()).upper() + "}"

class IntermediateProjectType():
    DynamicLibrary = "DynamicLibrary"
    Application = "Application"
    StaticLibrary = "StaticLibrary"

    def from_target_type(type:TargetType):
        if type == TargetType.Application:
            return IntermediateProjectType.Application
        if type == TargetType.SharedLibrary:
            return IntermediateProjectType.DynamicLibrary
        if type == TargetType.RuntimeLibrary:
            return IntermediateProjectType.DynamicLibrary
        if type == TargetType.StaticLibrary:
            return IntermediateProjectType.StaticLibrary
        
        return ""
        

# Now this is really fucky, but IntermediateProject is a Target, and IntermediateSolution is a Project

class IntermediateFilter():
    def __init__(self):
        self.path : str = ""
        self.guid : str = make_guid()

class IntermediateFilterSrc():
    def __init__(self):
        self.filter : IntermediateFilter = None 
        self.path : str = ""

class IntermediateProject():
    def __init__(self, ctx:RuntimeContext, source_project:SourceProject, source_target:SourceTarget):
        self.context : RuntimeContext = ctx
        self.source_project : SourceProject = source_project
        self.source_target : SourceTarget = source_target
        self.name : str = source_project.name + "." + source_target.name
        self.guid : str = make_guid()
        self.type : str = IntermediateProjectType.from_target_type(source_target.type)

        # extra "intermediate" paths, i.e. paths to reflection dirs where db.bin is
        # we use this to fill bases where we dont directly parse (speedup for clang parsing)
        self.extra_paths : list[str] = []
        
        # populate from intermediate solution
        self.internal_dependency_guids : list[str] = []

        # these are maps etc to be prepared later
        self.defines : dict[str,str] = {}
        self.defines_devel : dict[str,str] = {}
        self.defines_profile : dict[str,str] = {}
        self.include_dirs : set[str] = set()
        self.lib_dirs : set[str] = set()
        self.libs : set[str] = set()
        self.libs_devel : set[str] = set()
        self.libs_profile : set[str] = set()

        # These are prepared strings for project generation
        self.defines_string : str = ""
        self.defines_devel_string : str = ""
        self.defines_profile_string : str = ""
        self.include_dirs_string : str = ""
        self.lib_dirs_string : str = ""
        self.libs_string : str = ""
        self.libs_devel_string : str = ""
        self.libs_profile_string : str = ""
        self.include_files : list[str] = []
        self.source_files : list[str] = []

        self.binary_files : list[str] = []
        self.binary_files_devel : list[str] = []
        self.binary_files_profile : list[str] = []

        self.debug_binary : str = ""
        self.debug_binary_devel : str = ""
        self.debug_binary_profile : str = ""
        self.debug_dir : str = ""

        # For generating filters 
        self.filters : list[IntermediateFilter] = []
        self.filter_includes : list[IntermediateFilterSrc] = []
        self.filter_sources : list[IntermediateFilterSrc] = []

        # tags resovled deps so we dont accidentally resolve the same dep twice
        self.resolved_deps : set[str] = set()

        # Actually apply stuff from our source target
        for k, v in self.source_target.public.defines.items():
            self.defines[k] = v

        for k, v in self.source_target.public.defines_devel.items():
            self.defines_devel[k] = v

        for k, v in self.source_target.public.defines_profile.items():
            self.defines_profile[k] = v

        for inc_dir in self.source_target.public.include_paths:
            if inc_dir.startswith("!"):
                self.include_dirs.add( os.path.expandvars(inc_dir[1:]) )
            else:
                self.include_dirs.add( str( (self.source_project.get_folder() / inc_dir).resolve() ) )

        for k, v in self.source_target.private.defines.items():
            self.defines[k] = v

        for k, v in self.source_target.private.defines_devel.items():
            self.defines_devel[k] = v

        for k, v in self.source_target.private.defines_profile.items():
            self.defines_profile[k] = v

        for inc_dir in self.source_target.private.include_paths:
            if inc_dir.startswith("!"):
                self.include_dirs.add( os.path.expandvars(inc_dir[1:]) )
            else:
                self.include_dirs.add( str( (self.source_project.get_folder() / inc_dir).resolve() ) )

        # apply shit for reflection
        if self.source_target.reflection:
            self.include_dirs.add(  str( (self.source_project.get_folder() / self.name / "reflection/include").resolve() ) )
            self.include_dirs.add(  str( (self.source_project.get_folder() / self.name / "reflection/init").resolve() ) )


    def prepare(self):
        self.defines_string = ";".join(["{}={}".format(x, y) for x, y in self.defines.items()])
        self.defines_devel_string = ";".join(["{}={}".format(x, y) for x, y in self.defines_devel.items()])
        self.defines_profile_string = ";".join(["{}={}".format(x, y) for x, y in self.defines_profile.items()])
        self.include_dirs_string = ";".join(["\"{}\"".format(x) for x in self.include_dirs])
        self.lib_dirs_string = ";".join(["\"{}\"".format(x) for x in self.lib_dirs])
        self.libs_string = ";".join([x for x in self.libs])
        self.libs_devel_string = ";".join([x for x in self.libs_devel])
        self.libs_profile_string = ";".join([x for x in self.libs_profile])


        # populate filters 
        filter_map : dict[str, IntermediateFilter] = {}

        for inc_path in self.source_target.private.include_paths:
            full_path : Path = (self.source_project.get_folder() / inc_path).resolve() 
            for dir in full_path.glob("**/"):
                rel_path : Path = dir.relative_to(full_path)
                rel_path_str = str(rel_path)
                if rel_path_str not in filter_map:
                    new_filter = IntermediateFilter()
                    new_filter.path = rel_path_str 
                    filter_map[rel_path_str] = new_filter
                    self.filters.append(new_filter)

        for inc_path in self.source_target.public.include_paths:
            full_path : Path = (self.source_project.get_folder() / inc_path).resolve() 
            for dir in full_path.glob("**/"):
                rel_path : Path = dir.relative_to(full_path)
                rel_path_str = str(rel_path)
                if rel_path_str not in filter_map:
                    new_filter = IntermediateFilter()
                    new_filter.path = rel_path_str 
                    filter_map[rel_path_str] = new_filter
                    self.filters.append(new_filter)

        for inc_path in self.source_target.source_paths:
            full_path : Path = (self.source_project.get_folder() / inc_path).resolve() 
            for dir in full_path.glob("**/"):
                rel_path : Path = dir.relative_to(full_path)
                rel_path_str = str(rel_path)
                if rel_path_str not in filter_map:
                    new_filter = IntermediateFilter()
                    new_filter.path = rel_path_str 
                    filter_map[rel_path_str] = new_filter
                    self.filters.append(new_filter)

        if self.source_target.reflection:
            for cpp_file in (self.source_project.get_folder() / self.name / "reflection/src").resolve().glob("**/*.cpp"):
                #self.source_files.append(cpp_file)
                self.source_files.append(os.path.relpath(cpp_file, self.source_project.get_folder() / "vcx"))

        for src_path in self.source_target.source_paths:
            full_path : Path = (self.source_project.get_folder() / src_path).resolve()
            for c_file in full_path.glob("**/*.c"):
                #self.source_files.append(c_file)
                self.source_files.append(os.path.relpath(c_file, self.source_project.get_folder() / "vcx"))
                filter_path : Path = c_file.parent.relative_to(full_path)
                filter_path_str = str(filter_path)
                new_filter_src = IntermediateFilterSrc()
                new_filter_src.filter = filter_map[filter_path_str]
                new_filter_src.path = os.path.relpath(c_file, self.source_project.get_folder() / "vcx")
                self.filter_sources.append(new_filter_src)
            for cpp_file in full_path.glob("**/*.cpp"):
                #self.source_files.append(cpp_file)
                self.source_files.append(os.path.relpath(cpp_file, self.source_project.get_folder() / "vcx"))
                filter_path : Path = cpp_file.parent.relative_to(full_path)
                filter_path_str = str(filter_path)
                new_filter_src = IntermediateFilterSrc()
                new_filter_src.filter = filter_map[filter_path_str]
                
                new_filter_src.path = os.path.relpath(cpp_file, self.source_project.get_folder() / "vcx")
                self.filter_sources.append(new_filter_src)

        for inc_path in self.source_target.private.include_paths:
            full_path : Path = (self.source_project.get_folder() / inc_path).resolve() 
            for h_file in full_path.glob("**/*.h"):
                #self.include_files.append(h_file)
                self.include_files.append(os.path.relpath(h_file, self.source_project.get_folder() / "vcx"))
                filter_path : Path = h_file.parent.relative_to(full_path)
                filter_path_str = str(filter_path)
                new_filter_src = IntermediateFilterSrc()
                new_filter_src.filter = filter_map[filter_path_str]
                new_filter_src.path = os.path.relpath(h_file, self.source_project.get_folder() / "vcx")
                self.filter_includes.append(new_filter_src)
            for hpp_file in full_path.glob("**/*.hpp"):
                #self.include_files.append(hpp_file)
                self.include_files.append(os.path.relpath(hpp_file, self.source_project.get_folder() / "vcx"))
                filter_path : Path = hpp_file.parent.relative_to(full_path)
                filter_path_str = str(filter_path)
                new_filter_src = IntermediateFilterSrc()
                new_filter_src.filter = filter_map[filter_path_str]
                new_filter_src.path = os.path.relpath(hpp_file, self.source_project.get_folder() / "vcx")
                self.filter_includes.append(new_filter_src)

        for inc_path in self.source_target.public.include_paths:
            full_path : Path = (self.source_project.get_folder() / inc_path).resolve() 
            for h_file in full_path.glob("**/*.h"):
                #self.include_files.append(h_file)
                self.include_files.append(os.path.relpath(h_file, self.source_project.get_folder() / "vcx"))
                filter_path : Path = h_file.parent.relative_to(full_path)
                filter_path_str = str(filter_path)
                new_filter_src = IntermediateFilterSrc()
                new_filter_src.filter = filter_map[filter_path_str]
                new_filter_src.path = os.path.relpath(h_file, self.source_project.get_folder() / "vcx")
                self.filter_includes.append(new_filter_src)
            for hpp_file in full_path.glob("**/*.hpp"):
                #self.include_files.append(hpp_file)
                self.include_files.append(os.path.relpath(hpp_file, self.source_project.get_folder() / "vcx"))
                filter_path : Path = hpp_file.parent.relative_to(full_path)
                filter_path_str = str(filter_path)
                new_filter_src = IntermediateFilterSrc()
                new_filter_src.filter = filter_map[filter_path_str]
                new_filter_src.path = os.path.relpath(hpp_file, self.source_project.get_folder() / "vcx")
                self.filter_includes.append(new_filter_src)

        self.include_files = list(dict.fromkeys(self.include_files))
        self.source_files = list(dict.fromkeys(self.source_files))




class IntermediateSolution():

    def gather_dependencies(self, project:IntermediateProject):
        int_deps:list[str] = []
        ext_deps:list[str] = []

        initial_deps :list[str] =[]
        initial_deps.extend(project.source_target.private.dependencies)
        initial_deps.extend(project.source_target.public.dependencies)
        # remove duplicates while maintaining order
        initial_deps = list(dict.fromkeys(initial_deps))
        workload:list[str] = initial_deps
        done:set[str]=set()

        while workload:
            next:str = workload.pop(0)

            if next in done:
                continue

            done.add(next)

            if next in self.project_index:
                int_deps.append(next)
                
                workload.extend(self.project_index[next].source_target.private.dependencies)
                workload.extend(self.project_index[next].source_target.public.dependencies)

            else:
                ext_deps.extend(self.context.gather_dependencies(next))

        int_deps = list(dict.fromkeys(int_deps))
        ext_deps = list(dict.fromkeys(ext_deps))

        return (int_deps, ext_deps)

    def __init__(self, ctx:RuntimeContext, source_project:SourceProject):
        self.context : RuntimeContext = ctx
        self.source_project : SourceProject = source_project
        self.name : str = source_project.name
        self.guid : str = make_guid()
        self.intermediate_projects : list[IntermediateProject] = [IntermediateProject(self.context, self.source_project, x) for x in self.source_project.targets]
        self.project_index :dict[str, IntermediateProject] = {}
        for proj in self.intermediate_projects:
            self.project_index[proj.name] = proj

        for proj in self.intermediate_projects:


            base_dir = (proj.source_project.get_folder() / f"{proj.source_project.name}.{proj.source_target.name}" / "bin").resolve()
            deploy_dir = (proj.source_project.get_folder() / "deploy").resolve()
            proj.debug_dir = proj.source_project.get_folder()
            if proj.source_target.type == TargetType.Application:
                proj.binary_files.append( base_dir / f"{proj.source_project.name}.{proj.source_target.name}.exe" )
                proj.binary_files_devel.append( base_dir / f"{proj.source_project.name}.{proj.source_target.name}-dev.exe" )
                proj.binary_files_profile.append( base_dir / f"{proj.source_project.name}.{proj.source_target.name}-profiler.exe" )
                proj.debug_binary = str(deploy_dir / f"{proj.source_project.name}.{proj.source_target.name}.exe")
                proj.debug_binary_devel = str(deploy_dir / f"{proj.source_project.name}.{proj.source_target.name}-dev.exe")
                proj.debug_binary_profile = str(deploy_dir / f"{proj.source_project.name}.{proj.source_target.name}-profiler.exe")
            else:
                proj.binary_files.append( base_dir / f"{proj.source_project.name}.{proj.source_target.name}.dll" )
                proj.binary_files_devel.append( base_dir / f"{proj.source_project.name}.{proj.source_target.name}-dev.dll" )
                proj.binary_files_profile.append( base_dir / f"{proj.source_project.name}.{proj.source_target.name}-profiler.dll" )


            int_deps, ext_deps = self.gather_dependencies(proj)

            # resolve internal deps 
            for dep in int_deps:
                other = self.project_index[dep]
                proj.internal_dependency_guids.append(other.guid)
                
                for k, v in other.source_target.public.defines.items():
                    proj.defines[k] = v

                for k, v in other.source_target.public.defines_devel.items():
                    proj.defines_devel[k] = v

                for k, v in other.source_target.public.defines_profile.items():
                    proj.defines_profile[k] = v

                for inc_dir in other.source_target.public.include_paths:
                    proj.include_dirs.add( str( (other.source_project.get_folder() / inc_dir).resolve() ) )

                if other.source_target.reflection:
                    proj.include_dirs.add( str( (other.source_project.get_folder() / f"{other.source_project.name}.{other.source_target.name}" / "reflection/include" ).resolve()  ) )
                    proj.extra_paths.append( str( (other.source_project.get_folder() / f"{other.source_project.name}.{other.source_target.name}"  / "reflection/").resolve() ) )

                if other.source_target.libraries_relevant():
                    proj.lib_dirs.add( str( (other.source_project.get_folder() / f"{other.source_project.name}.{other.source_target.name}" / "lib").resolve() ))
                    proj.libs.add(other.name + ".lib")
                    proj.libs_devel.add(other.name + "-dev.lib")
                    proj.libs_profile.add(other.name + "-profiler.lib")
                
                if other.source_target.binaries_relevant():
                    base_dir = (other.source_project.get_folder() / f"{other.source_project.name}.{other.source_target.name}" / "bin").resolve()
                    if other.source_target.type == TargetType.Application:
                        proj.binary_files.append( base_dir / f"{other.source_project.name}.{other.source_target.name}.exe" )
                        proj.binary_files_devel.append( base_dir / f"{other.source_project.name}.{other.source_target.name}-dev.exe" )
                        proj.binary_files_profile.append( base_dir / f"{other.source_project.name}.{other.source_target.name}-profiler.exe" )
                    else:
                        proj.binary_files.append( base_dir / f"{other.source_project.name}.{other.source_target.name}.dll" )
                        proj.binary_files_devel.append( base_dir / f"{other.source_project.name}.{other.source_target.name}-dev.dll" )
                        proj.binary_files_profile.append( base_dir / f"{other.source_project.name}.{other.source_target.name}-profiler.dll" )

                proj.resolved_deps.add(other.name)

            for dep in ext_deps:
                name_parts = dep.split(".", 1)

                if len(name_parts) != 2:
                    print("invalid dependency: {}".format(dep))
                    continue

                pkg_name : str = name_parts[0]
                tgt_name : str = name_parts[1]
                dep_pkg = self.context.get_package(pkg_name)
                if not dep_pkg:
                    print("invalid dependency: {}".format(dep))
                    continue

                dep_tgt = dep_pkg.get_target(tgt_name)
                if not dep_tgt:
                    print("invalid dependency: {}".format(dep))
                    continue

                #print("analyzing dep: {}".format(dep))

                for k, v in dep_tgt.defines.items():
                    proj.defines[k] = v

                for k, v in dep_tgt.defines_devel.items():
                    proj.defines_devel[k] = v 

                for k, v in dep_tgt.defines_profile.items():
                    proj.defines_profile[k] = v 

                for inc_dir in dep_tgt.include_paths:
                    proj.include_dirs.add( str( (dep_pkg.get_folder() / inc_dir).resolve() ) )

                if dep_tgt.reflection:
                    proj.extra_paths.append( str( (dep_pkg.get_folder() / f"{pkg_name}.{tgt_name}" / "reflection/").resolve() ) )

                for bin in dep_tgt.binaries:
                    proj.binary_files.append( str( (dep_pkg.get_folder() / "bin" / bin).resolve() ))

                for bin in dep_tgt.binaries_devel:
                    proj.binary_files_devel.append( str( (dep_pkg.get_folder() / "bin" / bin).resolve() ))

                for bin in dep_tgt.binaries_profile:
                    proj.binary_files_profile.append( str( (dep_pkg.get_folder() / "bin" / bin).resolve() ))

                if dep_tgt.libraries_relevant():
                    proj.lib_dirs.add( str( (dep_pkg.get_folder() / "lib").resolve() ))
                    proj.lib_dirs.add( str( (dep_pkg.get_folder() / dep_tgt.name / "lib").resolve() ))

                    for lib in dep_tgt.libraries:
                        proj.libs.add(lib)

                    for lib in dep_tgt.libraries_devel:
                        proj.libs_devel.add(lib)

                    for lib in dep_tgt.libraries_profile:
                        proj.libs_profile.add(lib)
                
                proj.resolved_deps.add(dep_tgt.name)

        # prepare the intermediate projects 
        for project in self.intermediate_projects:
            project.prepare()


if __name__ == "__main__":

    ctx:RuntimeContext = RuntimeContext()
    
    parser = argparse.ArgumentParser(prog="e2 package manager")
    parser.add_argument("command", default="list")
    parser.add_argument("-s", "--source")
    parser.add_argument("-t", "--target")
    args = parser.parse_args()

    if args.command == "list":
        for repo in ctx.repositories:
            for pkg_name in repo.get_package_names():
                print("Found package {}".format(pkg_name))

    if args.command == "generate":
        if not args.source:
            print("generate needs source parameter")
            exit()
        
        src_path = Path(args.source).resolve()
        if not src_path.exists():
            print("source does not exist")
            exit()

        src_proj : SourceProject = SourceProject(str(src_path))
        solution : IntermediateSolution = IntermediateSolution(ctx, src_proj)    

        templates_path = Path(__file__).resolve().parent / "templates"
        env = Environment(loader=FileSystemLoader(templates_path), trim_blocks=True, lstrip_blocks=True)

        # Write solution

        tmpl_sln = env.get_template("solution.sln")
        outpath_sln = Path(src_proj.get_folder() / "vcx/{}.sln".format(src_proj.name))
        outpath_sln.parent.mkdir(parents=True, exist_ok=True)

        with open(outpath_sln, "w") as file:
            file.write(tmpl_sln.render(solution=solution))

        # Write projects
        for proj in solution.intermediate_projects:
            tmpl_proj = env.get_template("project.vcxproj")
            outpath_proj = Path(src_proj.get_folder() / "vcx/{}.vcxproj".format(proj.name))

            # python C:\Work\Haikatekk\Engine2\e2pm.py -s C:\Work\Haikatekk\Engine2\Engine2\e2.e2s -t core scg
            scg_string : str = "python \"{}\" -s \"{}\" -t {} scg".format(Path(__file__).resolve(), src_proj.get_file(), proj.source_target.name)
            deploy_string : str = "python \"{}\" -s \"{}\" -t {} deploy".format(Path(__file__).resolve(), src_proj.get_file(), proj.source_target.name)
            with open(outpath_proj, "w") as file:
                file.write(tmpl_proj.render(solution=solution, project=proj, scg_string=scg_string, deploy_string=deploy_string))

            tmpl_fltr = env.get_template("project.vcxproj.filters")
            outpath_fltr = Path(src_proj.get_folder() / "vcx/{}.vcxproj.filters".format(proj.name))
            with open(outpath_fltr, "w") as file:
                file.write(tmpl_fltr.render(solution=solution, project=proj))

            if proj.type == IntermediateProjectType.Application:
                tmpl_user = env.get_template("project.vcxproj.user")
                outpath_user = Path(src_proj.get_folder() / "vcx/{}.vcxproj.user".format(proj.name))
                with open(outpath_user, "w") as file:
                    file.write(tmpl_user.render(solution=solution, project=proj))

    if args.command == "scg":
        if not args.source:
            print("scg needs source parameter")
            exit()

        if not args.target:
            print("scg needs target parameter")
            exit()
        
        src_path = Path(args.source).resolve()
        if not src_path.exists():
            print("source does not exist")
            exit()

        src_proj : SourceProject = SourceProject(str(src_path))
  
        selected_target : SourceTarget = None
        for target in src_proj.targets:
            if target.name == args.target:
                selected_target = target 
                break 

        if not selected_target:
            print("no such target in source")
            exit()

        if not selected_target.reflection:
            print("static code generation skipped for target {}".format(args.target))
            exit()

        int_sln = IntermediateSolution(ctx, src_proj)
        int_proj : IntermediateProject = None
        for int_r in int_sln.intermediate_projects:
            if int_r.source_target == selected_target:
                int_proj = int_r 

        if not int_proj:
            print("unknown error for target {}".format(args.target))
            exit()

        options : list[str] = []

        for inc in int_proj.include_dirs:
            options.append("-I{}".format(inc))

        for k,v in selected_target.private.defines.items():
            options.append("-D{}={}".format(k, v))

        for k,v in selected_target.public.defines.items():
            options.append("-D{}={}".format(k, v))
        
        #print("generating {} with options:".format(selected_target.name))
        #print(options)

        full_target_name = "{}.{}".format(src_proj.name, selected_target.name)
        intermediate_path = str((src_proj.get_folder() / full_target_name / "reflection").resolve())
        header_path = str((src_proj.get_folder() / full_target_name / "include").resolve())
        extra_paths : list[str] = int_proj.extra_paths

        for p in extra_paths:
            print(f"Extra reflection: {p}")

        scg.generate(intermediate_path, header_path, options, extra_paths, selected_target.api_define)

        #print(Path(__file__).resolve())

    if args.command == "deploy":
        if not args.source:
            print("deploy needs source parameter")
            exit()

        if not args.target:
            print("deploy needs target parameter")
            exit()
        
        src_path = Path(args.source).resolve()
        if not src_path.exists():
            print("source does not exist")
            exit()

        src_proj : SourceProject = SourceProject(str(src_path))
  
        selected_target : SourceTarget = None
        for target in src_proj.targets:
            if target.name == args.target:
                selected_target = target 
                break 

        if not selected_target:
            print("no such target in source")
            exit()

        int_sln = IntermediateSolution(ctx, src_proj)
        int_proj : IntermediateProject = None
        for int_r in int_sln.intermediate_projects:
            if int_r.source_target == selected_target:
                int_proj = int_r 

        if not int_proj:
            print("unknown error for target {}".format(args.target))
            exit()

        all_bins : set[str] = set()

        for binary in int_proj.binary_files:
            print(f"using binary {binary}")
            all_bins.add(binary)

        for binary in int_proj.binary_files_devel:
            print(f"using devel binary {binary}")
            all_bins.add(binary)

        for binary in int_proj.binary_files_profile:
            print(f"using profile binary {binary}")
            all_bins.add(binary)

        for bin in all_bins:
            try:
                shutil.copy(bin, src_proj.get_folder() / "deploy")
            except:
                pass