/*
 * *****************************************************************
 * *                                                               *
 * *    Copyright (c) Digital Equipment Corporation, 1991, 1994    *
 * *                                                               *
 * *   All Rights Reserved.  Unpublished rights  reserved  under   *
 * *   the copyright laws of the United States.                    *
 * *                                                               *
 * *   The software contained on this media  is  proprietary  to   *
 * *   and  embodies  the  confidential  technology  of  Digital   *
 * *   Equipment Corporation.  Possession, use,  duplication  or   *
 * *   dissemination of the software and media is authorized only  *
 * *   pursuant to a valid written license from Digital Equipment  *
 * *   Corporation.                                                *
 * *                                                               *
 * *   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  *
 * *   by the U.S. Government is subject to restrictions  as  set  *
 * *   forth in Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  *
 * *   or  in  FAR 52.227-19, as applicable.                       *
 * *                                                               *
 * *****************************************************************
 */
/*
 * HISTORY
 */
#ifndef lint
static char	*sccsid = "@(#)$RCSfile: processor.c,v $ $Revision: 4.3.6.2 $ (DEC) $Date: 1993/05/22 17:40:39 $";
#endif 
/*
 */
/*
 * (c) Copyright 1990, OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED
 */
/* 
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * Copyright (c) 1988 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * OSF/1 Release 1.0
 */

/*
 *	processor.c: processor and processor_set manipulation routines.
 *
 *	Revision History:
 *
 * 18-Oct-91 jestabro
 *      Added ALPHA support
 *
 * 02-May-91	Peter H. Smith
 *	Turn on POSIX Realtime policies by default, and initialize the
 *	processor->was_first_quantum flag.
 */

#include <cpus.h>
#include <mach_host.h>
#include <rt_sched.h>
#include <rt_sched_rq.h>

#include <kern/ipc_globals.h>	/* for ipc_kernel_map */

#include <sys/types.h>
#include <mach/boolean.h>
#include <kern/lock.h>
#include <kern/host.h>
#include <mach/policy.h>
#include <kern/processor.h>
#include <mach/processor_info.h>
#include <kern/sched.h>
#include <kern/thread.h>
#include <mach/vm_param.h>
#include <kern/ipc_host.h>

#include <machine/cpu.h>

#if	MACH_HOST
#include <kern/zalloc.h>
zone_t	pset_zone;
#endif	MACH_HOST

/*
 *	Exported variables.
 */
struct processor_set default_pset;
struct processor processor_array[NCPUS];

queue_head_t		all_psets;
int			all_psets_count;
decl_simple_lock_data(, all_psets_lock);

processor_t	master_processor;
processor_t	processor_ptr[NCPUS];

/*
 *	Bootstrap the processor/pset system so the scheduler can run.
 */
void pset_sys_bootstrap()
{
	register int	i;
	void pset_init(), processor_init();

	pset_init(&default_pset);
	default_pset.empty = FALSE;
	for (i = 0; i < NCPUS; i++) {
		/*
		 *	Initialize processor data structures.
		 *	Note that cpu_to_processor(i) is processor_ptr[i].
		 */
		processor_ptr[i] = &processor_array[i];
		processor_init(processor_ptr[i], i);
	}
	master_processor = cpu_to_processor(master_cpu);
	queue_init(&all_psets);
	simple_lock_init(&all_psets_lock);
	queue_enter(&all_psets, &default_pset, processor_set_t, all_psets);
	all_psets_count = 1;
	default_pset.active = TRUE;
	default_pset.empty = FALSE;

	/*
	 *	Note: the default_pset has a max_priority of BASEPRI_USER.
	 *	Internal kernel threads override this in kernel_thread.
	 */
}

#if	MACH_HOST
/*
 *	Rest of pset system initializations.
 */
void pset_sys_init()
{
	pset_zone = zinit(sizeof(struct processor_set), 128*PAGE_SIZE,
		PAGE_SIZE, "processor sets");
}
#endif	MACH_HOST

/*
 *	Initialize the given processor_set structure.
 */

