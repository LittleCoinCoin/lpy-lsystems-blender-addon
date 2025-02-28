/* ---------------------------------------------------------------------------
 #
 #       L-Py: L-systems in Python
 #
 #       Copyright 2003-2008 UMR Cirad/Inria/Inra Dap - Virtual Plant Team
 #
 #       File author(s): F. Boudon (frederic.boudon@cirad.fr)
 #
 # ---------------------------------------------------------------------------
 #
 #                      GNU General Public Licence
 #
 #       This program is free software; you can redistribute it and/or
 #       modify it under the terms of the GNU General Public License as
 #       published by the Free Software Foundation; either version 2 of
 #       the License, or (at your option) any later version.
 #
 #       This program is distributed in the hope that it will be useful,
 #       but WITHOUT ANY WARRANTY; without even the implied warranty of
 #       MERCHANTABILITY or FITNESS For A PARTICULAR PURPOSE. See the
 #       GNU General Public License for more details.
 #
 #       You should have received a copy of the GNU General Public
 #       License along with this program; see the file COPYING. If not,
 #       write to the Free Software Foundation, Inc., 59
 #       Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 #
 # ---------------------------------------------------------------------------
 */

#define BOOST_PYTHON_STATIC_LIB
#include "lsyscontext.h"
#include "lsysrule.h"
#include "matching.h"
#include "lpy_parser.h"
#include "compilation.h"
#include "tracker.h"
#include <stack>

using namespace boost::python;
LPY_USING_NAMESPACE

#ifndef LPY_NO_PLANTGL_INTERPRETATION
#include "../plantgl/version.h"
PGL_USING(PglTurtle)
#endif
/*---------------------------------------------------------------------------*/

const std::string LsysContext::InitialisationFunctionName("__initialiseContext__");
const std::string LsysContext::AxiomVariable("__axiom__");
const std::string LsysContext::DerivationLengthVariable("__derivation_length__");
const std::string LsysContext::DecompositionMaxDepthVariable("__decomposition_max_depth__");
const std::string LsysContext::HomomorphismMaxDepthVariable("__homomorphism_max_depth__");

double LsysContext::DefaultAnimationTimeStep(0.05);
const int LsysContext::DEFAULT_OPTIMIZATION_LEVEL(1);

/*---------------------------------------------------------------------------*/

static GlobalContext * GLOBAL_LSYSCONTEXT = NULL;


static std::vector<LsysContext *> LSYSCONTEXT_STACK;
static LsysContext * DEFAULT_LSYSCONTEXT = NULL;
// static LsysContext * CURRENT_LSYSCONTEXT = LsysContext::globalContext();
static LsysContext * CURRENT_LSYSCONTEXT = NULL;

class ContextGarbageCollector
{
public:
	ContextGarbageCollector() {}
	~ContextGarbageCollector() { 
		// std::cerr  << "context garbage collector" << std::endl;
		if (GLOBAL_LSYSCONTEXT){
			LsysContext::cleanContexts();
		}
	}
protected:
	static ContextGarbageCollector m_INSTANCE;
};

void LsysContext::cleanContexts(){
	if (DEFAULT_LSYSCONTEXT){
		delete DEFAULT_LSYSCONTEXT;
		DEFAULT_LSYSCONTEXT = NULL;
	}
	if (GLOBAL_LSYSCONTEXT)
	{
		delete GLOBAL_LSYSCONTEXT;
		GLOBAL_LSYSCONTEXT = NULL;
	}
}

LsysContext *
LsysContext::globalContext()
{ 
    if(!GLOBAL_LSYSCONTEXT)  GLOBAL_LSYSCONTEXT = new GlobalContext();
	return GLOBAL_LSYSCONTEXT; 
}

void createDefaultContext()
{ 
    if(!DEFAULT_LSYSCONTEXT){
        DEFAULT_LSYSCONTEXT = new LocalContext();

		/*
        // copy __builtins__ for import and all.
		DEFAULT_LSYSCONTEXT->copyObjectToGlobal("__builtins__",global);

		if (!DEFAULT_LSYSCONTEXT->hasObject("__builtins__"))
			DEFAULT_LSYSCONTEXT->setObjectToGlobal("__builtins__", object(handle<>(borrowed( PyModule_GetDict(PyImport_AddModule("__builtin__"))))));

        
        // import lpy
        DEFAULT_LSYSCONTEXT->compileInGlobal("from lpy import *");
        */
   }
}


LsysContext *
LsysContext::defaultContext()
{ 
    if(!DEFAULT_LSYSCONTEXT) createDefaultContext();
    return DEFAULT_LSYSCONTEXT; 
}


LsysContext *
LsysContext::current()
{ 
    if(!CURRENT_LSYSCONTEXT) CURRENT_LSYSCONTEXT = globalContext(); // defaultContext();
	return CURRENT_LSYSCONTEXT; 
}

