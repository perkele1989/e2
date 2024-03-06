import sys
import os
import time
from jinja2 import Environment, PackageLoader, FileSystemLoader, select_autoescape
import json
import pickle
from pathlib import *
from clang.cindex import *
from anytree import Node, NodeMixin, LevelOrderGroupIter
import concurrent.futures

def clean_path(dirtyPath):
    return str(Path(str(dirtyPath)).resolve())

class Symbol():

    def get_arena_size(self):
        if "arenaSize" in self.tags:
            return self.tags["arenaSize"]
        
        return "16"

    def get_tag_as_int(self, name:str, default:int):
        if name in self.tags:
            return int(self.tags[name])
        
        return default

    def __init__(self, cursor):
        self.symbolId : str = ""
        self.tags : dict[str, str] = {}

        self.symbolId = cursor.get_usr()
        cmnt = cursor.raw_comment
        if not cmnt:
            return

        tagString :str  = ""
        cmntOffset = cmnt.find("@tags(")
        if cmntOffset >= 0:
            endOffset = cmnt.find(")", cmntOffset)
            if endOffset == -1:
                endOffset = len(cmnt)
            tagString = cmnt[cmntOffset+6:endOffset]

        for stringTags in tagString.split(","):
            tag = stringTags.split("=")
            value = "true"
            key = tag[0].strip()
            if len(tag) > 1:
                value = tag[1].strip()
            self.tags[key] = value

class Variable(Symbol):
    def __init__(self, cursor:Cursor):
        self.name : str = ""
        self.type : str = ""

        super().__init__(cursor)

        self.name = cursor.spelling
        self.type = cursor.type.spelling

class Argument():
    def __init__(self):
        self.name : str = ""
        self.type : str = ""

class Constructor(Symbol):
    def __init__(self, cursor:Cursor):
        self.name : str = ""
        self.arguments : list[Argument] = []

        super().__init__(cursor)

        self.name = cursor.spelling

        arg:Cursor
        for arg in cursor.get_arguments():
            newArg = Argument()
            newArg.type = arg.type.spelling
            newArg.name = arg.spelling
            self.arguments.append(newArg)

    def get_args_string(self):
        output:str = ""
        i:int = 0
        for arg in self.arguments:
            if i > 0:
                output = output + ", "
            output = output + "{} {}".format(arg.type, arg.name)
            i = i + 1
        return output

    def get_args_string_no_name(self):
        output:str = ""
        i:int = 0
        for arg in self.arguments:
            if i > 0:
                output = output + ", "
            output = output + "{}".format(arg.type)
            i = i + 1
        return output
    
    def get_args_string_no_type(self):
        output:str = ""
        i:int = 0
        for arg in self.arguments:
            if i > 0:
                output = output + ", "
            output = output + "{}".format(arg.name)
            i = i + 1
        return output


class Method(Symbol):
    def __init__(self, cursor:Cursor):
        self.name : str = ""
        self.returnType : str = ""
        self.arguments : list[Argument] = []

        super().__init__(cursor)

        self.name = cursor.spelling
        self.returnType = cursor.type.get_result().spelling
        arg:Cursor
        for arg in cursor.get_arguments():
            newArg = Argument()
            newArg.type = arg.type.spelling
            newArg.name = arg.spelling
            self.arguments.append(newArg)

        

class Class(Symbol):
    def __init__(self, cursor:Cursor):
        self.name : str = ""
        self.methods : list[Method] = []
        self.constructors : list[Constructor] = []
        self.variables : list[Variable] = []
        self.namespace : list[str] = []
        self.bases : list[str] = []
        self.deep_bases : set[str] = set()
        self.abstract : bool = cursor.is_abstract_record()

        super().__init__(cursor)

        self.name = cursor.displayname

        parent = cursor.semantic_parent
        while parent:
            if parent.kind == CursorKind.NAMESPACE:
                self.namespace.append(parent.displayname)
            parent = parent.semantic_parent
        
        self.namespace.reverse()

    @property
    def fqn(self) -> str:
        return ("::".join(self.namespace) + "::" + self.name) if self.namespace else self.name