void pset_init(pset)
register processor_set_t	pset;
{
	int	i;

	simple_lock_init(&pset->runq.lock);
#if RT_SCHED_RQ
	/*
	 * When the run queue count is 0, runq.low should point to the lowest
	 * priority run queue (indicates no need for preemption).
	 */
	pset->runq.low = BASEPRI_LOWEST;
#else /* RT_SCHED_RQ */
	pset->runq.low = 0;
#endif /* RT_SCHED_RQ */
	pset->runq.count = 0;
	for (i = 0; i < NRQS; i++) {
	    queue_init(&(pset->runq.runq[i]));
	}
#if RT_SCHED_RQ
	pset->runq.mask.bits[0] = 0;
#ifndef __alpha
	pset->runq.mask.bits[1] = 0;
#endif
#endif /* RT_SCHED_RQ */
	queue_init(&pset->idle_queue);
	pset->idle_count = 0;
	simple_lock_init(&pset->idle_lock);
	queue_init(&pset->processors);
	pset->processor_count = 0;
	pset->empty = TRUE;
	queue_init(&pset->tasks);
	pset->task_count = 0;
	queue_init(&pset->threads);
	pset->thread_count = 0;
	pset->ref_count = 1;
	queue_init(&pset->all_psets);
	pset->active = FALSE;
	simple_lock_init(&pset->lock);
	pset->pset_self = PORT_NULL;
	pset->pset_name_self = PORT_NULL;
	pset->max_priority = BASEPRI_USER;
	pset->policies = POLICY_TIMESHARE;
#if RT_SCHED
	/*
	 * To make RT policies dynamically configurable, test a doconfig option
	 * here.
	 */
	pset->policies |= POLICY_RR|POLICY_FIFO;
#endif /* RT_SCHED */
	pset->set_quantum = min_quantum;
#if	NCPUS > 1
	pset->quantum_adj_index = 0;
	simple_lock_init(&pset->quantum_adj_lock);

	for (i = 0; i <= NCPUS; i++) {
	    pset->machine_quantum[i] = min_quantum;
	}
#endif	NCPUS > 1
	pset->mach_factor = 0;
	pset->load_average = 0;
	pset->sched_load = SCHED_SCALE;		/* i.e. 1 */
}

/*
 *	Initialize the given processor structure for the processor in
 *	the slot specified by slot_num.
 */

void processor_init(pr, slot_num)
register processor_t	pr;
int	slot_num;
{
	int	i;

	simple_lock_init(&pr->runq.lock);
#if RT_SCHED_RQ
	/*
	 * When queues are empty, (count=0), set low to lowest priority.  This
	 * indicates that nothing needs to preempt.
	 */
	pr->runq.low = BASEPRI_LOWEST;
#else /* RT_SCHED_RQ */
	pr->runq.low = 0;
#endif /* RT_SCHED_RQ */
	pr->runq.count = 0;
	for (i = 0; i < NRQS; i++) {
	    queue_init(&(pr->runq.runq[i]));
	}
#if RT_SCHED_RQ
	pr->runq.mask.bits[0] = 0;
#ifndef __alpha
	pr->runq.mask.bits[1] = 0;
#endif
#endif /* RT_SCHED_RQ */
	queue_init(&pr->processor_queue);
	pr->state = PROCESSOR_OFF_LINE;
	pr->next_thread = THREAD_NULL;
	pr->idle_thread = THREAD_NULL;
	pr->quantum = 0;
	pr->first_quantum = FALSE;
#if RT_SCHED
	pr->was_first_quantum = FALSE;
#endif /* RT_SCHED */
	pr->last_quantum = 0;
	pr->processor_set = PROCESSOR_SET_NULL;
	pr->processor_set_next = PROCESSOR_SET_NULL;
	queue_init(&pr->processors);
	simple_lock_init(&pr->lock);
	pr->processor_self = PORT_NULL;
	pr->slot_num = slot_num;
}

/*
 *	pset_remove_processor() removes a processor from a processor_set.
 *	It can only be called on the current processor.  Caller must
 *	hold lock on current processor and processor set.
 */

