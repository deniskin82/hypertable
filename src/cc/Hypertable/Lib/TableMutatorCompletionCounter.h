/** -*- c++ -*-
 * Copyright (C) 2007-2012 Hypertable, Inc.
 *
 * This file is part of Hypertable.
 *
 * Hypertable is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 3 of the
 * License, or any later version.
 *
 * Hypertable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef HYPERTABLE_TABLEMUTATORCOMPLETIONCOUNTER_H
#define HYPERTABLE_TABLEMUTATORCOMPLETIONCOUNTER_H

#include "Common/ReferenceCount.h"
#include "Common/Time.h"

namespace Hypertable {

  /**
   * Tracks outstanding RangeServer update requests.  This class is used to
   * track the state of outstanding RangeServer update requests for a scatter
   * send.  It is initialized with the number of updates issued.  When each
   * update returns or times out the counter is decremented and when all
   * updates have completed or timed out, an object of this class will signal
   * completion.
   */
  class TableMutatorCompletionCounter {
  public:
    TableMutatorCompletionCounter()
      : m_outstanding(0), m_retries(false), m_errors(false), m_done(false) { }

    void set(size_t count) {
      ScopedLock lock(m_mutex);
      m_outstanding = count;
      m_done = (m_outstanding == 0) ? true : false;
      m_errors = m_retries = false;
    }

    void decrement() {
      ScopedLock lock(m_mutex);
      HT_ASSERT(m_outstanding);
      m_outstanding--;

      if (m_outstanding == 0) {
        m_done = true;
        m_cond.notify_all();
      }
    }

    bool wait_for_completion(Timer &timer) {
      ScopedLock lock(m_mutex);
      boost::xtime expire_time;

      timer.start();

      while (m_outstanding) {
        boost::xtime_get(&expire_time, boost::TIME_UTC_);
        xtime_add_millis(expire_time, timer.remaining());
        if (!m_cond.timed_wait(lock, expire_time))
          HT_THROW(Error::REQUEST_TIMEOUT, "");
      }

      return !(m_retries || m_errors);
    }

    bool wait_for_completion() {
      ScopedLock lock(m_mutex);
      while (m_outstanding)
        m_cond.wait(lock);
      return !(m_retries || m_errors);
    }

    void set_retries() { m_retries = true; }

    void set_errors() { m_errors = true; }

    bool has_retries() { return m_retries; }

    bool has_errors() { return m_errors; }

    void clear_errors() { m_errors = false; }

    bool is_complete() { return m_done; }

  private:
    Mutex m_mutex;
    boost::condition m_cond;
    size_t m_outstanding;
    bool m_retries;
    bool m_errors;
    bool m_done;
  };

}

#endif // HYPERTABLE_TABLEMUTATORCOMPLETIONCOUNTER_H
