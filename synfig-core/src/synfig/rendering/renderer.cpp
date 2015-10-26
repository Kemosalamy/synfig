/* === S Y N F I G ========================================================= */
/*!	\file synfig/rendering/renderer.cpp
**	\brief Renderer
**
**	$Id$
**
**	\legal
**	......... ... 2015 Ivan Mahonin
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

#ifndef WIN32
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#endif

#include <typeinfo>

#include <glibmm/threads.h>

#include <synfig/general.h>
#include <synfig/localization.h>
#include <synfig/debug/measure.h>
#include <synfig/debug/debugsurface.h>

#include "renderer.h"

#include "software/renderersw.h"
#include "opengl/renderergl.h"
#include "common/task/taskcallback.h"
#include "opengl/task/taskgl.h"

#endif

using namespace synfig;
using namespace rendering;


#ifdef _DEBUG
//#define DEBUG_TASK_LIST
//#define DEBUG_TASK_MEASURE
//#define DEBUG_TASK_SURFACE
//#define DEBUG_OPTIMIZATION
//#define DEBUG_THREAD_TASK
//#define DEBUG_THREAD_WAIT
#endif


/* === M A C R O S ========================================================= */

/* === G L O B A L S ======================================================= */

/* === P R O C E D U R E S ================================================= */

/* === M E T H O D S ======================================================= */

Renderer::Handle Renderer::blank;
std::map<String, Renderer::Handle> *Renderer::renderers;
Renderer::Queue *Renderer::queue;


class Renderer::Queue
{
private:
	Glib::Threads::Mutex mutex;
	Glib::Threads::Mutex threads_mutex;
	Glib::Threads::Cond cond;

	Task::List queue;
	bool started;

	std::list<Glib::Threads::Thread*> threads;

	void start() {
		Glib::Threads::Mutex::Lock lock(mutex);
		if (started) return;

		// one thread reserved for OpenGL
		// also this thread almost don't use CPU time
		// so we have ~50% of one core for GUI
		#ifdef DEBUG_TASK_SURFACE
		int count = 2;
		#else
		int count = g_get_num_processors();
		if (count < 2) count = 2;
		#endif

		for(int i = 0; i < count; ++i)
			threads.push_back(
				Glib::Threads::Thread::create(
					sigc::bind(sigc::mem_fun(*this, &Queue::process), i) ));
		info("rendering threads %d", count);
		started = true;
	}

	void stop() {
		{
			Glib::Threads::Mutex::Lock lock(mutex);
			started = false;
			cond.broadcast();
		}
		while(!threads.empty())
			{ threads.front()->join(); threads.pop_front(); }
	}

	void process(int index)
	{
		while(Task::Handle task = get(index))
		{
			#ifdef DEBUG_THREAD_TASK
			info("thread %d: begin task '%s'", index, typeid(*task).name());
			#endif

			if (!task->run(task->params))
				task->success = false;

			#ifdef DEBUG_TASK_SURFACE
			debug::DebugSurface::save_to_file(task->target_surface, etl::strprintf("task"));
			#endif

			#ifdef DEBUG_THREAD_TASK
			info("thread %d: end task '%s'", index, typeid(*task).name());
			#endif

			done(task);
		}
	}

	void done(const Task::Handle &task)
	{
		assert(task);
		bool unlocked = false;
		Glib::Threads::Mutex::Lock lock(mutex);
		for(Task::List::iterator i = queue.begin(); i != queue.end();)
		{
			assert(*i);
			if (*i == task) { i = queue.erase(i); continue; }
			for(Task::List::iterator j = (*i)->deps.begin(); j != (*i)->deps.end();)
			{
				assert(*j);
				if (*j == task)
				{
					if (!task->success) (*i)->success = false;
					j = (*i)->deps.erase(j);
					if ((*i)->deps.empty()) unlocked = true;
					continue;
				}
				++j;
			}
			++i;
		}
		if (unlocked) cond.broadcast();
	}

	Task::Handle get(int thread_index) {
		Glib::Threads::Mutex::Lock lock(mutex);
		while(true)
		{
			if (!started) return Task::Handle();

			for(Task::List::iterator i = queue.begin(); i != queue.end(); ++i)
			{
				assert(*i);
				if ((*i)->deps.empty() && ((thread_index == 0) == i->type_is<TaskGL>()))
				{
					Task::Handle task = *i;
					queue.erase(i);
					return task;
				}
			}

			#ifdef DEBUG_THREAD_WAIT
			if (thread_index > 0 && !queue.empty())
				info("thread %d: rendering wait for task", thread_index);
			#endif

			cond.wait(mutex);
		}
	}

	static void fix_task(const Task &task, const Task::RunParams &params)
	{
		for(Task::List::iterator i = task.deps.begin(); i != task.deps.end();)
			if (*i) ++i; else i = (*i)->deps.erase(i);
		task.params = params;
		task.success = true;
	}

public:
	Queue(): started(false) { start(); }
	~Queue() { stop(); }