void 
LsysContext::makeCurrent() 
{ 
  LsysContext * previous = currentContext();
  if (previous == this) {
	  LsysWarning("Multiple activation of same context!");
  }
  LSYSCONTEXT_STACK.push_back(previous);
  CURRENT_LSYSCONTEXT = this;
  previous->pushedEvent(CURRENT_LSYSCONTEXT);
  CURRENT_LSYSCONTEXT->currentEvent();
}

void 
LsysContext::done() 
{ 
  if(isCurrent() && !LSYSCONTEXT_STACK.empty()){
	CURRENT_LSYSCONTEXT = LSYSCONTEXT_STACK.back();
	LSYSCONTEXT_STACK.pop_back();
	doneEvent();
	CURRENT_LSYSCONTEXT->restoreEvent(this);
  }
  else if (this != DEFAULT_LSYSCONTEXT){ LsysError("Not current context trying to be done."); }
}

bool 
LsysContext::isCurrent() const 
{ 
  if (!CURRENT_LSYSCONTEXT) { CURRENT_LSYSCONTEXT = GLOBAL_LSYSCONTEXT; }
  return CURRENT_LSYSCONTEXT == this; 
}


void LsysContext::currentEvent()
{
	for(LsysOptions::iterator it = options.begin(); it != options.end(); ++it)
		(*it)->activateSelection();
	for(ModuleClassList::const_iterator it = m_modules.begin(); it != m_modules.end(); ++it)
		(*it)->activate();
	for(ModuleVTableList::const_iterator it = m_modulesvtables.begin(); it != m_modulesvtables.end(); ++it)
		(*it)->activate();
	for(AliasSet::const_iterator it = m_aliases.begin(); it != m_aliases.end(); ++it)
	    { ModuleClassTable::get().alias(it->first,it->second); }
}

void LsysContext::doneEvent()
{
	for(ModuleClassList::const_iterator it = m_modules.begin(); it != m_modules.end(); ++it)
		(*it)->desactivate();
	for(ModuleVTableList::const_iterator it = m_modulesvtables.begin(); it != m_modulesvtables.end(); ++it)
		(*it)->desactivate();
	for(AliasSet::const_iterator it = m_aliases.begin(); it != m_aliases.end(); ++it)
	    { ModuleClassTable::get().remove(it->first); }
}

void LsysContext::pushedEvent(LsysContext * newEvent)
{
	doneEvent();
}

void LsysContext::restoreEvent(LsysContext * previousEvent)
{
	currentEvent();
}

/*---------------------------------------------------------------------------*/

LsysContext::LsysContext():
m_direction(eForward),
m_group(0),
m_selection_always_required(false),
m_selection_requested(false),
m_warn_with_sharp_module(true),
return_if_no_matching(true),
optimizationLevel(DEFAULT_OPTIMIZATION_LEVEL),
m_animation_step(DefaultAnimationTimeStep),
m_animation_enabled(false),
m_iteration_nb(0),
m_nbargs_of_endeach(0),
m_nbargs_of_end(0),
m_nbargs_of_starteach(0),
m_nbargs_of_start(0),
m_early_return(false),
m_early_return_mutex(),
m_paramproductions()
{
	IncTracker(LsysContext)
	init_options();
}

LsysContext::LsysContext(const LsysContext& lsys):
  m_direction(lsys.m_direction),
  m_group(lsys.m_group),
  m_nproduction(lsys.m_nproduction),
  m_selection_always_required(lsys.m_selection_always_required),
  m_selection_requested(false),
  m_warn_with_sharp_module(lsys.m_warn_with_sharp_module),
  return_if_no_matching(lsys.return_if_no_matching),
  optimizationLevel(lsys.optimizationLevel),
  m_animation_step(lsys.m_animation_step),
  m_animation_enabled(lsys.m_animation_enabled),
  m_iteration_nb(0),
  m_nbargs_of_endeach(0),
  m_nbargs_of_end(0),
  m_nbargs_of_starteach(0),
  m_nbargs_of_start(0),
  m_early_return(false),
  m_early_return_mutex(),
  m_paramproductions()
{
	IncTracker(LsysContext)
	init_options();
}

