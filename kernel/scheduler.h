#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"

// Scheduler constants inspired by Linux 0.01
#define HZ 100                    // Timer frequency (100 Hz)
#define TIMER_INTERVAL (1000/HZ)  // Timer interval in ms
#define DEFAULT_PRIORITY 15       // Default task priority
#define MAX_PRIORITY 40          // Maximum priority value
#define MIN_PRIORITY 0           // Minimum priority value

// Time slice constants
#define DEFAULT_TIME_SLICE 10    // Default time slice in timer ticks
#define MIN_TIME_SLICE 1         // Minimum time slice
#define MAX_TIME_SLICE 50        // Maximum time slice

// Scheduler states
typedef enum {
    SCHED_RUNNING,    // Scheduler is active
    SCHED_STOPPED,    // Scheduler is stopped
    SCHED_PAUSED      // Scheduler is paused
} scheduler_state_t;

// Scheduling policy types
typedef enum {
    SCHED_ROUND_ROBIN,  // Round-robin scheduling
    SCHED_PRIORITY,     // Priority-based scheduling
    SCHED_FIFO          // First-in-first-out
} sched_policy_t;

// Scheduler statistics
typedef struct {
    unsigned int total_switches;     // Total context switches
    unsigned int idle_time;          // Time spent in idle
    unsigned int active_time;        // Time spent running tasks
    unsigned int timer_ticks;        // Total timer ticks
    unsigned int preemptions;        // Number of preemptions
    task_t* last_scheduled;          // Last scheduled task
} scheduler_stats_t;

// Main scheduler structure
typedef struct {
    task_t* current_task;           // Currently running task
    task_t* idle_task;              // Idle task
    scheduler_state_t state;        // Scheduler state
    sched_policy_t policy;          // Scheduling policy
    unsigned int time_slice;        // Current time slice
    unsigned int ticks_remaining;   // Remaining ticks for current task
    scheduler_stats_t stats;        // Scheduler statistics
} scheduler_t;

// Global scheduler instance
extern scheduler_t scheduler;

// Core scheduler functions
void init_scheduler(void);
void schedule(void);
void scheduler_tick(void);
void yield(void);

// Task management functions
void add_task_to_scheduler(task_t* task);
void remove_task_from_scheduler(task_t* task);
task_t* get_next_task(void);
void switch_to_task(task_t* task);

// Priority and time slice management
void set_task_priority(task_t* task, int priority);
void adjust_priority(task_t* task);
void calculate_time_slice(task_t* task);
void update_task_runtime(task_t* task, unsigned int ticks);

// Scheduler control functions
void start_scheduler(void);
void stop_scheduler(void);
void pause_scheduler(void);
void resume_scheduler(void);
void set_scheduling_policy(sched_policy_t policy);

// Statistics and monitoring
void print_scheduler_stats(void);
void reset_scheduler_stats(void);
scheduler_stats_t get_scheduler_stats(void);
void print_task_queue(void);

// Idle task functions
void create_idle_task(void);
void idle_task_function(void);

// Context switching (implemented in assembly)
extern void switch_context(task_t* from, task_t* to);
extern void save_context(task_t* task);
extern void restore_context(task_t* task);

// Timer integration
void setup_scheduler_timer(void);
void scheduler_timer_handler(void);

// Utility functions
int is_scheduler_running(void);
int can_preempt_current_task(void);
void force_schedule(void);
task_t* find_highest_priority_task(void);

// Debug functions
void debug_scheduler_state(void);
void validate_scheduler_state(void);

#endif // SCHEDULER_H