pset_remove_processor(pset, processor)
processor_set_t	pset;
processor_t	processor;
{
	if (pset != processor->processor_set)
		panic("pset_remove_processor: wrong pset");

	queue_remove(&pset->processors, processor, processor_t, processors);
	processor->processor_set = PROCESSOR_SET_NULL;
	pset->processor_count--;
	quantum_set(pset);
}

/*
 *	pset_add_processor() adds a  processor to a processor_set.
 *	It can only be called on the current processor.  Caller must
 *	hold lock on curent processor and on pset.  No reference counting on
 *	processors.  Processor reference to pset is implicit.
 */

pset_add_processor(pset, processor)
processor_set_t	pset;
processor_t	processor;
{
	queue_enter(&pset->processors, processor, processor_t, processors);
	processor->processor_set = pset;
	pset->processor_count++;
	quantum_set(pset);
}

/*
 *	pset_remove_task() removes a task from a processor_set.
 *	Caller must hold locks on pset and task.  Pset reference count
 *	is not decremented; caller must explicitly pset_deallocate.
 */

pset_remove_task(pset, task)
processor_set_t	pset;
task_t	task;
{
	if (pset != task->processor_set)
		return;

	queue_remove(&pset->tasks, task, task_t, pset_tasks);
	task->processor_set = PROCESSOR_SET_NULL;
	pset->task_count--;
}

/*
 *	pset_add_task() adds a  task to a processor_set.
 *	Caller must hold locks on pset and task.  Pset references to
 *	tasks are implicit.
 */

pset_add_task(pset, task)
processor_set_t	pset;
task_t	task;
{
	queue_enter(&pset->tasks, task, task_t, pset_tasks);
	task->processor_set = pset;
	pset->task_count++;
	pset->ref_count++;
}

/*
 *	pset_remove_thread() removes a thread from a processor_set.
 *	Caller must hold locks on pset and thread.  Pset reference count
 *	is not decremented; caller must explicitly pset_deallocate.
 */

pset_remove_thread(pset, thread)
processor_set_t	pset;
thread_t	thread;
{
	if (pset != thread->processor_set)
		return;

	queue_remove(&pset->threads, thread, thread_t, pset_threads);
	thread->processor_set = PROCESSOR_SET_NULL;
	pset->thread_count--;
}

/*
 *	pset_add_thread() adds a  thread to a processor_set.
 *	Caller must hold locks on pset and thread.  Pset references to
 *	threads are implicit.
 */

pset_add_thread(pset, thread)
processor_set_t	pset;
thread_t	thread;
{
	queue_enter(&pset->threads, thread, thread_t, pset_threads);
	thread->processor_set = pset;
	pset->thread_count++;
	pset->ref_count++;
}

/*
 *	pset_deallocate:
 *
 *	Remove one reference to the processor set.  Destroy processor_set
 *	if this was the last reference.
 */
pset_deallocate(pset)
processor_set_t	pset;
{
	pset_lock(pset);
	if (--pset->ref_count > 0) {
		pset_unlock(pset);
		return;
	}
#if	!MACH_HOST
	panic("pset_deallocate: default_pset destroyed");
#endif	!MACH_HOST
#if	MACH_HOST
	/*
	 *	Reference count is zero, however the all_psets list
	 *	holds an implicit reference and may make new ones.
	 *	Its lock also dominates the pset lock.  To check for this,
	 *	temporarily restore one reference, and then lock the
	 *	other structures in the right order.
	 */
	pset->ref_count = 1;
	pset_unlock(pset);
	
	simple_lock(&all_psets_lock);
	pset_lock(pset);
	if (--pset->ref_count > 0) {
		/*
		 *	Made an extra reference.
		 */
		pset_unlock(pset);
		simple_unlock(&all_psets_lock);
		return;
	}

	/*
	 *	Ok to destroy pset.  Make a few paranoia checks.
	 */

	if ((pset == &default_pset) || (pset->thread_count > 0) ||
	    (pset->task_count > 0)) {
		panic("pset_deallocate: destroy default or active pset");
	}
	/*
	 *	Remove from all_psets queue.
	 */
	queue_remove(&all_psets, pset, processor_set_t, all_psets);
	all_psets_count--;

	pset_unlock(pset);
	simple_unlock(&all_psets_lock);

	/*
	 *	That's it, free data structure.
	 */
	zfree(pset_zone, pset);
#endif	MACH_HOST
}

