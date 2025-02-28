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

#pragma once

#include "error.h"
#include <vector>
#include "../plantgl/tool/util_string.h"
#include "../plantgl/math/util_vector.h"

#include "moduleclass.h"
#include "argcollector.h"
#include <QtCore/QSharedData>

LPY_BEGIN_NAMESPACE

/*---------------------------------------------------------------------------*/

class LPY_API Module {
public:
   friend class MatchingEngine;
   friend class MatchingEngineImplementation;

  Module();
  Module(const std::string& c);
  Module(size_t classid);
  Module(const ModuleClassPtr m);
  Module(const Module&);

  virtual ~Module();

  inline const std::string& name() const {  return m_mclass->name; }
  inline void setName(const std::string& c) { m_mclass = ModuleClassTable::get().getClass(c); }
  inline bool sameName(const Module& m) const { return m_mclass == m.m_mclass;  }

  inline ModuleClassPtr getClass() const { lpyassert(m_mclass != NULL); return m_mclass; }
  inline size_t getClassId() const { return getClass()->getId(); }
  inline bool isinstance(const ModuleClassPtr& mclass) const 
  { return m_mclass->issubclass(mclass) ;  }

  virtual std::string str() const;
  virtual std::string repr() const;
  inline bool operator==(const Module& n) const { return sameName(n); }
  inline bool operator!=(const Module& other) const { return !operator==(other); }

  inline bool isLeftBracket() const {  return m_mclass == ModuleClass::LeftBracket; }
  inline bool isRightBracket() const {  return m_mclass == ModuleClass::RightBracket; }
  inline bool isExactRightBracket() const { return m_mclass == ModuleClass::ExactRightBracket; }
  inline bool isBracket() const { return isLeftBracket() || isRightBracket() || isExactRightBracket(); }
  inline bool isRequest() const { 
		return m_mclass == ModuleClass::QueryPosition || 
			   m_mclass == ModuleClass::QueryHeading ||
			   m_mclass == ModuleClass::QueryUp || 
			   m_mclass == ModuleClass::QueryLeft || 
		       m_mclass == ModuleClass::QueryRigth || 
		       m_mclass == ModuleClass::QueryFrame; }
  inline bool isCut() const { return m_mclass == ModuleClass::Cut; }
  inline bool isNone() const { return m_mclass == ModuleClass::None; }
  inline bool isNull() const { return m_mclass == ModuleClass::None || m_mclass == ModuleClass::Star; }
  inline bool isStar() const { return m_mclass == ModuleClass::Star; }
  inline bool isRepExp() const { return m_mclass == ModuleClass::RepExp; }
  inline bool isOr() const { return m_mclass == ModuleClass::Or; }
  inline bool isRE() const { return isRepExp() || isOr(); }
  inline bool isGetIterator() const { return m_mclass == ModuleClass::GetIterator; }
  inline bool isGetModule() const { return m_mclass == ModuleClass::GetModule; }
  bool isConsidered() const;
  bool isIgnored() const;

  inline int scale() const { return m_mclass->getScale(); }
protected:
  inline void setClass(size_t cid) { m_mclass = ModuleClassTable::get().find(cid); }
private:
  ModuleClassPtr m_mclass;
};

/*---------------------------------------------------------------------------*/

LPY_API extern boost::python::object getFunctionRepr();

template<class Parameter>
class AbstractParamModule : public Module {
public:
	typedef Parameter ParameterType;
	typedef std::vector<ParameterType> ParameterList;
	typedef AbstractParamModule<ParameterType> BaseType;
	typedef typename ParameterList::iterator iterator;
	typedef typename ParameterList::const_iterator const_iterator;
	typedef typename ParameterList::reverse_iterator reverse_iterator;
	typedef typename ParameterList::const_reverse_iterator const_reverse_iterator;