	void enqueue(const Task::Handle &task, const Task::RunParams &params)
	{
		if (!task) return;
		fix_task(*task, params);
		Glib::Threads::Mutex::Lock lock(mutex);
		queue.push_back(task);
		cond.broadcast();
	}

	void enqueue(const Task::List &tasks, const Task::RunParams &params)
	{
		int count = 0;
		for(Task::List::const_iterator i = tasks.begin(); i != tasks.end(); ++i)
			if (*i) { fix_task(**i, params); ++count; }
		if (!count) return;
		Glib::Threads::Mutex::Lock lock(mutex);
		for(Task::List::const_iterator i = tasks.begin(); i != tasks.end(); ++i)
			if (*i) queue.push_back(*i);
		cond.broadcast();
	}

	void clear()
	{
		Glib::Threads::Mutex::Lock lock(mutex);
		queue.clear();
	}
};



void
Renderer::initialize_renderers()
{
	// initialize renderers
	RendererSW::initialize();
	RendererGL::initialize();

	// register renderers
	register_renderer("software", new RendererSW());
	register_renderer("gl", new RendererGL());
}

void
Renderer::deinitialize_renderers()
{
	RendererGL::deinitialize();
	RendererSW::deinitialize();
}



Renderer::~Renderer() { }

bool
Renderer::is_optimizer_registered(const Optimizer::Handle &optimizer) const
{
	if (!optimizer)
		for(Optimizer::List::const_iterator i = optimizers[optimizer->category_id].begin(); i != optimizers[optimizer->category_id].end(); ++i)
			if (*i == optimizer) return true;
	return false;
}

void
Renderer::register_optimizer(const Optimizer::Handle &optimizer)
{
	if (optimizer) {
		assert(!is_optimizer_registered(optimizer));
		optimizers[optimizer->category_id].push_back(optimizer);
	}
}

void
Renderer::unregister_optimizer(const Optimizer::Handle &optimizer)
{
	for(Optimizer::List::iterator i = optimizers[optimizer->category_id].begin(); i != optimizers[optimizer->category_id].end();)
		if (*i == optimizer) i = optimizers[optimizer->category_id].erase(i); else ++i;
}

void
Renderer::optimize_recursive(const Optimizer::List &optimizers, const Optimizer::RunParams& params, bool first_level_only) const
{
	if (!params.ref_task) return;
	if (params.ref_affects_to & params.depends_from) return;

	for(Optimizer::List::const_iterator i = optimizers.begin(); i != optimizers.end(); ++i)
	{
		if (!(*i)->deep_first)
		{
			if ((*i)->for_task || ((*i)->for_root_task && !params.parent))
			{
				Optimizer::RunParams p(params);
				(*i)->run(p);
				params.ref_affects_to |= p.ref_affects_to;
				params.ref_mode |= p.ref_mode;
				params.ref_task = p.ref_task;
				if (!params.ref_task) return;
				if (params.ref_affects_to & params.depends_from) return;
			}
		}
	}

	if (!first_level_only)
	{
		bool task_clonned = false;
		for(Task::List::iterator i = params.ref_task->sub_tasks.begin(); i != params.ref_task->sub_tasks.end();)
		{
			if (*i)
			{
				Optimizer::RunParams sub_params = params.sub(*i);
				optimize_recursive(optimizers, sub_params, false);
				if (sub_params.ref_task != *i)
				{
					if (!task_clonned)
					{
						int index = i - params.ref_task->sub_tasks.begin();
						params.ref_task = params.ref_task->clone();
						i = params.ref_task->sub_tasks.begin() + index;
						task_clonned = true;
					}
					*i = sub_params.ref_task;
					if (!(sub_params.ref_mode & Optimizer::MODE_REPEAT)) ++i;
				} else ++i;
				params.ref_affects_to |= sub_params.ref_affects_to;
				if (sub_params.ref_mode & Optimizer::MODE_REPEAT_BRUNCH)
					params.ref_mode |= sub_params.ref_mode;
				if (params.ref_affects_to & params.depends_from) return;
			} else ++i;
		}
	}

	for(Optimizer::List::const_iterator i = optimizers.begin(); i != optimizers.end(); ++i)
	{
		if ((*i)->deep_first)
		{
			if ((*i)->for_task || ((*i)->for_root_task && !params.parent))
			{
				Optimizer::RunParams p(params);
				(*i)->run(p);
				params.ref_affects_to |= p.ref_affects_to;
				params.ref_mode |= p.ref_mode;
				params.ref_task = p.ref_task;
				if (!params.ref_task) return;
				if (params.ref_affects_to & params.depends_from) return;
			}
		}
	}

}