/*
 *	pset_reference:
 *
 *	Add one reference to the processor set.
 */
pset_reference(pset)
processor_set_t	pset;
{
	pset_lock(pset);
	pset->ref_count++;
	pset_unlock(pset);
}

kern_return_t processor_info(processor, flavor, host, info, count)
register processor_t	processor;
int			flavor;
host_t			*host;
processor_info_t	info;
unsigned int		*count;
{
	register int	slot_num, state;
	register processor_basic_info_t		basic_info;

	if (processor == PROCESSOR_NULL)
		return(KERN_INVALID_ARGUMENT);

	if (flavor != PROCESSOR_BASIC_INFO ||
		*count < PROCESSOR_BASIC_INFO_COUNT)
			return(KERN_FAILURE);

	basic_info = (processor_basic_info_t) info;

	slot_num = processor->slot_num;
	basic_info->cpu_type = machine_slot[slot_num].cpu_type;
	basic_info->cpu_subtype = machine_slot[slot_num].cpu_subtype;
	state = processor->state;
	if (state == PROCESSOR_SHUTDOWN || state == PROCESSOR_OFF_LINE)
		basic_info->running = FALSE;
	else
		basic_info->running = TRUE;
	basic_info->slot_num = slot_num;
	if (processor == master_processor) 
		basic_info->is_master = TRUE;
	else
		basic_info->is_master = FALSE;

	*count = PROCESSOR_BASIC_INFO_COUNT;
	*host = &realhost;
	return(KERN_SUCCESS);
}

kern_return_t processor_start(processor)
processor_t	processor;
{
    	if (processor == PROCESSOR_NULL)
		return(KERN_INVALID_ARGUMENT);
#if	NCPUS > 1
	return(cpu_start(processor->slot_num));
#else	NCPUS > 1
	return(KERN_FAILURE);
#endif	NCPUS > 1
}

kern_return_t processor_exit(processor)
processor_t	processor;
{
	if (processor == PROCESSOR_NULL)
		return(KERN_INVALID_ARGUMENT);

#if	NCPUS > 1
	return(processor_shutdown(processor));
#else	NCPUS > 1
	return(KERN_FAILURE);
#endif	NCPUS > 1
}

kern_return_t processor_control(processor, info, count)
processor_t		processor;
processor_info_t	info;
long			*count;
{
	if (processor == PROCESSOR_NULL)
		return(KERN_INVALID_ARGUMENT);

#if	NCPUS > 1
	return(cpu_control(processor->slot_num, (int *)info, *count));
#else	NCPUS > 1
	return(KERN_FAILURE);
#endif	NCPUS > 1
}

/*
 *	Precalculate the appropriate system quanta based on load.  The
 *	index into machine_quantum is the number of threads on the
 *	processor set queue.  It is limited to the number of processors in
 *	the set.
 */

quantum_set(pset)
processor_set_t	pset;
{
#if	NCPUS > 1
	register int	i,ncpus;

	ncpus = pset->processor_count;

	for ( i=1 ; i <= ncpus ; i++) {
		pset->machine_quantum[i] =
			((min_quantum * ncpus) + (i/2)) / i ;
	}
	pset->machine_quantum[0] = 2 * pset->machine_quantum[1];

	i = ((pset->runq.count > pset->processor_count) ?
		pset->processor_count : pset->runq.count);
	pset->set_quantum = pset->machine_quantum[i];
#else	NCPUS > 1
	default_pset.set_quantum = min_quantum;
#endif	NCPUS > 1
}

#if	MACH_HOST
/*
 *	processor_set_create:
 *
 *	Create and return a new processor set.
 */

