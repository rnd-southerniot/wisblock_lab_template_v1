/**
 * @file scheduler.cpp
 * @brief Cooperative task scheduler — Implementation
 * @version 1.0
 * @date 2026-02-24
 *
 * millis()-based tick dispatch. Handles wraparound via unsigned subtraction.
 * No dynamic allocation, no RTOS dependencies.
 */

#include <Arduino.h>
#include "scheduler.h"

TaskScheduler::TaskScheduler() : m_count(0) {
    for (uint8_t i = 0; i < SCHEDULER_MAX_TASKS; i++) {
        m_tasks[i].callback    = nullptr;
        m_tasks[i].interval_ms = 0;
        m_tasks[i].last_run_ms = 0;
        m_tasks[i].active      = false;
        m_tasks[i].name        = nullptr;
    }
}

int8_t TaskScheduler::registerTask(scheduler_callback_t callback,
                                uint32_t interval_ms,
                                const char* name) {
    if (m_count >= SCHEDULER_MAX_TASKS || callback == nullptr) {
        return -1;
    }

    uint8_t idx = m_count;
    m_tasks[idx].callback    = callback;
    m_tasks[idx].interval_ms = interval_ms;
    m_tasks[idx].last_run_ms = millis();  /* Start counting from now */
    m_tasks[idx].active      = true;
    m_tasks[idx].name        = name;
    m_count++;

    return (int8_t)idx;
}

void TaskScheduler::tick() {
    uint32_t now = millis();

    for (uint8_t i = 0; i < m_count; i++) {
        if (!m_tasks[i].active || m_tasks[i].callback == nullptr) {
            continue;
        }

        /* Unsigned subtraction handles millis() wraparound correctly */
        if ((now - m_tasks[i].last_run_ms) >= m_tasks[i].interval_ms) {
            m_tasks[i].last_run_ms = now;
            m_tasks[i].callback();
        }
    }
}

uint8_t TaskScheduler::taskCount() const {
    return m_count;
}

const char* TaskScheduler::taskName(uint8_t index) const {
    if (index >= m_count) {
        return nullptr;
    }
    return m_tasks[index].name;
}

void TaskScheduler::fireNow(uint8_t index) {
    if (index >= m_count || !m_tasks[index].active || m_tasks[index].callback == nullptr) {
        return;
    }
    m_tasks[index].last_run_ms = millis();
    m_tasks[index].callback();
}
