#include "scheduler.h"
#include "kernel.h"
#include "memory.h"
#include "interrupt.h"

// Global scheduler instance
scheduler_t scheduler;

// Task queue for round-robin scheduling
static task_t* task_queue_head = 0;
static task_t* task_queue_tail = 0;

/**
 * Initialize the scheduler system
 * مستوحى من Linux kernel 0.01 sched.c
 */
void init_scheduler(void) {
    // Initialize scheduler structure
    scheduler.current_task = 0;
    scheduler.idle_task = 0;
    scheduler.state = SCHED_STOPPED;
    scheduler.policy = SCHED_ROUND_ROBIN;
    scheduler.time_slice = DEFAULT_TIME_SLICE;
    scheduler.ticks_remaining = DEFAULT_TIME_SLICE;
    
    // Initialize statistics
    scheduler.stats.total_switches = 0;
    scheduler.stats.idle_time = 0;
    scheduler.stats.active_time = 0;
    scheduler.stats.timer_ticks = 0;
    scheduler.stats.preemptions = 0;
    scheduler.stats.last_scheduled = 0;
    
    // Initialize task queue
    task_queue_head = 0;
    task_queue_tail = 0;
    
    // Create idle task
    create_idle_task();
    
    // Setup scheduler timer
    setup_scheduler_timer();
}

// Note: schedule() function is already implemented in task.c

/**
 * Called on each timer tick
 * مستوحى من Linux kernel 0.01 timer interrupt handler
 */
void scheduler_tick(void) {
    scheduler.stats.timer_ticks++;
    
    if (scheduler.state != SCHED_RUNNING) {
        return;
    }
    
    // Update current task runtime
    if (scheduler.current_task) {
        update_task_runtime(scheduler.current_task, 1);
        
        // Decrease remaining time slice
        if (scheduler.ticks_remaining > 0) {
            scheduler.ticks_remaining--;
        }
        
        // Check if time slice expired
        if (scheduler.ticks_remaining == 0) {
            scheduler.stats.preemptions++;
            yield();
        }
    }
}

/**
 * Yield CPU to next task
 * مستوحى من Linux kernel 0.01
 */
void yield(void) {
    if (scheduler.current_task) {
        scheduler.current_task->state = TASK_READY;
    }
    schedule();
}

/**
 * Add task to scheduler queue
 */
void add_task_to_scheduler(task_t* task) {
    if (!task) return;
    
    task->next = 0;
    task->state = TASK_READY;
    
    if (!task_queue_head) {
        task_queue_head = task;
        task_queue_tail = task;
    } else {
        task_queue_tail->next = task;
        task_queue_tail = task;
    }
}

/**
 * Remove task from scheduler queue
 */
void remove_task_from_scheduler(task_t* task) {
    if (!task || !task_queue_head) return;
    
    if (task_queue_head == task) {
        task_queue_head = task->next;
        if (task_queue_tail == task) {
            task_queue_tail = 0;
        }
        return;
    }
    
    task_t* current = task_queue_head;
    while (current->next && current->next != task) {
        current = current->next;
    }
    
    if (current->next == task) {
        current->next = task->next;
        if (task_queue_tail == task) {
            task_queue_tail = current;
        }
    }
}

// Forward declarations for internal functions
static task_t* get_next_round_robin_task(void);
static task_t* get_next_fifo_task(void);

/**
 * Get next task to run based on scheduling policy
 */
task_t* get_next_task(void) {
    switch (scheduler.policy) {
        case SCHED_ROUND_ROBIN:
            return get_next_round_robin_task();
        case SCHED_PRIORITY:
            return find_highest_priority_task();
        case SCHED_FIFO:
            return get_next_fifo_task();
        default:
            return scheduler.idle_task;
    }
}

/**
 * Round-robin task selection
 */
static task_t* get_next_round_robin_task(void) {
    if (!task_queue_head) {
        return scheduler.idle_task;
    }
    
    // Find next ready task
    task_t* current = task_queue_head;
    task_t* start = current;
    
    do {
        if (current->state == TASK_READY) {
            return current;
        }
        current = current->next ? current->next : task_queue_head;
    } while (current != start);
    
    return scheduler.idle_task;
}