kern_return_t	processor_set_create(host, new_set, new_name)
host_t	host;
processor_set_t	*new_set, *new_name;
{
	processor_set_t	pset;

	if (host == HOST_NULL)
		return(KERN_INVALID_ARGUMENT);
	pset = (processor_set_t) zalloc(pset_zone);
	pset_init(pset);
	ipc_pset_init(pset);
	pset->active = TRUE;

	simple_lock(&all_psets_lock);
	queue_enter(&all_psets, pset, processor_set_t, all_psets);
	all_psets_count++;
	simple_unlock(&all_psets_lock);

	ipc_pset_enable(pset);

	/*
	 *	return new set as both set and name; MiG code will
	 *	translate correctly.
	 */
	*new_set = pset;
	*new_name = pset;
	return(KERN_SUCCESS);
}

/*
 *	processor_set_destroy:
 *
 *	destroy a processor set.  Any tasks, threads or processors
 *	currently assigned to it are reassigned to the default pset.
 */
kern_return_t processor_set_destroy(pset)
processor_set_t	pset;
{
	register queue_entry_t	elem;
	register queue_head_t	*list;

	if (pset == PROCESSOR_SET_NULL || pset == &default_pset)
		return(KERN_INVALID_ARGUMENT);

	/*
	 *	Handle multiple termination race.  First one through sets
	 *	active to FALSE.
	 */
	pset_lock(pset);
	if (!(pset->active)) {
		pset_unlock(pset);
		return(KERN_FAILURE);
	}
	pset->active = FALSE;
	pset_unlock(pset);

	/*
	 *	Disable ipc access.
	 */
	ipc_pset_disable(pset);


	/*
	 *	Now reassign everything in this set to the default set.
	 */
	pset_lock(pset);
	list = &pset->tasks;
	while (!queue_empty(list)) {
		elem = queue_first(list);
		task_reference((task_t) elem);
		pset_unlock(pset);
		task_assign((task_t) elem, &default_pset, FALSE);
		task_deallocate((task_t) elem);
		pset_lock(pset);

	}

	list = &pset->threads;
	while (!queue_empty(list)) {
		elem = queue_first(list);
		thread_reference((thread_t) elem);
		pset_unlock(pset);
		thread_assign((thread_t) elem, &default_pset);
		thread_deallocate((thread_t) elem);
		pset_lock(pset);

	}

	list = &pset->processors;
	while(!queue_empty(list)) {
		elem = queue_first(list);
		pset_unlock(pset);
		processor_assign((processor_t) elem, &default_pset, TRUE);
		pset_lock(pset);
	}

	pset_unlock(pset);

	/*
	 *	Destroy ipc state.
	 */
	ipc_pset_terminate(pset);

	/*
	 *	Deallocat pset's reference to itself.
	 */
	pset_deallocate(pset);
	return(KERN_SUCCESS);
}

#else	MACH_HOST
	    
kern_return_t	processor_set_create(host, new_set, new_name)
host_t	host;
processor_set_t	*new_set, *new_name;
{
#ifdef	lint
	host++; new_set++; new_name++;
#endif	lint
	return(KERN_FAILURE);
}

kern_return_t processor_set_destroy(pset)
processor_set_t	pset;
{
#ifdef	lint
	pset++;
#endif	lint
	return(KERN_FAILURE);
}

#endif	MACH_HOST

kern_return_t processor_get_assignment(processor, pset)
processor_t	processor;
processor_set_t	*pset;
{
    	int state;

	state = processor->state;
	if (state == PROCESSOR_SHUTDOWN || state == PROCESSOR_OFF_LINE)
		return(KERN_FAILURE);

	*pset = processor->processor_set;
	return(KERN_SUCCESS);
}

