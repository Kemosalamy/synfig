/* === S I N F G =========================================================== */
/*!	\file module.h
**	\brief writeme
**
**	$Id: module.h,v 1.1.1.1 2005/01/04 01:23:14 darco Exp $
**
**	\legal
**	Copyright (c) 2002 Robert B. Quattlebaum Jr.
**
**	This software and associated documentation
**	are CONFIDENTIAL and PROPRIETARY property of
**	the above-mentioned copyright holder.
**
**	You may not copy, print, publish, or in any
**	other way distribute this software without
**	a prior written agreement with
**	the copyright holder.
**	\endlegal
*/
/* ========================================================================= */

/* === S T A R T =========================================================== */

#ifndef __SINFG_MODULE_H
#define __SINFG_MODULE_H

/* === H E A D E R S ======================================================= */

#include "general.h"
#include <ETL/handle>
#include <map>
#include "string.h"
#include <utility>
#include "vector.h"
#include "color.h"
#include "layer.h"
#include "canvas.h"

//#include "value.h"

/* === M A C R O S ========================================================= */

//! Marks the start of a module description
#define MODULE_DESC_BEGIN(x) struct x##_modclass : public sinfg::Module { x##_modclass(sinfg::ProgressCallback *callback=NULL); 

//! Sets the localized name of the module
#define MODULE_NAME(x) 			virtual const char * Name() { return x; }

//! Sets a localized description of the module
#define MODULE_DESCRIPTION(x)	virtual const char * Desc() { return x; }

//! Sets the name of the module's author
#define MODULE_AUTHOR(x)		virtual const char * Author() { return x; }

//! Sets the version string for the module
#define MODULE_VERSION(x)		virtual const char * Version() { return x; }

//! Sets the copyright string for the module
#define MODULE_COPYRIGHT(x)		virtual const char * Copyright() { return x; }

//! Describes the module's construction function
#define MODULE_CONSTRUCTOR(x)	bool constructor_(sinfg::ProgressCallback *cb) { return x(cb); }

//! Describes the module's destruction function
#define MODULE_DESTRUCTOR(x)	virtual void destructor_() { return x(); }

//! Marks the end of a module description
#define MODULE_DESC_END };

//#if 0
#ifdef __APPLE__
//! Marks the start of a module's inventory
#define MODULE_INVENTORY_BEGIN(x)  extern "C" {		\
	sinfg::Module* _##x##_LTX_new_instance(sinfg::ProgressCallback *cb) \
	{ if(SINFG_CHECK_VERSION()){x##_modclass *mod=new x##_modclass(cb); mod->constructor_(cb); return mod; }\
	if(cb)cb->error(#x": Unable to load module due to version mismatch."); return NULL; } \
	}; x##_modclass::x##_modclass(sinfg::ProgressCallback *cb) { 
#else
//! Marks the start of a module's inventory
#define MODULE_INVENTORY_BEGIN(x)  extern "C" {		\
	sinfg::Module* x##_LTX_new_instance(sinfg::ProgressCallback *cb) \
	{ if(SINFG_CHECK_VERSION()){x##_modclass *mod=new x##_modclass(cb); mod->constructor_(cb); return mod; }\
	if(cb)cb->error(#x": Unable to load module due to version mismatch."); return NULL; } \
	}; x##_modclass::x##_modclass(sinfg::ProgressCallback *cb) { 
#endif

//! Marks the start of the layers in the module's inventory
#define BEGIN_LAYERS {

//! DEPRECATED - use @INCLUDE_LAYER()
//#define LAYER(x) sinfg::Layer::book()[sinfg::String(x::name__)]=x::create;
#define LAYER(class)  	sinfg::Layer::register_in_book(sinfg::Layer::BookEntry(class::create,class::name__,class::local_name__,class::category__,class::cvs_id__,class::version__));
#define LAYER_ALIAS(class,alias)  	sinfg::Layer::register_in_book(sinfg::Layer::BookEntry(class::create,alias,alias,_("Do Not Use"),class::cvs_id__,class::version__));

//! Marks the end of the layers in the module's inventory
#define END_LAYERS }

//! Marks the start of the targets in the module's inventory
#define BEGIN_TARGETS {

#define TARGET(x) sinfg::Target::book()[sinfg::String(x::name__)]=std::pair<sinfg::Target::Factory,sinfg::String>(x::create,sinfg::String(x::ext__));sinfg::Target::ext_book()[sinfg::String(x::ext__)]=x::name__;

#define TARGET_EXT(x,y) sinfg::Target::ext_book()[sinfg::String(y)]=x::name__;

//! Marks the end of the targets in the module's inventory
#define END_TARGETS }

//! Marks the start of the importers in the module's inventory
#define BEGIN_IMPORTERS {

#define IMPORTER(x) sinfg::Importer::book()[sinfg::String(x::ext__)]=x::create;

#define IMPORTER_EXT(x,y) sinfg::Importer::book()[sinfg::String(y)]=x::create;

//! Marks the end of the importers in the module's inventory
#define END_IMPORTERS }

//! Marks the end of a module's inventory
#define MODULE_INVENTORY_END	}

/* === T Y P E D E F S ===================================================== */

/* === C L A S S E S & S T R U C T S ======================================= */

namespace sinfg {

class ProgressCallback;

/*!	\class Module
**	\todo writeme
*/
class Module : public etl::shared_object
{
public:
	bool constructor_(sinfg::ProgressCallback *cb) { return true; }
	virtual void destructor_() { }
	
	typedef etl::handle<Module> Handle;
	typedef etl::loose_handle<Module> LooseHandle;
	typedef etl::handle<const Module> ConstHandle;
	
public:
	typedef Module*(*constructor_type)(ProgressCallback *);
	typedef std::map<String, Handle > Book;
private:
	static Book* book_;
public:
	static Book& book();

	static bool subsys_init(const String &prefix);
	static bool subsys_stop();
	static bool register_default_modules();

	static void Register(Handle mod);
	static bool Register(const String &module_name, ProgressCallback *cb=NULL);
	static inline void Register(Module *mod) { Register(Handle(mod)); }
	
	virtual const char * Name() { return " "; }
	virtual const char * Desc() { return " "; }
	virtual const char * Author() { return " "; }
	virtual const char * Version() { return " "; }
	virtual const char * Copyright() { return SINFG_COPYRIGHT; }

	virtual ~Module() { destructor_(); }
};

}; // END of namespace sinfg

/* === E N D =============================================================== */

#endif