	AbstractParamModule() : m_argholder(new ParamModuleInternal()) { }
	AbstractParamModule(const std::string& name) : Module(name), m_argholder(new ParamModuleInternal()) {}
    AbstractParamModule(size_t classid) : Module(classid), m_argholder(new ParamModuleInternal()) {}
    AbstractParamModule(const ModuleClassPtr mclass) : Module(mclass), m_argholder(new ParamModuleInternal()) {}

	~AbstractParamModule() { }

	inline size_t size() const { return getConstargs().size(); }
	inline bool empty() const  { return getConstargs().empty(); }

	/*attribute_deprecated*/ inline size_t argSize() const { return getConstargs().size(); }
	/*attribute_deprecated*/ inline bool hasArg() const  { return !getConstargs().empty(); }

	inline boost::python::list getPyArgs() const 
	{ return toPyList(getConstargs()); }
	
	inline void setPyArgs(const boost::python::list& l) 
	{ getArgs() = toParameterList(l); }

    inline const ParameterType& getAt(size_t i) const 
	{ lpyassert(size() > i); return getConstargs()[i]; }

    inline ParameterType& getAt(size_t i) 
	{ lpyassert(size() > i); return getArgs()[i]; }

    inline void setAt(size_t i, const ParameterType& value) 
	{ lpyassert(size() > i); getArgs()[i] = value; }

	inline void delAt(size_t i) 
	{ lpyassert(size() > i); getArgs().erase(getArgs().begin()+i); }

    inline ParameterType getItemAt(int i) const 
	{ return getAt(getValidIndex(i));  }

    inline void setItemAt(int i, const ParameterType& value)
	{ setAt(getValidIndex(i),value);  }

    inline void delItemAt(int i) 
	{ delAt(getValidIndex(i));  }

	boost::python::list getSlice(size_t i, size_t j) const {
		lpyassert( i <= j && j <= size() );
		boost::python::list res;
		for(const_iterator it = getConstargs().begin()+i; 
			it != getConstargs().begin()+j; ++it) res.append(*it);
		return res;
	}

	inline void delSlice(size_t i, size_t j) 
	{ lpyassert( i <= j && j <= size() );
	  getArgs().erase(getArgs().begin()+i,getArgs().begin()+j); }

    inline boost::python::list getSliceItemAt(int i, int j) const 
	{ size_t ri, rj; getValidIndices(i,j,ri,rj) ; return getSlice(ri,rj);  }

    inline void delSliceItemAt(int i, int j)
	{ size_t ri, rj; getValidIndices(i,j,ri,rj) ; return delSlice(ri,rj);  }

	inline void append(const ParameterType& value)  { getArgs().push_back(value); }
	inline void prepend(const ParameterType& value) { getArgs().insert(getArgs().begin(), value); }

	inline bool operator==(const BaseType& other) const 
	{ return (sameName(other) && (m_argholder == other.m_argholder || getConstargs() == other.getConstargs())); }

    inline bool operator!=(const BaseType& other) const { return !operator==(other); }

	virtual std::string str() const {
		std::string st = name();
		if(!empty())st +='('+strArg()+')'; 
		return st;
	}

    virtual std::string repr() const {
		std::string st = name();
		if(!empty())st +='('+reprArg()+')'; 
		return st;
	}

	inline boost::python::tuple toTuple() const {
		boost::python::tuple res(name());
		res += boost::python::tuple(toPyList(getConstargs()));
		return res;
	}

	std::string strArg() const 
	{ 
	  boost::python::str res(",");
	  boost::python::list argstr;
	  for (const_iterator it = getConstargs().begin(); it != getConstargs().end(); ++it)
			argstr.append(boost::python::str(*it));
      return boost::python::extract<std::string>(res.join(argstr));
    }

    std::string reprArg() const
	{ 
	  boost::python::str res(",");
	  boost::python::list argstr;
      boost::python::object repr = getFunctionRepr();
	  for (const_iterator it = getConstargs().begin(); it != getConstargs().end(); ++it)
			argstr.append(repr(*it));
      return boost::python::extract<std::string>(res.join(argstr));
    }