kern_return_t processor_set_info(pset, flavor, host, info, count)
processor_set_t		pset;
int			flavor;
host_t			*host;
processor_set_info_t	info;
unsigned int		*count;
{
	if (pset == PROCESSOR_SET_NULL)
		return(KERN_INVALID_ARGUMENT);

	if (flavor == PROCESSOR_SET_BASIC_INFO) {
		register processor_set_basic_info_t	basic_info;

		if (*count < PROCESSOR_SET_BASIC_INFO_COUNT)
			return(KERN_FAILURE);

		basic_info = (processor_set_basic_info_t) info;

		pset_lock(pset);
		basic_info->processor_count = pset->processor_count;
		basic_info->task_count = pset->task_count;
		basic_info->thread_count = pset->thread_count;
		basic_info->mach_factor = pset->mach_factor;
		basic_info->load_average = pset->load_average;
		pset_unlock(pset);

		*count = PROCESSOR_SET_BASIC_INFO_COUNT;
		*host = &realhost;
		return(KERN_SUCCESS);
	}
	else if (flavor == PROCESSOR_SET_SCHED_INFO) {
		register processor_set_sched_info_t	sched_info;

		if (*count < PROCESSOR_SET_SCHED_INFO_COUNT)
			return(KERN_FAILURE);

		sched_info = (processor_set_sched_info_t) info;

		pset_lock(pset);
		sched_info->policies = pset->policies;
		sched_info->max_priority = pset->max_priority;
		pset_unlock(pset);

		*count = PROCESSOR_SET_BASIC_INFO_COUNT;
		*host = &realhost;
		return(KERN_SUCCESS);
	}

	*host = HOST_NULL;
	return(KERN_INVALID_ARGUMENT);
}

/*
 *	processor_set_max_priority:
 *
 *	Specify max priority permitted on processor set.  This affects
 *	newly created and assigned threads.  Optionally change existing
 * 	ones.
 */
kern_return_t
processor_set_max_priority(pset, max_priority, change_threads)
processor_set_t	pset;
int		max_priority;
boolean_t	change_threads;
{
	if (pset == PROCESSOR_SET_NULL || invalid_pri(max_priority))
		return(KERN_INVALID_ARGUMENT);

	pset_lock(pset);
	pset->max_priority = max_priority;

	if (change_threads) {
	    register queue_head_t *list;
	    register thread_t	thread;

	    list = &pset->threads;
	    thread = (thread_t) queue_first(list);
	    while (!queue_end(list, (queue_entry_t) thread)) {
		if (thread->max_priority < max_priority)
			thread_max_priority(thread, pset, max_priority, TRUE);

		thread = (thread_t) queue_next(&thread->pset_threads);
	    }
	}

	pset_unlock(pset);

	return(KERN_SUCCESS);
}

/*
 *	processor_set_policy_enable:
 *
 *	Allow indicated policy on processor set.
 */

kern_return_t
processor_set_policy_enable(pset, policy)
processor_set_t	pset;
int	policy;
{
	if ((pset == PROCESSOR_SET_NULL) || invalid_policy(policy))
		return(KERN_INVALID_ARGUMENT);

	pset_lock(pset);
	pset->policies |= policy;
	pset_unlock(pset);
	
    	return(KERN_SUCCESS);
}

/*
 *	processor_set_policy_disable:
 *
 *	Forbid indicated policy on processor set.  Time sharing cannot
 *	be forbidden.
 */

kern_return_t
processor_set_policy_disable(pset, policy, change_threads)
processor_set_t	pset;
int	policy;
boolean_t	change_threads;
{
	if ((pset == PROCESSOR_SET_NULL) || policy == POLICY_TIMESHARE ||
	    invalid_policy(policy))
		return(KERN_INVALID_ARGUMENT);

	pset_lock(pset);

	/*
	 *	Check if policy enabled.  Disable if so, then handle
	 *	change_threads.
	 */
	if (pset->policies & policy) {
	    pset->policies &= ~policy;

	    if (change_threads) {
		register queue_head_t	*list;
		register thread_t	thread;

		list = &pset->threads;
		thread = (thread_t) queue_first(list);
		while (!queue_end(list, (queue_entry_t) thread)) {
		    if (thread->policy == policy)
			thread_policy(thread, POLICY_TIMESHARE, 0);

		    thread = (thread_t) queue_next(&thread->pset_threads);
		}
	    }
	}
	pset_unlock(pset);

	return(KERN_SUCCESS);
}

