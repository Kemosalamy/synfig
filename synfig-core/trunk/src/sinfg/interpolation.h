/* === S I N F G =========================================================== */
/*!	\file interpolation.h
**	\brief Template Header
**
**	$Id: interpolation.h,v 1.1.1.1 2005/01/04 01:23:14 darco Exp $
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

#ifndef __SINFG_INTERPOLATION_H
#define __SINFG_INTERPOLATION_H

/* === H E A D E R S ======================================================= */

/* === M A C R O S ========================================================= */

/* === T Y P E D E F S ===================================================== */

/* === C L A S S E S & S T R U C T S ======================================= */

namespace sinfg { 

enum Interpolation
{
	INTERPOLATION_TCB,
	INTERPOLATION_CONSTANT,
	INTERPOLATION_LINEAR,
	INTERPOLATION_HALT,
	INTERPOLATION_MANUAL,
	INTERPOLATION_UNDEFINED,
	INTERPOLATION_NIL
}; // END enum Interpolation

}; // END of namespace sinfg

/* === E N D =============================================================== */

#endif