    inline std::vector<std::string> getParameterNames() const
    { return getClass()->getParameterNames(); }

	inline size_t getNamedParameterNb() const
    { return getClass()->getNamedParameterNb(); }

    inline size_t getParameterPosition(const std::string& pname) const  
    { 
        int firstnamedparameter = std::max<int>(0,size() - getNamedParameterNb());
        return firstnamedparameter + getClass()->getParameterPosition(pname); 
    }

    inline bool hasParameter(const std::string& pname) const  
    { return getClass()->hasParameter(pname); }

    inline ParameterType getParameter(const std::string& pname) const
	{ return getAt(getValidParameterPosition(pname)); }

    inline void setParameter(const std::string& pname, const ParameterType& value)
	{ return setAt(getValidParameterPosition(pname),value); }

    inline const ParameterList& getParameterList() const { return getConstargs(); }

	void getNamedParameters(boost::python::dict& parameters, size_t fromIndex = 0) const
	{
        int firstnamedparameter = std::max<int>(0,size() - getNamedParameterNb());
		const ParameterNameDict& pnames = getClass()->getParameterNameDict();
		for(ParameterNameDict::const_iterator itp = pnames.begin(); itp != pnames.end(); ++itp){
			if(itp->second >= fromIndex) {
				parameters[boost::python::object(itp->first)] = getAt(firstnamedparameter+itp->second);
			}
		}
	}

protected:
     inline size_t getValidParameterPosition(const std::string& pname) const
	 {
		size_t pos = getParameterPosition(pname);
		if (pos == ModuleClass::NOPOS) LsysError("Invalid parameter name '"+pname+"' for module '"+name()+"'.");
		if (pos >= size()) LsysError("Inexisting value for parameter '"+pname+" '("+TOOLS(number)(pos)+") for module '"+name()+"'.");
		return pos;
	 }

	 inline size_t getValidIndex(int i) const {
		size_t s = size();
		if( i < 0 ) i += s;
		if (i < 0  || i >= s) throw PythonExc_IndexError("index out of range");
		return (size_t)i;
	 }

	 inline void getValidIndices(int i, int j, size_t& resi, size_t& resj) const {
		size_t s = size();
		if( i < 0 ) i += s;
		if( j < 0 ) j += s;
	    if( j > s ) j = s;
		if (i < 0  || i >= s || j < i) throw PythonExc_IndexError("index out of range");
		resi =(size_t)i;
		resj =(size_t)j;
	 }

 template<class PModule>
 struct LPY_API ParamListInternal 
     : public QSharedData 
 {
	 typedef PModule ParamModule;
	 typedef typename ParamModule::ParameterList ParameterList;

	 ParamListInternal() : QSharedData() {}
	 ParamListInternal(const ParamListInternal& other) : QSharedData(), m_args(other.m_args) { }
	 ~ParamListInternal() {}

     ParameterList m_args;
 };
 
 typedef ParamListInternal<BaseType> ParamModuleInternal;
 typedef QSharedDataPointer<ParamModuleInternal> ParamModuleInternalPtr;

  ParamModuleInternalPtr m_argholder;

  inline ParameterList& getArgs() { return m_argholder->m_args; }
  inline const ParameterList& getArgs() const { return m_argholder->m_args; }
  inline const ParameterList& getConstargs() const { return m_argholder->m_args; }

  static inline boost::python::list toPyList(const ParameterList& pl) {
	boost::python::list result;
	for(const_iterator it = pl.begin(); it != pl.end(); ++it)
      result.append(*it);
    return result;
  }

  static inline ParameterList toParameterList(const boost::python::object& t){
	ParameterList result;
	boost::python::object iter_obj = boost::python::object( boost::python::handle<>( PyObject_GetIter( t.ptr() ) ) );
	try {  while( 1 ) result.push_back(boost::python::extract<ParameterType>(iter_obj.attr( "next" )())()); }
    catch( boost::python::error_already_set ){ PyErr_Clear(); }
   return result;
 }

};