LsysContext::LsysContext(const boost::python::dict& locals):
m_direction(eForward),
m_group(0),
m_selection_always_required(false),
m_selection_requested(false),
m_warn_with_sharp_module(true),
return_if_no_matching(true),
optimizationLevel(DEFAULT_OPTIMIZATION_LEVEL),
m_animation_step(DefaultAnimationTimeStep),
m_animation_enabled(false),
m_iteration_nb(0),
m_nbargs_of_endeach(0),
m_nbargs_of_end(0),
m_nbargs_of_starteach(0),
m_nbargs_of_start(0),
m_early_return(false),
m_early_return_mutex(),
m_paramproductions(),
m_locals(locals)
{
	IncTracker(LsysContext)
	init_options();
}
LsysContext& 
LsysContext::operator=(const LsysContext& lsys)
{
  m_direction = lsys.m_direction;
  m_group = lsys.m_group;
  m_nproduction = lsys.m_nproduction;
  m_selection_always_required = lsys.m_selection_always_required;
  m_selection_requested = false;
  m_warn_with_sharp_module = lsys.m_warn_with_sharp_module;
  return_if_no_matching = lsys.return_if_no_matching;
  optimizationLevel = lsys.optimizationLevel;
  m_animation_step =lsys.m_animation_step;
  m_animation_enabled =lsys.m_animation_enabled;
  m_nbargs_of_endeach =lsys.m_nbargs_of_endeach;
  m_nbargs_of_end =lsys.m_nbargs_of_end;
  m_nbargs_of_starteach =lsys.m_nbargs_of_starteach;
  m_nbargs_of_start =lsys.m_nbargs_of_start;
  m_early_return = false;
  m_paramproductions = lsys.m_paramproductions;
  return *this;
}

void 
LsysContext::importContext(const LsysContext& other)
{
	// declareModules(other.declaredModules());
    size_t nb = m_modules.size();

	m_modules.insert(m_modules.end(),other.m_modules.begin(),other.m_modules.end()); 
	m_modulesvtables.insert(m_modulesvtables.end(),other.m_modulesvtables.begin(),other.m_modulesvtables.end()); 

	bool iscurrent = isCurrent();
	for(ModuleClassList::iterator it = m_modules.begin()+nb; it != m_modules.end(); ++it)
	{
		(*it)->activate(iscurrent);
	}

	add_pproductions(other.get_pproductions());
	updateFromContextNamespace(other);

}


LsysContext::~LsysContext()
{
	DecTracker(LsysContext)
	// std::cerr << "context deleted" << std::endl;
}

void LsysContext::init_options()
{
	LsysOption* option;
	/** module declaration option */
	option = options.add("Module declaration","Specify if module declaration is mandatory.","Compilation");
	option->addValue("On the fly",&ModuleClassTable::setMandatoryDeclaration,false,"Module declaration is made on the fly when parsing string.");
	option->addValue("Mandatory",&ModuleClassTable::setMandatoryDeclaration,true,"Module declaration is mandatory before use in string.");
	option->setDefault(0);
	option->setGlobal(true);
	/** Warning with sharp module option */
	option = options.add("Warning with sharp module","Set whether a warning is made when sharp symbol is met when parsing (compatibility with cpfg).","Compilation");
	option->addValue("Disabled",this,&LsysContext::setWarnWithSharpModule,false,"Disable Warning.");
	option->addValue("Enabled",this,&LsysContext::setWarnWithSharpModule,true,"Enable Warning.");
	option->setDefault(1);	
	/** compiler option */
	option = options.add("Compiler","Specify the compiler to use","Compilation");
	option->addValue("Python",&Compilation::setCompiler,Compilation::ePythonStr,"Use python compiler.");
	option->addValue("Python -OO",&Compilation::setCompiler,Compilation::ePythonFile,"Use python compiler.");
	if(Compilation::isCythonAvailable())
		option->addValue("Cython",&Compilation::setCompiler,Compilation::eCython,"Use Cython compiler.");
	option->setDefault(Compilation::eDefaultCompiler);
	option->setGlobal(true);
	/** optimization option */
	option = options.add("Optimization","Specify the level of optimization to use","Compilation");
	option->addValue("Level 0",this,&LsysContext::setOptimizationLevel,0,"Use Level 0.");
	option->addValue("Level 1",this,&LsysContext::setOptimizationLevel,1,"Use Level 1.");
	option->addValue("Level 2",this,&LsysContext::setOptimizationLevel,2,"Use Level 2.");
	option->setDefault(DEFAULT_OPTIMIZATION_LEVEL);
	/** module matching option */
	option = options.add("Module matching","Specify the way modules are matched to rules pattern","Matching");
	option->addValue("Simple",&MatchingEngine::setModuleMatchingMethod,MatchingEngine::eMSimple,"Simple module matching : Same name and same number of arguments . '*' module allowed.");
	option->addValue("Extension : *args",&MatchingEngine::setModuleMatchingMethod,MatchingEngine::eMWithStar,"Add matching rule that * module can match any module and allow *args to match a number of module arguments");
	option->addValue("Extensions : *args and arg value constraints",&MatchingEngine::setModuleMatchingMethod,MatchingEngine::eMWithStarNValueConstraint,"With * module and *args. A module argument can also be set to a given value adding thus a new matching constraint.");
	option->setDefault(MatchingEngine::eDefaultModuleMatching);
	option->setGlobal(true);
	/** module matching option */
	option = options.add("Module inheritance","Specify if modules inheritance is taken into account in rules pattern matching","Matching");
	option->addValue("Disabled",&MatchingEngine::setInheritanceModuleMatchingActivated,false,"Modules with different names are considered as different even if they inherit from each other.");
	option->addValue("Enabled",&MatchingEngine::setInheritanceModuleMatchingActivated,true,"A module inheriting from a second one can be used for application of the first one if number of parameters are also compatible.");
	option->setDefault(0);
	option->setGlobal(true);
	/** string matching option */
	option = options.add("String matching","Specify the way strings are matched to rules pattern","Matching");
	option->addValue("As String",&MatchingEngine::setStringMatchingMethod,MatchingEngine::eString,"String is considered as a simple string.");
	option->addValue("As AxialTree",&MatchingEngine::setStringMatchingMethod,MatchingEngine::eAxialTree,"String is considered as an axial tree and some modules can be skipped according to tree connectivity.");
	option->addValue("As Multi-level AxialTree",&MatchingEngine::setStringMatchingMethod,MatchingEngine::eMLevelAxialTree,"String is considered as a multi level axial tree and some modules can be skipped according to module level. Level are not ordered.");
	option->addValue("As Multiscale AxialTree",&MatchingEngine::setStringMatchingMethod,MatchingEngine::eMScaleAxialTree,"String is considered as a multi scale axial tree and some modules can be skipped according to module scale.");
	option->setDefault(MatchingEngine::eDefaultStringMatching);
	option->setGlobal(true);
	/** early return when no matching option */
	option = options.add("Early return when no matching","Set whether the L-systems end prematurely if no matching has occured in the last iteration.","Processing");
	option->addValue("Disabled",this,&LsysContext::setReturnIfNoMatching,false,"Disable early return.");
	option->addValue("Enabled",this,&LsysContext::setReturnIfNoMatching,true,"Enable early return.");
	option->setDefault(0);
	
#ifndef LPY_NO_PLANTGL_INTERPRETATION
#if (PGL_VERSION >= 0x020B00)
	/** warn if turtle has invalid value option */
	option = options.add("Warning with Turtle inconsistency","Set whether a warning/error is raised when an invalid value is found during turtle processing.","Processing");
	option->addValue<PglTurtle,bool>("Disabled",&turtle,&PglTurtle::setWarnOnError,false,"Disable warnings/errors.");
	option->addValue<PglTurtle,bool>("Enabled",&turtle,&PglTurtle::setWarnOnError,true,"Enable warnings/errors.");
	option->setDefault(turtle.warnOnError());	
#endif
#endif

	/** selection required option */
	option = options.add("Selection Always Required","Set whether selection check in GUI is required or not. Selection is then transform in X module in the Lstring.","Interaction");
	option->addValue("Disabled",this,&LsysContext::setSelectionAlwaysRequired,false,"Disable Selection Check.");
	option->addValue("Enabled",this,&LsysContext::setSelectionAlwaysRequired,true,"Enable Selection Check.");
	option->setDefault(0);
}

