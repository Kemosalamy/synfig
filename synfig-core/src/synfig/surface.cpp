/* === S Y N F I G ========================================================= */
/*!	\file surface.cpp
**	\brief Template File
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
**
** === N O T E S ===========================================================
**
** ========================================================================= */

/* === H E A D E R S ======================================================= */

#ifdef USING_PCH
#	include "pch.h"
#else
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "canvas.h"
#include "surface.h"
#include "target_scanline.h"
#include "target_cairo.h"
#include "general.h"

#ifdef HAS_VIMAGE
#include <Accelerate/Accelerate.h>
#endif

#endif

using namespace synfig;
using namespace std;
using namespace etl;

/* === M A C R O S ========================================================= */

/* === G L O B A L S ======================================================= */

class target2surface : public synfig::Target_Scanline
{
public:
	Surface *surface;
	bool	 sized;
public:
	target2surface(Surface *surface);
	virtual ~target2surface();

	virtual bool set_rend_desc(synfig::RendDesc *newdesc);

	virtual bool start_frame(synfig::ProgressCallback *cb);

	virtual void end_frame();

	virtual Color * start_scanline(int scanline);

	virtual bool end_scanline();
};

target2surface::target2surface(Surface *surface):surface(surface)
{
}

target2surface::~target2surface()
{
}

bool
target2surface::set_rend_desc(synfig::RendDesc *newdesc)
{
	assert(newdesc);
	assert(surface);
	desc=*newdesc;
	return synfig::Target_Scanline::set_rend_desc(newdesc);
}

bool
target2surface::start_frame(synfig::ProgressCallback */*cb*/)
{
	if(surface->get_w() != desc.get_w() || surface->get_h() != desc.get_h())
	{
		surface->set_wh(desc.get_w(),desc.get_h());
	}
	return true;
}

void
target2surface::end_frame()
{
	return;
}

Color *
target2surface::start_scanline(int scanline)
{
	return (*surface)[scanline];
}

bool
target2surface::end_scanline()
{
	return true;
}

class target2cairosurface: public synfig::Target_Cairo
{ 

	CairoSurface * surface_;
public:
	target2cairosurface(CairoSurface *surface);
	virtual ~target2cairosurface();
	
	virtual bool set_rend_desc(synfig::RendDesc *newdesc);
	
	virtual bool start_frame(synfig::ProgressCallback *cb);
	
	virtual void end_frame();

	virtual CairoSurface* obtain_surface();
};

target2cairosurface::target2cairosurface(CairoSurface *surface):surface_(surface)
{
}

target2cairosurface::~target2cairosurface()
{
}

bool
target2cairosurface::set_rend_desc(synfig::RendDesc *newdesc)
{
	assert(newdesc);
	desc=*newdesc;
	return synfig::Target_Cairo::set_rend_desc(newdesc);
}


CairoSurface*
target2cairosurface::obtain_surface()
{
	return surface_;
}

bool
target2cairosurface::start_frame(synfig::ProgressCallback */*cb*/)
{

	if(!surface_->map_cairo_image())
		return false;
	if(surface_->get_w() != desc.get_w() || surface_->get_h() != desc.get_h())
		return false;
	return true;
}

void
target2cairosurface::end_frame()
{
	surface_->unmap_cairo_image();
}
/* === P R O C E D U R E S ================================================= */

/* === M E T H O D S ======================================================= */

Target_Scanline::Handle
synfig::surface_target(Surface *surface)
{
	return Target_Scanline::Handle(new target2surface(surface));
}

Target_Cairo::Handle
synfig::cairosurface_target(CairoSurface *surface)
{
	if(surface->get_cairo_surface()==NULL)
		return NULL;
	return Target_Cairo::Handle(new target2cairosurface(surface));
}

void
synfig::Surface::clear()
{
#ifdef HAS_VIMAGE
	fill(Color(0.5,0.5,0.5,0.0000001));
#else
	etl::surface<Color, ColorAccumulator, ColorPrep>::clear();
#endif
}