class CppParser():
    def __init__(self):
        self.classes : list[Class] = []

        self.external_classes : list[Class] = []

        self.include_paths : list[Path] = []

        # this index uses the class USR as key, only to be used during parsing (as it doesnt uniquely identify the class outside the parser)
        self.classIndex : dict[str, Class] = {}

        # this index uses the class FQN as key, and uniquely identifies the class outside the parser 
        self.classIndexFqn : dict[str, Class] = {}

        self.valid = True

    def parse_type(self, cursor:Cursor, primary:bool):
        newClass = Class(cursor)
        if primary:
            self.classes.append(newClass)
        else:
            self.external_classes.append(newClass)

        self.classIndex[newClass.symbolId] = newClass
        self.classIndexFqn[newClass.fqn] = newClass
        return newClass

    def parse_constructor(self, cursor:Cursor):
        # Filter out template classes and non-definitions
        if cursor.semantic_parent.kind not in [CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL] or not cursor.semantic_parent.is_definition():
            return
        
        newConstructor = Constructor(cursor)
        existingClass = self.classIndex[cursor.semantic_parent.get_usr()]
        existingClass.constructors.append(newConstructor)

    def parse_method(self, cursor:Cursor):
        # Filter out template classes and non-definitions
        if cursor.semantic_parent.kind not in [CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL] or not cursor.semantic_parent.is_definition():
            return
        
        newMethod = Method(cursor)
        existingClass = self.classIndex[cursor.semantic_parent.get_usr()]
        existingClass.methods.append(newMethod)

    def parse_field(self, cursor:Cursor):
        # Filter out template classes and non-definitions
        if cursor.semantic_parent.kind not in [CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL] or not cursor.semantic_parent.is_definition():
            return
        
        newVar = Variable(cursor)
        existingClass = self.classIndex[cursor.semantic_parent.get_usr()]
        existingClass.variables.append(newVar)

    def parse_base(self, cursor:Cursor, currentClass:Class):
        # Filter out template classes and non-definitions
        if currentClass == None:
            return
        currentClass.bases.append(cursor.type.spelling)

    def parse_include(self, cursor:Cursor):
        f = cursor.get_included_file()
        if f:
            self.include_paths.append(Path(f.name).resolve())

    def allow_path(self, path:str):
        if Path(path).resolve().is_relative_to("C:\\Program Files\\"):
            return False
        if Path(path).resolve().is_relative_to("C:\\Program Files (x86)\\"):
            return False
        return True

    def walk_all_files(self, cursor:Cursor):
        yield cursor
        for child in cursor.get_children():
            for descendant in self.walk_all_files(child):
                yield descendant

    def walk_only_same_file(self, cursor:Cursor, cleanPath:str):
        yield cursor
        for child in cursor.get_children():
            #if child.kind != CursorKind.INCLUSION_DIRECTIVE:
            childPath = clean_path(child.location.file)
            if cleanPath != childPath:
                continue
            for descendant in self.walk_only_same_file(child, cleanPath):
                #if descendant.kind != CursorKind.INCLUSION_DIRECTIVE:
                descPath = clean_path(descendant.location.file)
                if cleanPath != descPath:
                    continue
                yield descendant

    #def find_bases(self, fqn:str, out_bases:list[str]):
    #    if fqn not in self.classIndexFqn:
    #        return
    #    curr_class = self.classIndexFqn[fqn]
    #    out_bases.extend(curr_class.bases)
    #    for base in curr_class.bases:
    #        self.find_bases(base, out_bases)



    def namelinify(self, location):
        if location.file:
            tmp = Path(location.file.name)
            n = ""
            if tmp.name:
                n = tmp.name
            return "{}:{}".format(n, location.line)
        else:
            return "Unknown file:0"

    def parse(self, sourcePath : str, args : list[str], only_headers:bool):
        #print(f"parsing {sourcePath}")
        cleanPath = clean_path(sourcePath)
        index = Index.create()

        # only parse a single file
        opt_single_file = 0x400
        # keep parsing even for fatal errors
        opt_keep_going = 0x200
        # ignore warnings in included files
        opt_ignore_ext_warn = 0x4000
        # files are probably incomplete
        opt_incomplete = 0x02
        # want detailed parsing (include preprocessor directives so we get includes)
        opt_detailed = 0x01

        #opt = opt_ignore_ext_warn | opt_incomplete
        opt = 0x00
        if only_headers:
            opt = opt | opt_detailed | opt_single_file

        #opt = TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD if only_headers else TranslationUnit.PARSE_INCOMPLETE
        translationUnit = index.parse(cleanPath, args,options=opt)
        didPrint : bool = False
 
        # Only care about errors (and flag invalid) when doing a full parse
        if not only_headers:
            for d in translationUnit.diagnostics:
                if d.severity < Diagnostic.Error:
                    continue

                self.valid = False

                if not didPrint:
                    print("Errors while parsing {}:".format(Path(sourcePath).name))
                    didPrint = True

                print("{}: {}".format(self.namelinify(d.location), d.spelling))
                for c in d.children:
                    print("\t\t\t\t{}: {}".format(self.namelinify(c.location), c.spelling))
        cursor: Cursor
        # Used to resolve the class of CXX_BASE_SPECIFIER's as they apparently dont have any semantic or lexical parent 
        currentClass:Class = None
        for cursor in self.walk_only_same_file(translationUnit.cursor, cleanPath):
        #for cursor in self.walk_all_files(translationUnit.cursor):
            cursorPath = clean_path(cursor.location.file)
            #if cleanPath != cursorPath:
            #    continue
            if not only_headers:
                if cursor.kind in [CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL] and cursor.is_definition():
                    currentClass = self.parse_type(cursor, cursorPath == cleanPath)
                if cursor.kind == CursorKind.CXX_METHOD:
                    self.parse_method(cursor)
                if cursor.kind == CursorKind.CONSTRUCTOR:
                    self.parse_constructor(cursor)
                if cursor.kind == CursorKind.FIELD_DECL:
                    self.parse_field(cursor)
                if cursor.kind == CursorKind.CXX_BASE_SPECIFIER:
                    self.parse_base(cursor, currentClass)
            if cursor.kind == CursorKind.INCLUSION_DIRECTIVE:
                self.parse_include(cursor)
        
        # fill bases
        #for c in self.classes:
        #    tmp_bases : list[str] = []
        #    self.find_bases(c.fqn, tmp_bases)
        #    c.deep_bases.intersection_update(tmp_bases) #set(tmp_bases)