void 
LsysContext::clear(){
  m_direction = eForward;
  m_group = 0;
  m_selection_always_required = false;
  m_selection_requested = false;
  m_iteration_nb = 0;
  m_animation_enabled = false;
  m_nbargs_of_endeach = 0;
  m_nbargs_of_end = 0;
  m_nbargs_of_starteach = 0;
  m_nbargs_of_start = 0;
  m_modules.clear();
  m_modulesvtables.clear();
  m_aliases.clear();
  m_paramproductions.clear();
  m_early_return = false;
  clearNamespace();
}

bool
LsysContext::empty( ) const {
  return m_modules.empty(); 
}
/*---------------------------------------------------------------------------*/

void 
LsysContext::declare(const std::string& modules)
{
	LpyParsing::ModDeclarationList moduleclasses = LpyParsing::parse_moddeclist(modules);
	bool iscurrent = isCurrent();
	for(LpyParsing::ModDeclarationList::const_iterator it = moduleclasses.begin();
		it != moduleclasses.end(); ++it)
	{
		ModuleClassPtr mod;
		if(!it->alias){
			mod = ModuleClassTable::get().declare(it->name);
			m_modules.push_back(mod);
			if(!it->parameters.empty()){
				std::vector<std::string> args = LpyParsing::parse_arguments(it->parameters);
				for(std::vector<std::string>::const_iterator itarg = args.begin(); itarg != args.end(); ++itarg){
					if(!LpyParsing::isValidVariableName(*itarg))LsysError("Invalid parameter name '"+*itarg+"'");
				}
			    mod->setParameterNames(args);
			}
		}
		else mod = ModuleClassTable::get().alias(it->name,it->parameters);
		mod->activate(iscurrent);
	}
}

void LsysContext::declareModules(const ModuleClassList& other) { 
	size_t nb = m_modules.size();
	m_modules.insert(m_modules.end(),other.begin(),other.end()); 
	bool iscurrent = isCurrent();
	for(ModuleClassList::iterator it = m_modules.begin()+nb; it != m_modules.end(); ++it)
	{
		(*it)->activate(iscurrent);
	}
}