void
Renderer::optimize(Task::List &list) const
{
	//debug::Measure t("Renderer::optimize");

	int current_category_id = 0;
	int current_optimizer_index = 0;
	Optimizer::Category current_affected = 0;
	Optimizer::Category categories_to_process = Optimizer::CATEGORY_ALL;
	Optimizer::List single(1);

	while(categories_to_process &= Optimizer::CATEGORY_ALL)
	{
		if (current_category_id >= Optimizer::CATEGORY_ID_COUNT)
		{
			current_category_id = 0;
			current_optimizer_index = 0;
			current_affected = 0;
			continue;
		}

		if (!((1 << current_category_id) & categories_to_process))
		{
			++current_category_id;
			current_optimizer_index = 0;
			current_affected = 0;
			continue;
		}

		if (current_optimizer_index >= (int)optimizers[current_category_id].size())
		{
			categories_to_process &= ~(1 << current_category_id);
			categories_to_process |= current_affected;
			++current_category_id;
			current_optimizer_index = 0;
			current_affected = 0;
			continue;
		}

		bool simultaneous_run = Optimizer::categories_info[current_category_id].simultaneous_run;
		const Optimizer::List &current_optimizers = simultaneous_run ? optimizers[current_category_id] : single;
		if (!simultaneous_run) {
			single.front() = optimizers[current_category_id][current_optimizer_index];
			Optimizer::Category depends_from_self = (1 << current_category_id) & single.front()->depends_from;
			if (current_affected & depends_from_self)
			{
				current_category_id = 0;
				current_optimizer_index = 0;
				current_affected = 0;
				continue;
			}
		}

		Optimizer::Category depends_from = 0;
		bool for_list = false;
		bool for_task = false;
		bool for_root_task = false;
		for(Optimizer::List::const_iterator i = current_optimizers.begin(); i != current_optimizers.end(); ++i)
		{
			depends_from |= ((1 << current_category_id) - 1) & (*i)->depends_from;
			if ((*i)->for_list) for_list = true;
			if ((*i)->for_task) for_task = true;
			if ((*i)->for_root_task) for_root_task = true;
		}

		#ifdef DEBUG_OPTIMIZATION
		log(list, etl::strprintf("before optimize category %d index %d", current_category_id, current_optimizer_index));
		//debug::Measure t(etl::strprintf("optimize category %d index %d", current_category_id, current_optimizer_index));
		#endif

		if (for_list)
		{
			for(Optimizer::List::const_iterator i = current_optimizers.begin(); !(categories_to_process & depends_from) && i != current_optimizers.end(); ++i)
			{
				if ((*i)->for_list)
				{
					Optimizer::RunParams params(*this, list, depends_from);
					(*i)->run(params);
					categories_to_process |= current_affected |= params.ref_affects_to;
				}
			}
		}

		if (for_task || for_root_task)
		{
			for(Task::List::iterator j = list.begin(); !(categories_to_process & depends_from) && j != list.end();)
			{
				if (*j)
				{
					Optimizer::RunParams params(*this, list, depends_from, *j);
					optimize_recursive(current_optimizers, params, !for_task);
					if (*j != params.ref_task)
					{
						if (params.ref_task)
						{
							*j = params.ref_task;
							if (!(params.ref_mode & Optimizer::MODE_REPEAT)) ++j;
						}
						else
							j = list.erase(j);
					} else ++j;
					categories_to_process |= current_affected |= params.ref_affects_to;
				}
				else
				{
					j = list.erase(j);
				}
			}
		}

		if (categories_to_process & depends_from)
		{
			current_category_id = 0;
			current_optimizer_index = 0;
			current_affected = 0;
			continue;
		}

		current_optimizer_index += current_optimizers.size();
	}

	// remove nulls
	for(Task::List::iterator j = list.begin(); j != list.end();)
		if (*j) ++j; else j = list.erase(j);
}

