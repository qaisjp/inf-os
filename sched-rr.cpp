/*
 * Round-robin Scheduling Algorithm
 * SKELETON IMPLEMENTATION -- TO BE FILLED IN FOR TASK (1)
 */

/*
 * STUDENT NUMBER: s1620208
 */
#include <infos/kernel/sched.h>
#include <infos/kernel/thread.h>
#include <infos/kernel/log.h>
#include <infos/util/list.h>
#include <infos/util/lock.h>

using namespace infos::kernel;
using namespace infos::util;

/**
 * A round-robin scheduling algorithm
 */
class RoundRobinScheduler : public SchedulingAlgorithm
{
public:
	/**
	 * Returns the friendly name of the algorithm, for debugging and selection purposes.
	 */
	const char* name() const override { return "rr"; }

	/**
	 * Called when a scheduling entity becomes eligible for running.
	 * @param entity
	 */
	void add_to_runqueue(SchedulingEntity& entity) override
	{
		// You must make sure that interrupts are
		// disabled when manipulating the runqueue.
		UniqueIRQLock l;

		runqueue.enqueue(&entity);
	}

	/**
	 * Called when a scheduling entity is no longer eligible for running.
	 * @param entity
	 */
	void remove_from_runqueue(SchedulingEntity& entity) override
	{
		// You must make sure that interrupts are
		// disabled when manipulating the runqueue.
		UniqueIRQLock l;

		runqueue.remove(&entity);
	}

	/**
	 * Called every time a scheduling event occurs, to cause the next eligible entity
	 * to be chosen.  The next eligible entity might actually be the same entity, if
	 * e.g. its timeslice has not expired.
	 *
	 * In our case, when a new task is picked for execution, it is removed
	 * from the front of the list, and placed at the back.
	 * Then, this task is allow to run for its timeslice.
	 */
	SchedulingEntity *pick_next_entity() override
	{
		// If there's nothing in our queue, return nothing
		if (runqueue.empty()) {
			return nullptr;
		}

		// Don't bother with the overhead if we only have one item
		if (runqueue.count() == 1) {
			return runqueue.first();
		}

		// You must make sure that interrupts are
		// disabled when manipulating the runqueue.
		UniqueIRQLock l;

		// Remove from the front of the list
		auto entity = runqueue.dequeue();

		// Add it to the end of the list
		runqueue.enqueue(entity);

		// Return our entity
		return entity;
	}

private:
	// A list containing the current runqueue.
	List<SchedulingEntity *> runqueue;
};

/* --- DO NOT CHANGE ANYTHING BELOW THIS LINE --- */

RegisterScheduler(RoundRobinScheduler);