void 
LsysContext::undeclare(const std::string& modules)
{
	LpyParsing::ModNameList moduleclasses = LpyParsing::parse_modlist(modules);
	bool iscurrent = isCurrent();
	for(LpyParsing::ModNameList::const_iterator it = moduleclasses.begin();
		it != moduleclasses.end(); ++it)
	{
		ModuleClassPtr mod = ModuleClassTable::get().getClass(*it);
		undeclare(mod);
	}
}

void 
LsysContext::undeclare(ModuleClassPtr module)
{
	ModuleClassList::iterator it = std::find(m_modules.begin(),m_modules.end(),module);
	if (it == m_modules.end()) LsysError("Cannot undeclare module '"+module->name+"'. Not declared in this scope.");
	else { 
			m_modules.erase(it); 
			if(module->m_vtable){
				ModuleVTableList::iterator it = 
					find(m_modulesvtables.begin(),m_modulesvtables.end(),module->m_vtable);
				m_modulesvtables.erase(it);
			}
			if (isCurrent())module->desactivate(); 
		 }
}


bool LsysContext::isDeclared(const std::string& module)
{
	ModuleClassPtr mod = ModuleClassTable::get().getClass(module);
	if(!mod) return false;
	else return isDeclared(mod);
}

bool LsysContext::isDeclared(ModuleClassPtr module)
{
	ModuleClassList::iterator it = std::find(m_modules.begin(),m_modules.end(),module);
	return it != m_modules.end();
}

void LsysContext::declareAlias(const std::string& alias, ModuleClassPtr module)
{ m_aliases[alias] = module; }

void LsysContext::setModuleScale(const std::string& modules, int scale)
{
	LpyParsing::ModNameList moduleclasses = LpyParsing::parse_modlist(modules);
	if (moduleclasses.size() > 0){
		ContextMaintainer cm(this);
		for(LpyParsing::ModNameList::const_iterator it = moduleclasses.begin();
			it != moduleclasses.end(); ++it)
		{
			ModuleClassPtr mod = ModuleClassTable::get().getClass(*it);
			if(mod)mod->setScale(scale);
		}
	}
}

/*---------------------------------------------------------------------------*/

bool 
LsysContext::hasObject(const std::string& name) const{
  if (m_locals.has_key(name)) return true;  
  return PyDict_Contains(globals(),object(name).ptr());
}

object
LsysContext::getObject(const std::string& name, const boost::python::object& defaultvalue) const
{
  if (m_locals.has_key(name)) {
    //printf("key found: %s\n", name.c_str());
    return m_locals.get(name,defaultvalue);
  }
  //printf("%s: %s\n","key not found", name.c_str());
  handle<> res(allow_null(PyDict_GetItemString(globals(),name.c_str())));
  if(res) return object(handle<>(borrowed(res.get())));
  return defaultvalue;
}

void 
LsysContext::setObject(const std::string& name, 
					   const boost::python::object& o)
{
  m_locals[name] = o;
}

void 
LsysContext::setObjectToGlobals(const std::string& name, 
							   const boost::python::object& o)
{
  PyObject * _globals = globals();
  assert(_globals != NULL);
  PyDict_SetItemString(_globals,name.c_str(),o.ptr());
}

void 
LsysContext::delObject(const std::string& name) {
  if (m_locals.has_key(name)) m_locals[name].del();
  else PyDict_DelItemString(globals(),name.c_str());
}

bool 
LsysContext::copyObject(const std::string& name, LsysContext * sourceContext)
{
	if (!sourceContext) return false;
	boost::python::object obj = sourceContext->getObject(name);
	if (obj == object()) return false;
	m_locals[name] = obj;
	return true;
	// PyObject * obj = PyDict_GetItemString(sourceContext->Namespace(),name.c_str());
}


bool 
LsysContext::copyObjectToGlobals(const std::string& name, LsysContext * sourceContext)
{
	if (!sourceContext) return false;
	PyObject * obj = PyDict_GetItemString(sourceContext->locals().ptr(),name.c_str());
	if (obj == NULL) obj = PyDict_GetItemString(sourceContext->globals(),name.c_str());
	if (obj == NULL) return false;
	PyDict_SetItemString(globals(),name.c_str(),obj);
	return true;
}



void
LsysContext::updateNamespace(const boost::python::dict& d){
  PyDict_Update(locals().ptr(),d.ptr());
}

void 
LsysContext::updateFromContextNamespace(const LsysContext& other)
{
  PyDict_Update(locals().ptr(),other.locals().ptr());
  PyDict_Update(globals(),other.globals());
}

void
LsysContext::getNamespace(boost::python::dict& d) const{
	PyDict_Update(d.ptr(),locals().ptr());
}


void
LsysContext::clearNamespace() {
	m_locals.clear();
	// PyDict_Clear(globals());
	namespaceInitialisation();
}