class HeaderFile():
    def __init__(self, full_path:str, relative_path:str):
        self.full_path : str = clean_path(full_path)
        self.relative_path = relative_path
        self.last_parsed : float = 0.0
        self.last_generated : float = 0.0
        self.classes : list[Class] = []
        self.include_paths : list[Path] = []
        

    @property
    def include_path(self) -> str:
        return self.full_path.replace("\\", "/")

    def get_last_modified(self):
        return os.path.getmtime(self.full_path)

    def update_last_parsed(self):
        self.last_parsed = time.time() 

    def update_last_generated(self):
        self.last_generated = time.time() 

    def needs_parse(self):
        return self.get_last_modified() > self.last_parsed
    
    def has_managed_objects(self):
        for c in self.classes:
            if "e2::Object" in c.deep_bases:
                return True
        return False

    def needs_init(self):
        return self.has_managed_objects()

    def needs_generation(self):
        if self.last_parsed <= self.last_generated:
            return False

        return self.has_managed_objects()

    def exists(self):
        return os.path.exists(self.full_path)


class Database():
    def __init__(self, intermediate_path:str, source_path:str = None, readonly:bool = True):
        self.intermediate_path = clean_path(intermediate_path)
        self.read_only = readonly
        if source_path and not self.read_only:
            self.source_path = clean_path(source_path)

        self.files : list[HeaderFile] = []
        self.file_index : dict[str, HeaderFile] = {}
        self.class_index : dict[str, Class] = {}

        if not self.read_only:
            Path(self.intermediate_path).mkdir(parents=True, exist_ok=True)

    def get_database_file(self):
        return clean_path("{}/db.bin".format(self.intermediate_path))

    def load(self):
        try:
            with open(self.get_database_file(), "rb") as file:
                tmp_dict = pickle.load(file)
                self.__dict__.update(tmp_dict)
        except EnvironmentError:
            print("No existing database")
            return False

        if self.read_only:
            return True

        # Clean any files from the database that has been deleted from disk
        self.files = [file for file in self.files if file.exists()]

        # Build an index for the files for quicker searches when globbing in the next step
        file_map : dict[str, HeaderFile] = {file.full_path: file for file in self.files}

        # Add any new files that doesnt already exist to the database
        new_files = [HeaderFile(clean_path(x), str(Path(clean_path(x)).relative_to(self.source_path))) for x in Path(self.source_path).glob("**/*.hpp") if clean_path(x) not in file_map]
        self.files.extend(new_files)

        self.file_index = {file.full_path: file for file in self.files}

        return True

    def save(self):
        if not self.read_only:
            with open(self.get_database_file(), "wb") as file:
                pickle.dump(self.__dict__, file)
 
    def resolve_file(self, filePath:str):
        cleanedPath:str = clean_path(filePath)
        for f in self.files:
            if f.full_path == cleanedPath:
                return f
            
        newFile = HeaderFile(cleanedPath)
        self.files.append(newFile)
        self.file_index[cleanedPath] = newFile
        return newFile
        
class DatabaseEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, set):
            return list(obj)
        if isinstance(obj, object):
            return obj.__dict__
        # Let the base class default method raise the TypeError
        return json.JSONEncoder.default(self, obj)



#
#   @tags(arena, arenaSize=1024)
#   
#

class PreParseJob():
    def __init__(self, file:HeaderFile):
        self.file = file

class ParseJob():
    def __init__(self, file:HeaderFile):
        self.file = file
        self.valid = True

def prepare_preparse_job(job:PreParseJob, intermediate_path:str):
    outfile_inc = Path(clean_path("{}/include/{}".format(intermediate_path, Path(job.file.relative_path).name))).with_suffix(".generated.hpp")
    if not outfile_inc.exists():
        outfile_inc.parent.mkdir(parents=True, exist_ok=True)
        outfile_inc.touch()

def execute_parse_job(job:ParseJob, options:list[str]):

    parser = CppParser()
    parser.parse(job.file.full_path, options, False)

    print(f"Parsed {job.file.relative_path}")

    job.valid = parser.valid
    job.file.classes = parser.classes
    job.file.include_paths = parser.include_paths

    return job



def execute_preparse_job(job:PreParseJob, options:list[str]):
    parser = CppParser()
    parser.parse(job.file.full_path, options, True)

    job.file.include_paths = parser.include_paths

    return job



def find_all_bases(cs:str, out_set:set[str], class_index:dict[str, Class], extra_dbs:list[Database]):

    # find the class
    c:Class = None
    if cs in class_index:
        c = class_index[cs]
    else:
        for db in extra_dbs:
            if cs in db.class_index:
                c = db.class_index[cs]
                break

    if not c:
        return

    for b in c.bases:
        out_set.add(b)
        find_all_bases(b, out_set, class_index, extra_dbs)
    
class GenerateJob():
    def __init__(self, file:HeaderFile):
        self.file = file

def execute_generate_job(job:GenerateJob, intermediate_path:str, api_define:str):
    templates_path = Path(__file__).resolve().parent / "templates"
    env = Environment(loader=FileSystemLoader(templates_path), trim_blocks=True, lstrip_blocks=True)

    outfile_inc = Path(clean_path("{}/include/{}".format(intermediate_path, Path(job.file.relative_path).name))).with_suffix(".generated.hpp")
    tmpl_inc = env.get_template("inc.hpp")
    Path(outfile_inc).parent.mkdir(parents=True, exist_ok=True)
    with open(outfile_inc, "w") as file:
        file.write(tmpl_inc.render(classes=job.file.classes, api_define=api_define))

    outfile_src = Path(clean_path("{}/src/{}".format(intermediate_path, Path(job.file.relative_path).name))).with_suffix(".generated.cpp")
    tmpl_src = env.get_template("src.cpp")
    Path(outfile_src).parent.mkdir(parents=True, exist_ok=True)
    with open(outfile_src, "w") as file:
        file.write(tmpl_src.render(classes=job.file.classes, file=job.file))

    return job
