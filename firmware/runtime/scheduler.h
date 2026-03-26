/**
 * @file scheduler.h
 * @brief Cooperative task scheduler — millis()-based periodic dispatch
 * @version 1.0
 * @date 2026-02-24
 *
 * Lightweight cooperative scheduler for Arduino loop()-based firmware.
 * Register callbacks with intervals; tick() dispatches ready tasks.
 * No RTOS, no dynamic allocation, no blocking.
 *
 * Part of firmware/runtime/ production abstraction layer.
 */

#pragma once
#include <stdint.h>

typedef void (*scheduler_callback_t)(void);

#define SCHEDULER_MAX_TASKS  8

struct SchedulerTask {
    scheduler_callback_t callback;
    uint32_t interval_ms;
    uint32_t last_run_ms;
    bool     active;
    const char* name;
};

class TaskScheduler {
public:
    TaskScheduler();

    /**
     * Register a periodic task.
     * @param callback  Function pointer — must be non-blocking
     * @param interval_ms  Minimum interval between calls (millis-based)
     * @param name  Human-readable name for logging
     * @return Task index (0..MAX-1), or -1 if full
     */
    int8_t registerTask(scheduler_callback_t callback,
                        uint32_t interval_ms,
                        const char* name);

    /**
     * Call from loop(). Checks all tasks, fires those whose interval has elapsed.
     * Non-blocking — returns immediately after dispatching ready tasks.
     */
    void tick();

    /** Number of registered tasks */
    uint8_t taskCount() const;

    /** Get task name by index */
    const char* taskName(uint8_t index) const;

    /** Force-fire a specific task immediately (for gate testing) */
    void fireNow(uint8_t index);

    /** Change interval of a registered task. Returns true if index valid. */
    bool setInterval(uint8_t index, uint32_t new_interval_ms);

    /** Get current interval of a registered task. Returns 0 if index invalid. */
    uint32_t taskInterval(uint8_t index) const;
    /**
     * Change the interval of a registered task at runtime.
     * @param index  Task index (from registerTask return value)
     * @param interval_ms  New interval in milliseconds
     * @return true if task index is valid and interval was set
     */
    bool setTaskInterval(uint8_t index, uint32_t interval_ms);

private:
    SchedulerTask m_tasks[SCHEDULER_MAX_TASKS];
    uint8_t m_count;
};
