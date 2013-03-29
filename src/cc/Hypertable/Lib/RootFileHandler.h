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

#ifndef HYPERTABLE_ROOTFILEHANDLER_H
#define HYPERTABLE_ROOTFILEHANDLER_H

#include "Hyperspace/HandleCallback.h"


namespace Hypertable {

  using namespace Hyperspace;

  class RangeLocator;

  /**
   *
   */
  class RootFileHandler : public HandleCallback {
  public:
    RootFileHandler(RangeLocator *rangelocator)
        : HandleCallback(EVENT_MASK_ATTR_SET),
          m_range_locator(rangelocator) { return; }

    virtual void attr_set(const std::string &name);

    RangeLocator    *m_range_locator;
    RootFileHandler *m_root_handler;
  };
}

#endif // HYPERTABLE_ROOTFILEHANDLER_H