#
def generate(intermediate_path:str, source_path:str, options:list[str], extra_intermediate_paths:list[str], api_define:str):

    if not Config.loaded:
        Config.set_library_file("C:\\Program Files\\LLVM\\bin\libclang.dll")

    all_options:list[str] = ["-std=c++20", "-DE2_SCG"] # -IFullPathInclude 
    all_options.extend(options)

    extra_dbs : list[Database] = []
    for int_path in extra_intermediate_paths:
        new_db = Database(int_path, None, True)
        if not new_db.load():
            print(f"Need to generate reflection database for dependencies first")
        extra_dbs.append(new_db)

    print("Listing options:")
    for opt in all_options:
        print("\t\t\t\t{}".format(opt))

    # Load the database of parsed files
    db = Database(intermediate_path=intermediate_path, source_path=source_path, readonly=False)

    print("Loading database..")
    db.load()
    
    # Gather jobs for dirty files
    print("Pre-processing..")
    preparse_jobs = [PreParseJob(file) for file in db.files if file.needs_parse()]
    #preparse_jobs = preparse_jobs[:2]

    for job in preparse_jobs:
        prepare_preparse_job(job, intermediate_path)

    # Execute the jobs on a thread pool and wait for them to complete
    with concurrent.futures.ThreadPoolExecutor(max_workers=14) as executor:
        futures = []
        for preparse_job in preparse_jobs:
            #print("Pre-processing {}..".format(preparse_job.file.relative_path))
            futures.append(executor.submit(execute_preparse_job, job=preparse_job, options=all_options))
        for x in concurrent.futures.as_completed(futures):
            pass
            #print("x")


    # create filename -> headerfile index 
    files_to_order: list[HeaderFile] = [x.file for x in preparse_jobs]
    header_index : dict[str, HeaderFile] = { file.full_path: file for file in files_to_order}

    queues : list[list[HeaderFile]] = []
    global_queue_list : list[HeaderFile] = []

    queue_count = 0
    while files_to_order:
        queue_list : list[HeaderFile] = []
        remove_list : list[HeaderFile]  = []
        for man_file in files_to_order:

            # we should add F if
            # 1. every include path in F is in global_queue_list 

            do_add = True
            for inc_path in man_file.include_paths:
                inc_str = str(inc_path)

                # ignore includes that are not part of this project 
                if inc_str not in header_index:
                    continue

                # dont add files that are already in the global queue
                inc_file = header_index[inc_str]
                if inc_file not in global_queue_list:
                    do_add = False
                    break

            if do_add:
                queue_list.append(man_file)
                remove_list.append(man_file)
        if queue_list:
            queues.append(queue_list)
            global_queue_list.extend(queue_list)
        
        for f in remove_list:
            files_to_order.remove(f)

        queue_count = queue_count + 1 

        if queue_count > 8:
            break
    
    #i = 0
    #for q in queues:
    #    print("QUEUE {}:".format(i))
    #    for f in q:
    #        print("\t\t{}".format(f.full_path))
    #    i = i + 1
    #exit()

    # todo: resolve header dependencies and more intelligently create parse job sequences

    #class_index : dict[str, Class] = {}

    # Gather jobs for dirty files
    i = 0
    for q in queues:
        print("Parsing jobs (pass {})..".format(i))
        #parse_jobs = [ParseJob(file) for file in db.files if file.needs_parse()]
        parse_jobs = [ParseJob(file) for file in q if file.needs_parse()] 

        # Execute the jobs on a thread pool and wait for them to complete
        with concurrent.futures.ThreadPoolExecutor(max_workers=14) as executor:
            futures = []
            for parse_job in parse_jobs:
                #print("Parsing {}..".format(parse_job.file.relative_path))
                futures.append(executor.submit(execute_parse_job, job=parse_job, options=all_options))
            for future in concurrent.futures.as_completed(futures):
                if future.result().valid:
                    completed_file:HeaderFile = future.result().file
                    completed_file.update_last_parsed()

        # populate class index from this queue
        for f in q:
            for c in f.classes:
                db.class_index[c.fqn] = c 


        # Populate deep bases from this queue
        for f in q:
            print("\tF: {}".format(f.relative_path))
            for c in f.classes:
                print("\t\tC: {}".format(c.fqn))
                tmpset:set[str] = set()
                find_all_bases(c.fqn, tmpset, db.class_index, extra_dbs)
                c.deep_bases = tmpset
                for dbb in c.deep_bases:
                    print("\t\t\tDB: {}".format(dbb))

        print("Generating files.. (pass {})..".format(i))
        generate_jobs = [GenerateJob(file) for file in q if file.needs_generation()]

        with concurrent.futures.ThreadPoolExecutor(max_workers=14) as executor:
            futures = []
            for generate_job in generate_jobs:
                #print("Generating files for {}..".format(generate_job.file.relative_path))
                futures.append(executor.submit(execute_generate_job, job=generate_job, intermediate_path=intermediate_path, api_define=api_define))
            for future in concurrent.futures.as_completed(futures):
                completed_file:HeaderFile = future.result().file
                completed_file.update_last_generated()
        i  = i  + 1



    # Writing init file 
    print("Writing init file..")
    outfile_init = Path(clean_path("{}/init/init.inl".format(intermediate_path)))
    Path(outfile_init).parent.mkdir(parents=True, exist_ok=True)
    templates_path = Path(__file__).resolve().parent / "templates"
    env = Environment(loader=FileSystemLoader(templates_path), trim_blocks=True, lstrip_blocks=True)
    tmpl_init = env.get_template("init.inl")
    Path(outfile_init).parent.mkdir(parents=True, exist_ok=True)
    with open(outfile_init, "w") as file:
        file.write(tmpl_init.render(files=db.files))    

    # Save the database
    print("Saving database..")
    db.save()