void 
LsysContext::namespaceInitialisation()
{
#ifndef Py_PYTHON_H
  #error Python headers needed to compile C extensions, please install development version of Python.
#endif
#if PY_VERSION_HEX < 0x03000000
   if (!hasObject("__builtins__"))
		setObjectToGlobals("__builtins__", object(handle<>(borrowed( PyModule_GetDict(PyImport_AddModule("__builtin__"))))));
#else
   if (!hasObject("__builtins__"))
		setObjectToGlobals("__builtins__", object(handle<>(borrowed( PyModule_GetDict(PyImport_AddModule("builtins")))))); // module __builtin__ was renamed to builtins in python3
#endif

   if (!hasObject("nproduce")){
	   Compilation::compile("from lpy import *",globals(),globals());

	   /* handle<>  lpymodule (borrowed( PyModule_GetDict(PyImport_AddModule("lpy"))));
		PyDict_Update(globals(),lpymodule.get());
		PyDict_DelItemString(globals(),"__file__");
		PyDict_DelItemString(globals(),"__doc__");
		PyDict_DelItemString(globals(),"__path__"); */

   }
}


std::string 
LsysContext::str() const {
  std::stringstream s;
  s << "<LsysContext instance at " << this << " with " << m_modules.size() << " declared modules>";
  return s.str();
}

void 
LsysContext::compile(const std::string& code) {
  ContextMaintainer c(this);
  Compilation::compile(code,globals(),locals().ptr());
}

object 
LsysContext::compile(const std::string& name, const std::string& code) {
  if(!code.empty()){
	  Compilation::compile(code,globals(),locals().ptr());
	return getObject(name);
  }
  return object();
}
object 
LsysContext::evaluate(const std::string& code) {
  ContextMaintainer c(this);
  if(!code.empty()){
	dict local_namespace;
	handle<> result(allow_null( 
	  PyRun_String(code.c_str(),Py_eval_input,globals(),local_namespace.ptr())));
	return object(result);
  }
  return object();
}


object 
LsysContext::try_evaluate(const std::string& code) {
  if(!code.empty()){
	dict local_namespace;
	handle<> result(allow_null( 
	  PyRun_String(code.c_str(),Py_eval_input,globals(),local_namespace.ptr())));
	if(result)return object(result);
	else {
	  PyErr_Clear();
	  return object();
	}
  }
  return object();
}



int
LsysContext::readInt(const std::string& code)  {
  std::string::const_iterator b = code.begin();
  while(b != code.end() && (*b == ' ' ||*b == '\t' ||*b == '\n'))b++;
  return extract<int>(evaluate(std::string(b,code.end())));
}

float
LsysContext::readReal(const std::string& code)  {
  std::string::const_iterator b = code.begin();
  while(b != code.end() && (*b == ' ' ||*b == '\t' ||*b == '\n'))b++;
  return extract<float>(evaluate(std::string(b,code.end())));
}


boost::python::object 
LsysContext::start(){
  return func("Start");
}

boost::python::object
LsysContext::start(AxialTree& lstring)
{ return controlMethod("Start",lstring); }


boost::python::object 
LsysContext::end(){
  return func("End");
}

boost::python::object
LsysContext::end(AxialTree& lstring)
{ return controlMethod("End",lstring); }

#ifndef LPY_NO_PLANTGL_INTERPRETATION
boost::python::object
LsysContext::end(AxialTree& lstring, const PGL::ScenePtr& scene)
{ return controlMethod("End",lstring,scene); }
#endif

boost::python::object  
LsysContext::startEach(){
  return func("StartEach");
}

boost::python::object  
LsysContext::startEach(AxialTree& lstring){
  return controlMethod("StartEach", lstring);
}

void
LsysContext::postDraw(){
  func("PostDraw");
}

boost::python::object 
LsysContext::endEach(){
  return func("EndEach");
}

boost::python::object
LsysContext::endEach(AxialTree& lstring)
{ return controlMethod("EndEach",lstring); }

#ifndef LPY_NO_PLANTGL_INTERPRETATION
boost::python::object
LsysContext::endEach(AxialTree& lstring, const PGL::ScenePtr& scene)
{ return controlMethod("EndEach",lstring,scene); }
#endif

AxialTree
LsysContext::startInterpretation(){
    if(hasStartInterpretationFunction()){
          func("StartInterpretation");
          AxialTree nprod = LsysContext::currentContext()->get_nproduction(); 
          if (nprod.empty())  {
            return AxialTree();
          }
          else { 
              LsysContext::currentContext()->reset_nproduction(); // to avoid deep copy
              return nprod;
          }
    }
    return AxialTree();
}


AxialTree
LsysContext::endInterpretation(){
    if(hasEndInterpretationFunction()){
          func("EndInterpretation");
          AxialTree nprod = LsysContext::currentContext()->get_nproduction(); 
          if (nprod.empty())  return AxialTree();
          else { 
              LsysContext::currentContext()->reset_nproduction(); // to avoid deep copy
              return nprod;
          }
    }
    return AxialTree();
}