#define THING_TASK	0
#define THING_THREAD	1

/*
 *	processor_set_things:
 *
 *	Common internals for processor_set_{threads,tasks}
 */
kern_return_t
processor_set_things(pset, thing_list, count, type)
processor_set_t	pset;
port_t		**thing_list;
unsigned int	*count;
int		type;
{
	unsigned int actual;	/* this many things */
	port_t *things;
	int i;

	vm_size_t size;
	vm_offset_t addr;

	/*
	 *	Need a type-breaking union and some cpp magic.
	 */
	union {
	    thread_t	foo_thread;
	    task_t	foo_task;
	} foo;
#define thread	(foo.foo_thread)
#define task	(foo.foo_task)

	if (pset == PROCESSOR_SET_NULL)
		return KERN_INVALID_ARGUMENT;

	size = 0; addr = 0;

	for (;;) {
		vm_size_t size_needed;

		pset_lock(pset);
		if (!pset->active) {
			pset_unlock(pset);
			return KERN_FAILURE;
		}

		if (type == THING_TASK)
			actual = pset->task_count;
		else
			actual = pset->thread_count;

		/* do we have the memory we need? */

		size_needed = actual * sizeof(port_t);
		if (size_needed <= size)
			break;

		/* unlock the task and allocate more memory */
		pset_unlock(pset);

		if (size != 0)
			(void) kmem_free(ipc_kernel_map, addr, size);

		size = round_page(2 * size_needed);

		/* allocate memory non-pageable, so no faults
		   while holding locks */

		(void) vm_allocate(ipc_kernel_map, &addr, size, TRUE);
		(void) vm_map_pageable(ipc_kernel_map, addr, addr + size,
			VM_PROT_READ|VM_PROT_WRITE);
	}

	/* OK, have memory and the processor_set is locked & active */

	things = (port_t *) addr;

	switch (type) {
	    case THING_TASK:
		for (i = 0, task = (task_t) queue_first(&pset->tasks);
		     i < actual;
		     i++, task = (task_t) queue_next(&task->pset_tasks))
			things[i] = convert_task_to_port(task);
		assert(queue_end(&pset->tasks, (queue_entry_t) task));
		break;

	    case THING_THREAD:
		for (i = 0, thread = (thread_t) queue_first(&pset->threads);
		     i < actual;
		     i++,
		     thread = (thread_t) queue_next(&thread->pset_threads))
			things[i] = convert_thread_to_port(thread);
		assert(queue_end(&pset->threads, (queue_entry_t) thread));
		break;
	}

	/* can unlock processor set now that we have the ports */
	pset_unlock(pset);

	if (actual == 0) {
		/* no things, so return null pointer and deallocate memory */
		*thing_list = 0;
		*count = 0;

		if (size != 0)
			(void) kmem_free(ipc_kernel_map, addr, size);
	} else {
		vm_size_t size_used;

		*thing_list = things;
		*count = actual;

		size_used = round_page(actual * sizeof(port_t));

		/* finished touching it, so make the memory pageable */
		(void) vm_map_pageable(ipc_kernel_map,
				       addr, addr + size_used, VM_PROT_NONE);

		/* free any unused memory */
		if (size_used != size)
			(void) kmem_free(ipc_kernel_map,
					 addr + size_used, size - size_used);
	}

	return(KERN_SUCCESS);
}


/*
 *	processor_set_tasks:
 *
 *	List all tasks in the processor set.
 */
kern_return_t
processor_set_tasks(pset, task_list, count)
processor_set_t	pset;
task_array_t	*task_list;
unsigned int	*count;
{
    return(processor_set_things(pset, task_list, count, THING_TASK));
}

/*
 *	processor_set_threads:
 *
 *	List all threads in the processor set.
 */
kern_return_t
processor_set_threads(pset, thread_list, count)
processor_set_t	pset;
thread_array_t	*thread_list;
unsigned int	*count;
{
    return(processor_set_things(pset, thread_list, count, THING_THREAD));
}