bool
Renderer::run(const Task::List &list) const
{
	#ifdef DEBUG_TASK_MEASURE
	debug::Measure t("Renderer::run");
	#endif

	#ifdef DEBUG_TASK_LIST
	log(list, "input list");
	#endif

	Task::List optimized_list(list);
	{
		#ifdef DEBUG_TASK_MEASURE
		debug::Measure t("optimize");
		#endif
		optimize(optimized_list);
	}

	#ifdef DEBUG_TASK_LIST
	log(optimized_list, "optimized list");
	#endif

	{
		#ifdef DEBUG_TASK_MEASURE
		debug::Measure t("find deps");
		#endif

		//int brunches = 0;
		for(Task::List::const_iterator i = optimized_list.begin(); i != optimized_list.end(); ++i)
		{
			(*i)->deps.clear();
			for(Task::List::const_iterator j = (*i)->sub_tasks.begin(); j != (*i)->sub_tasks.end(); ++j)
				if (*j)
					for(Task::List::const_reverse_iterator rk(i); rk != optimized_list.rend(); ++rk)
						if ( (*j)->target_surface == (*rk)->target_surface
						  && etl::intersect((*j)->target_rect, (*rk)->target_rect) )
							{ (*i)->deps.push_back(*rk); break; }
			for(Task::List::const_reverse_iterator rk(i); rk != optimized_list.rend(); ++rk)
				if ( (*i)->target_surface == (*rk)->target_surface
				  && etl::intersect((*i)->target_rect, (*rk)->target_rect) )
					{ (*i)->deps.push_back(*rk); break; }
			//if ((*i)->deps.empty()) ++brunches;
		}
		//info("rendering brunches %d", brunches);
	}

	bool success = true;

	{
		#ifdef DEBUG_TASK_MEASURE
		debug::Measure t("run tasks");
		#endif

		Glib::Threads::Cond cond;
		Glib::Threads::Mutex mutex;
		Glib::Threads::Mutex::Lock lock(mutex);

		TaskCallbackCond::Handle task_cond = new TaskCallbackCond();
		task_cond->cond = &cond;
		task_cond->mutex = &mutex;
		task_cond->deps = optimized_list;
		optimized_list.push_back(task_cond);

		queue->enqueue(optimized_list, Task::RunParams());

		task_cond->cond->wait(mutex);
		if (!task_cond->success) success = false;
	}

	return success;
}

void
Renderer::log(const Task::Handle &task, const String &prefix) const
{
	if (task)
	{
		info( prefix
			+ (typeid(*task).name() + 19)
			+ ( task->bounds.valid()
			  ? etl::strprintf(" bounds (%f, %f)-(%f, %f)",
				task->bounds.minx, task->bounds.miny,
				task->bounds.maxx, task->bounds.maxy )
			  : "" )
			+ ( task->source_rect_lt[0] && task->source_rect_lt[1]
			 && task->source_rect_rb[0] && task->source_rect_rb[1]
              ? etl::strprintf(" source (%f, %f)-(%f, %f)",
				task->source_rect_lt[0], task->source_rect_lt[1],
				task->source_rect_rb[0], task->source_rect_rb[1] )
		      : "" )
			+ ( task->target_rect.valid()
			  ? etl::strprintf(" target (%d, %d)-(%d, %d)",
				task->target_rect.minx, task->target_rect.miny,
				task->target_rect.maxx, task->target_rect.maxy )
			  : "" )
			+ ( task->target_surface
			  ?	etl::strprintf(" surface %s (%dx%d) 0x%x",
					(typeid(*task->target_surface).name() + 19),
					task->target_surface->get_width(),
					task->target_surface->get_height(),
					task->target_surface.get() )
			  : "" ));
		for(Task::List::const_iterator i = task->sub_tasks.begin(); i != task->sub_tasks.end(); ++i)
			log(*i, prefix + "  ");
	}
	else
	{
		info(prefix + " NULL");
	}
}

void
Renderer::log(const Task::List &list, const String &name) const
{
	String line = "-------------------------------------------";
	String n = "    " + name;
	n.resize(line.size(), ' ');
	for(int i = 0; i < (int)line.size(); ++i)
		if (n[i] == ' ') n[i] = line[i];
	info(n);
	for(Task::List::const_iterator i = list.begin(); i != list.end(); ++i)
		log(*i);
	info(line);
}

void
Renderer::initialize()
{
	if (renderers != NULL || queue != NULL)
		synfig::error("rendering::Renderer already initialized");
	renderers = new std::map<String, Handle>();
	queue = new Queue();

	initialize_renderers();
}

void
Renderer::deinitialize()
{
	if (renderers == NULL || queue == NULL)
		synfig::error("rendering::Renderer not initialized");

	while(!get_renderers().empty())
		unregister_renderer(get_renderers().begin()->first);

	deinitialize_renderers();

	delete renderers;
	delete queue;
}

void
Renderer::register_renderer(const String &name, const Renderer::Handle &renderer)
{
	if (get_renderer(name))
		synfig::error("rendering::Renderer renderer '%s' already registered", name.c_str());
	(*renderers)[name] = renderer;
}

void
Renderer::unregister_renderer(const String &name)
{
	if (!get_renderer(name))
		synfig::error("rendering::Renderer renderer '%s' not registered", name.c_str());
	renderers->erase(name);
}

const Renderer::Handle&
Renderer::get_renderer(const String &name)
{
	return get_renderers().count(name) > 0
		 ? get_renderers().find(name)->second
		 : blank;
}

const std::map<String, Renderer::Handle>&
Renderer::get_renderers()
{
	if (renderers == NULL)
		synfig::error("rendering::Renderer not initialized");
	return *renderers;
}

/* === E N T R Y P O I N T ================================================= */