/*---------------------------------------------------------------------------*/

class PatternModule;

/*---------------------------------------------------------------------------*/

class LPY_API ParamModule : public AbstractParamModule<boost::python::object> {
public:
  friend class ParametricProduction;

  typedef boost::python::object Parameter;
  typedef AbstractParamModule<Parameter> BaseType;

  ParamModule(const std::string& name);
  ParamModule(const ParamModule& name);
  ParamModule(size_t classid);
  ParamModule(size_t classid, const std::string& args);

  ParamModule(boost::python::tuple t);
  ParamModule(boost::python::list t);

  ParamModule(const std::string& name, 
			  const boost::python::list& args);
  ParamModule(size_t classid, 
              const boost::python::list& args);

  ParamModule(const ModuleClassPtr m, 
              const boost::python::tuple& args);

  ParamModule(const std::string& name, 
			  const boost::python::object& args);

  ParamModule(const std::string& name, 
			  const boost::python::object&,
			  const boost::python::object&);
  ParamModule(const std::string& name, 
			  const boost::python::object&,
			  const boost::python::object&,
			  const boost::python::object&);

  ParamModule(const std::string& name, 
			  const boost::python::object&,
			  const boost::python::object&,
			  const boost::python::object&,
			  const boost::python::object&);

  ParamModule(const std::string& name, 
			  const boost::python::object&,
			  const boost::python::object&,
			  const boost::python::object&,
			  const boost::python::object&,
			  const boost::python::object&);

  virtual ~ParamModule();

  int  _getInt(int)   const;
  real_t _getReal(int) const;
  bool _getBool(int) const;
  std::string _getString(int) const;

  template<class T>
  T _get(int i) const { return boost::python::extract<T>(getAt(i))(); }

  template<class T>
  bool _check(int i) const { return boost::python::extract<T>(getAt(i)).check(); }


  virtual void _setValues(real_t,real_t,real_t) ;
  inline void _setValues(const TOOLS(Vector3)& v) { _setValues(v.x(),v.y(),v.z()); }
  virtual void _setFrameValues(const TOOLS(Vector3)& p, const TOOLS(Vector3)& h, const TOOLS(Vector3)& u, const TOOLS(Vector3)& l) ;

  bool match(const PatternModule&m) const;
  bool match(const PatternModule&m, ArgList&) const;
  bool match(const std::string&, size_t nbargs) const;

#ifndef LPY_NO_PLANTGL_INTERPRETATION
  inline void interpret(PGL::Turtle& t) { getClass()->interpret(*this,t); }
#endif

  inline bool operator==(const ParamModule& other) const 
  { return (sameName(other) && (m_argholder == other.m_argholder || getConstargs() == other.getConstargs())); }
  inline bool operator==(const std::string& other) const 
  { return other == name();}

  inline bool operator!=(const ParamModule& other) const { return !operator==(other); }
  inline bool operator!=(const std::string& other) const 
  { return other != name();}

  void appendArgumentList(const boost::python::object& arglist) ;


protected:
  ParamModule();

};

/*---------------------------------------------------------------------------*/

inline bool is_lower_scale(int scale1, int scale2) { return scale1 < scale2; }
inline bool is_eq_scale(int scale1, int scale2)    { return scale1 == scale2; }
inline bool is_neq_scale(int scale1, int scale2)    { return scale1 != scale2; }
inline bool is_upper_scale(int scale1, int scale2) { return scale1 > scale2; }
inline int up_scale(int scale1) { return scale1 + 1; }
inline int down_scale(int scale1) { return scale1 - 1; }
// See Also ModuleClass::DEFAULT_SCALE

/*---------------------------------------------------------------------------*/

LPY_END_NAMESPACE
