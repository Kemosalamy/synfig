/* === S I N F G =========================================================== */
/*!	\file valuenode_timedswap.h
**	\brief Template Header
**
**	$Id: valuenode_timedswap.h,v 1.1.1.1 2005/01/04 01:23:15 darco Exp $
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

#ifndef __SINFG_VALUENODE_TIMEDSWAP_H
#define __SINFG_VALUENODE_TIMEDSWAP_H

/* === H E A D E R S ======================================================= */

#include "valuenode.h"

/* === M A C R O S ========================================================= */

/* === C L A S S E S & S T R U C T S ======================================= */

namespace sinfg {

struct ValueNode_TimedSwap : public LinkableValueNode
{
	typedef etl::handle<ValueNode_TimedSwap> Handle;
	typedef etl::handle<const ValueNode_TimedSwap> ConstHandle;
	
private:

	ValueNode::RHandle before;
	ValueNode::RHandle after;
	ValueNode::RHandle swap_time;
	ValueNode::RHandle swap_length;

	ValueNode_TimedSwap(ValueBase::Type id);

public:

//	static Handle create(ValueBase::Type id);

	virtual ~ValueNode_TimedSwap();

	bool set_before(const ValueNode::Handle &a);
	ValueNode::Handle get_before()const;
	bool set_after(const ValueNode::Handle &a);
	ValueNode::Handle get_after()const;

	void set_swap_time_real(Time x);
	bool set_swap_time(const ValueNode::Handle &x);
	ValueNode::Handle get_swap_time()const;

	void set_swap_length_real(Time x);
	bool set_swap_length(const ValueNode::Handle &x);
	ValueNode::Handle get_swap_length()const;


	virtual bool set_link_vfunc(int i,ValueNode::Handle x);
	virtual ValueNode::LooseHandle get_link_vfunc(int i)const;
	virtual int link_count()const;
	virtual String link_local_name(int i)const;
	virtual String link_name(int i)const;
	virtual int get_link_index_from_name(const String &name)const;

	virtual ValueBase operator()(Time t)const;

	virtual String get_name()const;	
	virtual String get_local_name()const;
//	static bool check_type(const ValueBase::Type &type);

protected:
	
	virtual LinkableValueNode* create_new()const;

public:
	using sinfg::LinkableValueNode::get_link_vfunc;
	using sinfg::LinkableValueNode::set_link_vfunc;
	static bool check_type(ValueBase::Type type);
	static ValueNode_TimedSwap* create(const ValueBase &x);
}; // END of class ValueNode_TimedSwap

}; // END of namespace sinfg

/* === E N D =============================================================== */

#endif