boost::python::object
LsysContext::controlMethod(const std::string& name, AxialTree& lstring){
  ContextMaintainer c(this);
  if (hasObject(name)){
	// reference_existing_object::apply<AxialTree*>::type converter;
	// PyObject* obj = converter( &lstring );
	// object real_obj = object( handle<>( obj ) );
	return getObject(name)(object(lstring));
  }
  return object();
}

#ifndef LPY_NO_PLANTGL_INTERPRETATION
boost::python::object
LsysContext::controlMethod(const std::string& name, AxialTree& lstring, const PGL::ScenePtr& scene)
{
  ContextMaintainer c(this);
  if (hasObject(name)){
	// reference_existing_object::apply<AxialTree*>::type converter;
	// PyObject* obj = converter( &lstring );
	// object real_obj = object( handle<>( obj ) );
	return getObject(name)(lstring, scene); 
  }
  return object();
}
#endif


bool LsysContext::initialise()
{
  ContextMaintainer c(this);
  return m_initialise();
}

bool LsysContext::m_initialise()
{
  reference_existing_object::apply<LsysContext*>::type converter;
  PyObject* obj = converter( this );
  object real_obj = object( handle<>( obj ) );
  if (hasObject(InitialisationFunctionName)){
	  getObject(InitialisationFunctionName)(real_obj);
	  currentEvent();
	  return true;
  }
  else return false;
}


size_t LsysContext::m_initialiseFrom(const std::string& lcode)
{
	ContextMaintainer c(this);
	size_t pos = lcode.find(LpyParsing::InitialisationBeginTag);
	if (pos != std::string::npos) {
		compile(std::string(lcode.begin()+pos,lcode.end()));
		m_initialise();
		return pos;
	}
	return std::string::npos;
}

void 
LsysContext::setStart(object func){
  setObject("Start",func);
}

void 
LsysContext::setEnd(object func){
  setObject("End",func);
  check_init_functions();
}

void 
LsysContext::setStartEach(object func){
  setObject("StartEach",func);
}

void 
LsysContext::setEndEach(object func){
  setObject("EndEach",func);
  check_init_functions();
}

void 
LsysContext::setPostDraw(object func){
  setObject("PostDraw",func);
}

void 
LsysContext::setStartInterpretation(object func){
  setObject("StartInterpretation",func);
}

void 
LsysContext::setEndInterpretation(object func){
  setObject("EndInterpretation",func);
}

boost::python::object 
LsysContext::func(const std::string& funcname){
  ContextMaintainer c(this);
  if (hasObject(funcname))return getObject(funcname)();
  return object();
}

#define FUNC_CODE_ATTR "__code__"
#if PY_VERSION_HEX < 0x03000000
  #undef FUNC_CODE_ATTR
  #define FUNC_CODE_ATTR "func_code"
#endif

void 
LsysContext::check_init_functions()
{
	if (hasObject("StartEach")) {
		try {
			m_nbargs_of_starteach = extract<size_t>(getObject("StartEach").attr(FUNC_CODE_ATTR).attr("co_argcount"))();
		}
		catch (...) { PyErr_Clear(); m_nbargs_of_starteach = 0; }
	}
	else m_nbargs_of_starteach = 0;

	if (hasObject("Start")) {
		try {
			m_nbargs_of_start = extract<size_t>(getObject("Start").attr(FUNC_CODE_ATTR).attr("co_argcount"))();
		}
		catch (...) { PyErr_Clear(); m_nbargs_of_start = 0; }
	}
	else m_nbargs_of_start = 0;

	if (hasObject("EndEach")) {
		try {
			m_nbargs_of_endeach = extract<size_t>(getObject("EndEach").attr(FUNC_CODE_ATTR).attr("co_argcount"))();
		}
		catch (...) { PyErr_Clear(); m_nbargs_of_endeach = 0; }
	}
	else m_nbargs_of_endeach = 0;
	if (hasObject("End")) {
		try {
			m_nbargs_of_end = extract<size_t>(getObject("End").attr(FUNC_CODE_ATTR).attr("co_argcount"))();
		}
		catch (...) { PyErr_Clear(); m_nbargs_of_end = 0; }
	}
	else m_nbargs_of_end = 0;
}

/*---------------------------------------------------------------------------*/


size_t LsysContext::getIterationNb()
{
    QReadLocker ml(&m_iteration_nb_lock);
	return m_iteration_nb;
}

void LsysContext::setIterationNb(size_t val)
{
    QWriteLocker ml(&m_iteration_nb_lock);
    m_iteration_nb = val; 
}

/*---------------------------------------------------------------------------*/

double 
LsysContext::get_animation_timestep()
{
    QReadLocker ml(&m_animation_step_mutex);
    return m_animation_step;
}

