/* === S I N F G =========================================================== */
/*!	\file target_scanline.cpp
**	\brief Template File
**
**	$Id: target_scanline.cpp,v 1.1.1.1 2005/01/04 01:23:15 darco Exp $
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

/* === H E A D E R S ======================================================= */

#ifdef USING_PCH
#	include "pch.h"
#else
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "target_scanline.h"
#include "string.h"
#include "surface.h"
#include "render.h"
#include "canvas.h"
#include "context.h"

#endif

/* === U S I N G =========================================================== */

using namespace std;
using namespace etl;
using namespace sinfg;

/* === M A C R O S ========================================================= */

#define SINFG_OPTIMIZE_LAYER_TREE 1

#define PIXEL_RENDERING_LIMIT 1500000

#define USE_PIXELRENDERING_LIMIT 0

/* === G L O B A L S ======================================================= */

/* === P R O C E D U R E S ================================================= */

/* === M E T H O D S ======================================================= */

Target_Scanline::Target_Scanline():
	threads_(2)
{
	curr_frame_=0;
}

int
Target_Scanline::next_frame(Time& time)
{
	int
		total_frames(1),
		frame_start(0),
		frame_end(0);
	Time
		time_start(0),
		time_end(0);

	// If the description's end frame is equal to
	// the start frame, then it is assumed that we
	// are rendering only one frame. Correct it.
	if(desc.get_frame_end()==desc.get_frame_start())
		desc.set_frame_end(desc.get_frame_start()+1);

	frame_start=desc.get_frame_start();
	frame_end=desc.get_frame_end();
	time_start=desc.get_time_start();
	time_end=desc.get_time_end();
	
	// Calculate the number of frames
	total_frames=frame_end-frame_start;
	if(total_frames<=0)total_frames=1;
	
	//RendDesc rend_desc=desc;
	//rend_desc.set_gamma(1);

//	int total_tiles(total_tiles());
	time=(time_end-time_start)*curr_frame_/total_frames+time_start;
	curr_frame_++;

/*	sinfg::info("curr_frame_: %d",curr_frame_);
	sinfg::info("total_frames: %d",total_frames);
	sinfg::info("time_end: %s",time_end.get_string().c_str());
	sinfg::info("time_start: %s",time_start.get_string().c_str());
*/
//	sinfg::info("time: %s",time.get_string().c_str());

	return total_frames- curr_frame_+1;
}
bool
sinfg::Target_Scanline::render(ProgressCallback *cb)
{
	SuperCallback super_cb;
	int
//		i=0,
		total_frames,
		quality=get_quality(),
		frame_start,
		frame_end;
	Time
		t=0,
		time_start,
		time_end;

	assert(canvas);
	curr_frame_=0;

	init();
	
	// If the description's end frame is equal to
	// the start frame, then it is assumed that we
	// are rendering only one frame. Correct it.
	if(desc.get_frame_end()==desc.get_frame_start())
		desc.set_frame_end(desc.get_frame_start()+1);

	frame_start=desc.get_frame_start();
	frame_end=desc.get_frame_end();
	time_start=desc.get_time_start();
	time_end=desc.get_time_end();
	
	// Calculate the number of frames
	total_frames=frame_end-frame_start;
	
	
	//RendDesc rend_desc=desc;
		
	try {
	// Grab the time
	int i=next_frame(t);
	
	//sinfg::info("1time_set_to %s",t.get_string().c_str());
	
	if(i>1)
	do{
	
	//if(total_frames>1)
	//for(i=0,t=time_start;i<total_frames;i++)
	//{
		//t=((time_end-time_start)*((Real)i/(Real)total_frames)).round(desc.get_frame_rate())+time_start;

		// If we have a callback, and it returns
		// false, go ahead and bail. (it may be a user cancel)
		if(cb && !cb->amount_complete(total_frames-(i-1),total_frames))
			return false;
		
		// Set the time that we wish to render
		if(!get_avoid_time_sync() || canvas->get_time()!=t)
			canvas->set_time(t);
		
		Context context;
		
		#ifdef SINFG_OPTIMIZE_LAYER_TREE
		Canvas::Handle op_canvas(Canvas::create());
		optimize_layers(canvas->get_context(), op_canvas);
		context=op_canvas->get_context();
		#else
		context=canvas->get_context();
		#endif
		
		// If the quality is set to zero, then we
		// use the parametric scanline-renderer.
		if(quality==0)
		{
			if(threads_<=0)
			{
				if(!sinfg::render(context,this,desc,0))
					return false;
			}
			else
			{
				if(!sinfg::render_threaded(context,this,desc,0,threads_))
					return false;
			}
		}
		else // If quality is set otherwise, then we use the accelerated renderer
		{
			#if USE_PIXELRENDERING_LIMIT
			if(desc.get_w()*desc.get_h() > PIXEL_RENDERING_LIMIT)
			{					
				sinfg::info("Render BROKEN UP! (%d pixels)", desc.get_w()*desc.get_h());
								
				Surface surface;				
				int rowheight = PIXEL_RENDERING_LIMIT/desc.get_w();
				int rows = desc.get_h()/rowheight;
				int lastrowheight = desc.get_h() - rows*rowheight;
				
				rows++;
				
				sinfg::info("\t blockh=%d,remh=%d,totrows=%d", rowheight,lastrowheight,rows);				
				
				// loop through all the full rows
				if(!start_frame())
				{
					throw(string("add_frame(): target panic on start_frame()"));
					return false;
				}
				
				for(int i=0; i < rows; ++i)
				{
					RendDesc	blockrd = desc;
					
					//render the strip at the normal size unless it's the last one...
					if(i == rows)
					{
						if(!lastrowheight) break;
						blockrd.set_subwindow(0,i*rowheight,desc.get_w(),lastrowheight);
					}
					else
					{
						blockrd.set_subwindow(0,i*rowheight,desc.get_w(),rowheight);
					}
					
					if(!context.accelerated_render(&surface,quality,blockrd,0))
					{
						if(cb)cb->error(_("Accelerated Renderer Failure"));
						return false;
					}else
					{
						int y;
						int rowspan=sizeof(Color)*surface.get_w();
						Surface::pen pen = surface.begin();
						
						int yoff = i*rowheight;
						
						for(y = 0; y < blockrd.get_h(); y++, pen.inc_y())
						{
							Color *colordata= start_scanline(y + yoff);
							if(!colordata)
							{
								throw(string("add_frame(): call to start_scanline(y) returned NULL"));
								return false;
							}
							
							if(get_remove_alpha())
							{
								for(int i = 0; i < surface.get_w(); i++)
									colordata[i] = Color::blend(surface[y][i],desc.get_bg_color(),1.0f);
							}
							else
								memcpy(colordata,surface[y],rowspan);
						
							if(!end_scanline())
							{
								throw(string("add_frame(): target panic on end_scanline()"));
								return false;
							}
						}
					}
				}	

				end_frame();				
				
			}else //use normal rendering...
			{
			#endif
				Surface surface;
				
				if(!context.accelerated_render(&surface,quality,desc,0))
				{
					// For some reason, the accelerated renderer failed.
					if(cb)cb->error(_("Accelerated Renderer Failure"));
					return false;
				}
				else
				{
					// Put the surface we renderer
					// onto the target.
					if(!add_frame(&surface))
					{
						if(cb)cb->error(_("Unable to put surface on target"));
						return false;
					}
				}
			#if USE_PIXELRENDERING_LIMIT
			}
			#endif
		}
	}while((i=next_frame(t)));
    else
    {
		// Set the time that we wish to render
		if(!get_avoid_time_sync() || canvas->get_time()!=t)
			canvas->set_time(t);
		Context context;
		
		#ifdef SINFG_OPTIMIZE_LAYER_TREE
		Canvas::Handle op_canvas(Canvas::create());
		optimize_layers(canvas->get_context(), op_canvas);
		context=op_canvas->get_context();
		#else
		context=canvas->get_context();
		#endif
		
		// If the quality is set to zero, then we
		// use the parametric scanline-renderer.
		if(quality==0)
		{
			if(threads_<=0)
			{
				if(!sinfg::render(context,this,desc,cb))
					return false;
			}
			else
			{
				if(!sinfg::render_threaded(context,this,desc,cb,threads_))
					return false;
			}
		}
		else // If quality is set otherwise, then we use the accelerated renderer
		{
			#if USE_PIXELRENDERING_LIMIT
			if(desc.get_w()*desc.get_h() > PIXEL_RENDERING_LIMIT)
			{
				sinfg::info("Render BROKEN UP! (%d pixels)", desc.get_w()*desc.get_h());
				
				Surface surface;				
				int totalheight = desc.get_h();				
				int rowheight = PIXEL_RENDERING_LIMIT/desc.get_w();
				int rows = desc.get_h()/rowheight;
				int lastrowheight = desc.get_h() - rows*rowheight;
				
				rows++;
				
				sinfg::info("\t blockh=%d,remh=%d,totrows=%d", rowheight,lastrowheight,rows);				
				
				// loop through all the full rows
				if(!start_frame())
				{
					throw(string("add_frame(): target panic on start_frame()"));
					return false;
				}
				
				for(int i=0; i < rows; ++i)
				{
					RendDesc	blockrd = desc;
					
					//render the strip at the normal size unless it's the last one...
					if(i == rows)
					{
						if(!lastrowheight) break;
						blockrd.set_subwindow(0,i*rowheight,desc.get_w(),lastrowheight);
					}
					else
					{
						blockrd.set_subwindow(0,i*rowheight,desc.get_w(),rowheight);
					}
					
					SuperCallback	sc(cb, i*rowheight, (i+1)*rowheight, totalheight);
					
					if(!context.accelerated_render(&surface,quality,blockrd,&sc))
					{
						if(cb)cb->error(_("Accelerated Renderer Failure"));
						return false;
					}else
					{
						int y;
						int rowspan=sizeof(Color)*surface.get_w();
						Surface::pen pen = surface.begin();
						
						int yoff = i*rowheight;
						
						for(y = 0; y < blockrd.get_h(); y++, pen.inc_y())
						{
							Color *colordata= start_scanline(y + yoff);
							if(!colordata)
							{
								throw(string("add_frame(): call to start_scanline(y) returned NULL"));
								return false;
							}
							
							if(get_remove_alpha())
							{
								for(int i = 0; i < surface.get_w(); i++)
									colordata[i] = Color::blend(surface[y][i],desc.get_bg_color(),1.0f);
							}
							else
								memcpy(colordata,surface[y],rowspan);
						
							if(!end_scanline())
							{
								throw(string("add_frame(): target panic on end_scanline()"));
								return false;
							}
						}
					}
					
					//I'm done with this part
					sc.amount_complete(100,100);
				}	

				end_frame();				
				
			}else
			{
			#endif			
				Surface surface;
				
				if(!context.accelerated_render(&surface,quality,desc,cb))
				{
					if(cb)cb->error(_("Accelerated Renderer Failure"));
					return false;
				}
				else
				{
					// Put the surface we renderer
					// onto the target.
					if(!add_frame(&surface))
					{
						if(cb)cb->error(_("Unable to put surface on target"));
						return false;
					}
				}
			#if USE_PIXELRENDERING_LIMIT
			}
			#endif
		}
	}
	
	}
	catch(String str)
	{
		if(cb)cb->error(_("Caught string :")+str);
		return false;
	}
	catch(std::bad_alloc)
	{
		if(cb)cb->error(_("Ran out of memory (Probably a bug)"));
		return false;
	}
	catch(...)
	{
		if(cb)cb->error(_("Caught unknown error, rethrowing..."));
		throw;
	}
	return true;
}

bool
Target_Scanline::add_frame(const Surface *surface)
{
	assert(surface);


	int y;
	int rowspan=sizeof(Color)*surface->get_w();
	Surface::const_pen pen=surface->begin();
	
	if(!start_frame())
	{
		throw(string("add_frame(): target panic on start_frame()"));
		return false;
	}
		
	for(y=0;y<surface->get_h();y++,pen.inc_y())
	{
		Color *colordata= start_scanline(y);
		if(!colordata)
		{
			throw(string("add_frame(): call to start_scanline(y) returned NULL"));
			return false;
		}
		
		if(get_remove_alpha())
		{
			for(int i=0;i<surface->get_w();i++)
				colordata[i]=Color::blend((*surface)[y][i],desc.get_bg_color(),1.0f);
		}
		else
			memcpy(colordata,(*surface)[y],rowspan);
	
		if(!end_scanline())
		{
			throw(string("add_frame(): target panic on end_scanline()"));
			return false;
		}
	}
	
	end_frame();
	
	return true;
}