void
synfig::Surface::blit_to(alpha_pen& pen, int x, int y, int w, int h)
{
	static const float epsilon(0.00001);
	const float alpha(pen.get_alpha());
	if(	pen.get_blend_method()==Color::BLEND_STRAIGHT && fabs(alpha-1.0f)<epsilon )
	{
		if(x>=get_w() || y>=get_w())
			return;

		//clip source origin
		if(x<0)
		{
			w+=x;	//decrease
			x=0;
		}

		if(y<0)
		{
			h+=y;	//decrease
			y=0;
		}

		//clip width against dest width
		w = min((long)w,(long)(pen.end_x()-pen.x()));
		h = min((long)h,(long)(pen.end_y()-pen.y()));

		//clip width against src width
		w = min(w,get_w()-x);
		h = min(h,get_h()-y);

		if(w<=0 || h<=0)
			return;

		for(int i=0;i<h;i++)
		{
			char* src(static_cast<char*>(static_cast<void*>(operator[](y)+x))+i*get_w()*sizeof(Color));
			char* dest(static_cast<char*>(static_cast<void*>(pen.x()))+i*pen.get_width()*sizeof(Color));
			memcpy(dest,src,w*sizeof(Color));
		}
		return;
	}

#ifdef HAS_VIMAGE
	if(	pen.get_blend_method()==Color::BLEND_COMPOSITE && fabs(alpha-1.0f)<epsilon )
	{
		if(x>=get_w() || y>=get_w())
			return;

		//clip source origin
		if(x<0)
		{
			//u-=x;	//increase
			w+=x;	//decrease
			x=0;
		}

		if(y<0)
		{
			//v-=y;	//increase
			h+=y;	//decrease
			y=0;
		}

		//clip width against dest width
		w = min(w,pen.end_x()-pen.x());
		h = min(h,pen.end_y()-pen.y());

		//clip width against src width
		w = min(w,get_w()-x);
		h = min(h,get_h()-y);

		if(w<=0 || h<=0)
			return;



		vImage_Buffer top,bottom;
		vImage_Buffer& dest(bottom);

		top.data=static_cast<void*>(operator[](y)+x);
		top.height=h;
		top.width=w;
		//top.rowBytes=get_w()*sizeof(Color); //! \todo this should get the pitch!!
		top.rowBytes=get_pitch();

		bottom.data=static_cast<void*>(pen.x());
		bottom.height=h;
		bottom.width=w;
		//bottom.rowBytes=pen.get_width()*sizeof(Color); //! \todo this should get the pitch!!
		bottom.rowBytes=pen.get_pitch(); //! \todo this should get the pitch!!

		vImage_Error ret;
		ret=vImageAlphaBlend_ARGBFFFF(&top,&bottom,&dest,kvImageNoFlags);

		assert(ret!=kvImageNoError);

		return;
	}
#endif
	etl::surface<Color, ColorAccumulator, ColorPrep>::blit_to(pen,x,y,w,h);
}

void
synfig::CairoSurface::blit_to(alpha_pen& pen, int x, int y, int w, int h)
{
	static const float epsilon(0.00001);
	const float alpha(pen.get_alpha());
	if(	pen.get_blend_method()==Color::BLEND_STRAIGHT && fabs(alpha-1.0f)<epsilon )
	{
		if(x>=get_w() || y>=get_w())
			return;
		
		//clip source origin
		if(x<0)
		{
			w+=x;	//decrease
			x=0;
		}
		
		if(y<0)
		{
			h+=y;	//decrease
			y=0;
		}
		
		//clip width against dest width
		w = min((long)w,(long)(pen.end_x()-pen.x()));
		h = min((long)h,(long)(pen.end_y()-pen.y()));
		
		//clip width against src width
		w = min(w,get_w()-x);
		h = min(h,get_h()-y);
		
		if(w<=0 || h<=0)
			return;
		
		for(int i=0;i<h;i++)
		{
			char* src(static_cast<char*>(static_cast<void*>(operator[](y)+x))+i*get_w()*sizeof(CairoColor));
			char* dest(static_cast<char*>(static_cast<void*>(pen.x()))+i*pen.get_width()*sizeof(CairoColor));
			memcpy(dest,src,w*sizeof(CairoColor));
		}
		return;
	}
	else
		etl::surface<CairoColor, CairoColor, CairoColorPrep>::blit_to(pen,x,y,w,h);
}

void
CairoSurface::set_wh(int /*w*/, int /*h*/, int /*pitch*/)
{
	// I shouldn't reach this code never but I'll write it down to verify I don't 
	// call any set_wh when copying the software redner code
	synfig::error("Cannot resize a CairoImage directly. Use its Target_Cairo instance");
	assert(0);
}

void 
CairoSurface::set_cairo_surface(cairo_surface_t *cs)
{
	if(cs==NULL)
	{
		synfig::error("CairoSruface received a NULL cairo_surface_t");
		return;
	}
	if(cairo_surface_status(cs))
	{
		synfig::error("CairoSurface received a non valid cairo_surface_t");
		return;
	}
	else
	{
		if(cs_!=NULL)
			cairo_surface_destroy(cs_);
		cs_=cairo_surface_reference(cs);
	}
}

cairo_surface_t* 
CairoSurface::get_cairo_surface()const
{
	assert(cs_);
	return cairo_surface_reference(cs_);
}

bool
CairoSurface::map_cairo_image()
{
	assert(cs_);
	cairo_surface_flush(cs_);
	cs_image_=cairo_surface_map_to_image (cs_, NULL);
	if(cs_image_!=NULL)
	{
		cairo_format_t t=cairo_image_surface_get_format(cs_image_);
		if(t==CAIRO_FORMAT_ARGB32 || t==CAIRO_FORMAT_RGB24)
		{
			unsigned char* data(cairo_image_surface_get_data(cs_image_));
			int w(cairo_image_surface_get_width(cs_image_));
			int h(cairo_image_surface_get_height(cs_image_));
			int stride(cairo_image_surface_get_stride(cs_image_));
			set_wh(w, h, data, stride);
			return true;
		}
		return false;	
	}
	return false;
}

void
CairoSurface::unmap_cairo_image()
{
	assert(cs_image_);
	assert(cs_);
	cairo_surface_unmap_image(cs_, cs_image_);
	cairo_surface_mark_dirty(cs_);
}