void 
LsysContext::set_animation_timestep(double value)
{
    QWriteLocker ml(&m_animation_step_mutex);
    m_animation_step = value;
}

bool 
LsysContext::is_animation_timestep_to_default()
{
    QReadLocker ml(&m_animation_step_mutex);
    return fabs(m_animation_step - DefaultAnimationTimeStep) < GEOM_EPSILON;
}

/*---------------------------------------------------------------------------*/

bool 
LsysContext::isSelectionAlwaysRequired() const
{ return m_selection_always_required; }

void 
LsysContext::setSelectionAlwaysRequired(bool enabled)
{ 
	if (m_selection_always_required != enabled){
		m_selection_always_required = enabled; 
		options.setSelection("Selection Always Required",(size_t)enabled);
	}
}

/*---------------------------------------------------------------------------*/

void 
LsysContext::setWarnWithSharpModule(bool enabled)
{ 
	if (m_warn_with_sharp_module != enabled){
		m_warn_with_sharp_module = enabled; 
		options.setSelection("Warning with sharp module",(size_t)m_warn_with_sharp_module);
	}
}

/*---------------------------------------------------------------------------*/

void LsysContext::enableEarlyReturn(bool val) 
{ 
    QWriteLocker ml(&m_early_return_mutex);
    m_early_return = val; 
}

bool LsysContext::isEarlyReturnEnabled() 
{ 
    QReadLocker ml(&m_early_return_mutex);
    return m_early_return; 
}

/*---------------------------------------------------------------------------*/

bool LsysContext::pInLeftContext(size_t pid, boost::python::dict& res)
{ 
	if (is_null_ptr(m_lstringmatcher)) LsysError("Cannot call InLeftContext");
	return m_lstringmatcher->pInLeftContext(pid, res);
}

bool LsysContext::inLeftContext(const PatternString& pattern, boost::python::dict& res)
{ 
	if (is_null_ptr(m_lstringmatcher)) LsysError("Cannot call inLeftContext");
	return m_lstringmatcher->inLeftContext(pattern, res);
}


bool LsysContext::pInRightContext(size_t pid, boost::python::dict& res)
{ 
	if (is_null_ptr(m_lstringmatcher)) LsysError("Cannot call InRightContext");
	return m_lstringmatcher->pInRightContext(pid, res);
}

bool LsysContext::inRightContext(const PatternString& pattern, boost::python::dict& res)
{ 
	if (is_null_ptr(m_lstringmatcher)) LsysError("Cannot call inRightContext");
	return m_lstringmatcher->inRightContext(pattern, res);
}


/*---------------------------------------------------------------------------*/

LocalContext::LocalContext(const boost::python::dict& globals):
  LsysContext(globals),
  m_globals(globals)
{ 
   namespaceInitialisation();
}

LocalContext::LocalContext(const boost::python::dict& locals, const boost::python::dict& globals):
  LsysContext(locals),
  m_globals(globals)
{ 
   namespaceInitialisation();
}

LocalContext::~LocalContext()
{
	if(isCurrent()) done();
	if (!LSYSCONTEXT_STACK.empty()){
		std::vector<LsysContext *>::iterator it = LSYSCONTEXT_STACK.begin();
		for(;it!=LSYSCONTEXT_STACK.end();++it){
			if (*it == this){
				if(it == LSYSCONTEXT_STACK.begin()){
					LSYSCONTEXT_STACK.erase(it);
					it = LSYSCONTEXT_STACK.begin();
				}
				else {
					std::vector<LsysContext *>::iterator it2 = it+1;
					LSYSCONTEXT_STACK.erase(it);
					it = it2;
				}
			}
		}
	}
}


void
LocalContext::clearNamespace() {
  m_locals.clear();
  namespaceInitialisation();
}


PyObject * 
LocalContext::globals() const 
{ return m_globals.ptr(); }

/*---------------------------------------------------------------------------*/

GlobalContext::GlobalContext():
  LsysContext(){
    m_globals = handle<>(borrowed( PyModule_GetDict(PyImport_AddModule("__main__"))));
}

GlobalContext::~GlobalContext()
{

	if(!(LSYSCONTEXT_STACK.empty() && isCurrent()))
		while(!isCurrent()) currentContext()->done();
	assert(LSYSCONTEXT_STACK.empty() && isCurrent() && "LsysContext not all done!");
}


PyObject * 
GlobalContext::globals()  const 
{ return m_globals.get(); }


boost::python::object GlobalContext::m_reprFunc;

boost::python::object bprepr(boost::python::object obj)
{
    return boost::python::object(boost::python::handle<>(PyObject_Repr(obj.ptr())));
}

boost::python::object 
GlobalContext::getFunctionRepr() {
	if(m_reprFunc == boost::python::object()){
		m_reprFunc =  boost::python::make_function(bprepr);
    }
	return m_reprFunc;
}


/*---------------------------------------------------------------------------*/