/**
 * FIFO task selection
 */
static task_t* get_next_fifo_task(void) {
    task_t* current = task_queue_head;
    
    while (current) {
        if (current->state == TASK_READY) {
            return current;
        }
        current = current->next;
    }
    
    return scheduler.idle_task;
}

// Note: switch_to_task() function is already implemented in task.c

/**
 * Set task priority
 */
void set_task_priority(task_t* task, int priority) {
    if (!task) return;
    
    if (priority < MIN_PRIORITY) priority = MIN_PRIORITY;
    if (priority > MAX_PRIORITY) priority = MAX_PRIORITY;
    
    task->priority = priority;
    calculate_time_slice(task);
}

/**
 * Calculate time slice based on priority
 */
void calculate_time_slice(task_t* task) {
    if (!task) return;
    
    // Higher priority = longer time slice
    int slice = DEFAULT_TIME_SLICE + (MAX_PRIORITY - task->priority);
    
    if (slice < MIN_TIME_SLICE) slice = MIN_TIME_SLICE;
    if (slice > MAX_TIME_SLICE) slice = MAX_TIME_SLICE;
    
    scheduler.time_slice = slice;
}

/**
 * Update task runtime statistics
 */
void update_task_runtime(task_t* task, unsigned int ticks) {
    if (!task) return;
    
    // Note: runtime tracking would require adding runtime field to task_t
    // For now, just update scheduler statistics
    
    if (task == scheduler.idle_task) {
        scheduler.stats.idle_time += ticks;
    } else {
        scheduler.stats.active_time += ticks;
    }
}

/**
 * Start the scheduler
 */
void start_scheduler(void) {
    scheduler.state = SCHED_RUNNING;
    
    // Start with idle task if no other tasks
    if (!scheduler.current_task) {
        scheduler.current_task = scheduler.idle_task;
    }
}

/**
 * Stop the scheduler
 */
void stop_scheduler(void) {
    scheduler.state = SCHED_STOPPED;
}

/**
 * Create idle task
 */
void create_idle_task(void) {
    scheduler.idle_task = create_task("idle", idle_task_function);
    if (scheduler.idle_task) {
        scheduler.idle_task->state = TASK_READY;
        // Note: priority setting would require adding priority field to task_t
        // Idle task created successfully
    } else {
        // Failed to create idle task
    }
}

/**
 * Idle task function - runs when no other tasks are ready
 */
void idle_task_function(void) {
    while (1) {
        // Simple idle loop
        asm volatile("hlt"); // Halt until next interrupt
    }
}

/**
 * Find highest priority ready task
 */
task_t* find_highest_priority_task(void) {
    task_t* best_task = 0;
    int highest_priority = MAX_PRIORITY + 1;
    
    task_t* current = task_queue_head;
    while (current) {
        if (current->state == TASK_READY && current->priority < highest_priority) {
            highest_priority = current->priority;
            best_task = current;
        }
        current = current->next;
    }
    
    return best_task ? best_task : scheduler.idle_task;
}

/**
 * Setup scheduler timer
 */
void setup_scheduler_timer(void) {
    // Timer setup will be integrated with interrupt system
    // This is a placeholder for timer configuration
}

/**
 * Print scheduler statistics
 */
void print_scheduler_stats(void) {
    // Scheduler statistics logged
    // Total switches, idle time, active time logged
}

/**
 * Check if scheduler is running
 */
int is_scheduler_running(void) {
    return scheduler.state == SCHED_RUNNING;
}

/**
 * Simple context switch implementation
 * In a real system, this would be in assembly
 */
void switch_context(task_t* from, task_t* to) {
    // Simplified context switch
    // In a real implementation, this would save/restore all registers
    if (from) {
        // Save current context (simplified)
        from->esp = 0; // Placeholder
    }
    
    if (to) {
        // Restore new context (simplified)
        // Load new stack pointer, etc.
    }
}

/**
 * Force immediate scheduling
 */
void force_schedule(void) {
    scheduler.ticks_remaining = 0;
    schedule();
}