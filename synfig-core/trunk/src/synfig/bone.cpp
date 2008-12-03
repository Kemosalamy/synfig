/* === S Y N F I G ========================================================= */
/*!	\file bone.cpp
**	\brief Bone File
**
**	$Id$
**
**	\legal
**	Copyright (c) 2002-2005 Robert B. Quattlebaum Jr., Adrian Bentley
**
**	This package is free software; you can redistribute it and/or
**	modify it under the terms of the GNU General Public License as
**	published by the Free Software Foundation; either version 2 of
**	the License, or (at your option) any later version.
**
**	This package is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
**	General Public License for more details.
**	\endlegal
*/
/* ========================================================================= */

/* === H E A D E R S ======================================================= */

#ifdef USING_PCH
#	include "pch.h"
#else
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "bone.h"
#include "guid.h"
#include "valuenode_bone.h"
#include <ETL/stringf>
#endif

/* === U S I N G =========================================================== */

using namespace std;
using namespace etl;
using namespace synfig;

/* === M A C R O S ========================================================= */

/* === G L O B A L S ======================================================= */

/* === P R O C E D U R E S ================================================= */

//! Default constructor
Bone::Bone():
	origin_(Point(0,0)),
	origin0_(Point(0,0)),
	angle_(Angle::deg(0.0)),
	angle0_(Angle::deg(0.0)),
	scale_(1.0),
	length_(1.0),
	strength_(1.0),
	parent_(0)
{
	if (getenv("SYNFIG_DEBUG_NEW_BONES"))
		printf("%s:%d new bone\n", __FILE__, __LINE__);
}

//!Constructor by origin and tip
Bone::Bone(const Point &o, const Point &t):
	origin_(o),
	origin0_(o),
	angle_((t-o).angle()),
	angle0_((t-o).angle()),
	scale_(1.0),
	length_(1.0),
	strength_(1.0),
	parent_(0)
{
	if (getenv("SYNFIG_DEBUG_NEW_BONES"))
		printf("%s:%d new bone\n", __FILE__, __LINE__);
}

//!Constructor by origin, angle, length, strength, parent bone (default = no parent)
Bone::Bone(const String &n, const Point &o, const Angle &a, const Real &l, const Real &s, GUID p):
	name_(n),
	origin_(o),
	origin0_(o),
	angle_(a),
	angle0_(a),
	scale_(1.0),
	length_(l),
	strength_(s),
	parent_(p)
{
	if (getenv("SYNFIG_DEBUG_NEW_BONES"))
		printf("%s:%d new bone\n", __FILE__, __LINE__);
}

GUID
Bone::get_parent()const
{
	return parent_;
}

void
Bone::set_parent(const GUID g)
{
	parent_ = g;
}

//! get_tip() member function
//!@return The tip Point of the bone (calculated) based on
//! tip=origin+[length,0]*Scale(scale,0)*Rotate(alpha)
Point
Bone::get_tip()
{
	Matrix s, r, sr;
	s.set_scale(scale_,0);
	r.set_rotate(angle_);
	sr=s*r;
	return (Point)sr.get_transformed(Vector(length_,0));
}

//!Setup Transformation matrix.
//!This matrix applied to a setup point in global
//!coordinates calculates the local coordinates of
//!the point relative to the current bone.
Matrix
Bone::get_setup_matrix()const
{
	Matrix t,r,bparent;
	t.set_translate((Vector)(-origin0_));
	r.set_rotate(-angle0_);
	bparent=t*r;
#if 0
	Bone const *currparent=parent_;
	while (currparent)
	{
		bparent*=currparent->get_setup_matrix();
		currparent=currparent->parent_;
	}
#endif
	return bparent;
}

//!Animated Transformation matrix.
//!This matrix applied to a setup point in local
//!coordinates (the one obtained form the Setup
//!Transformation matrix) would obtain the
//!animated position of the point due the current
//!bone influence
Matrix
Bone::get_animated_matrix() const
{
	Matrix s,r,t,banimated;
	banimated.set_identity();
	banimated*=s.set_scale(scale_)*r.set_rotate(angle_)*t.set_translate(origin_);
#if 0
	if(parent_)
	{
		return parent_->get_animated_matrix()*banimated;
	}
	else
#endif
		return banimated;

}
//!Get the string of the Bone
//!@return String type. A string representation of the bone
//!components.
synfig::String
Bone::get_string()const
{
	return strprintf("N=%s O=(%.4f %.4f) O0=(%.4f %.4f) a=%.4f a0=%.4f s=%.4f l=%.4f S=%.4f P=%s",
					 name_.c_str(),
					 origin_[0], origin_[1],
					 origin0_[0], origin0_[1],
					 Angle::deg(angle_).get(),
					 Angle::deg(angle0_).get(),
					 scale_, length_, strength_, parent_.get_string().substr(0,GUID_PREFIX_LEN).c_str());
}

bool
Bone::is_root()
{
	return get_parent();
}

/* === M E T H O D S ======================================================= */

/* === E N T R Y P O I N T ================================================= */
