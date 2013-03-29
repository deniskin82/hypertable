/**
 * Copyright (C) 2007-2012 Hypertable, Inc.
 *
 * This file is part of Hypertable.
 *
 * Hypertable is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or any later version.
 *
 * Hypertable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hypertable. If not, see <http://www.gnu.org/licenses/>
 */

#ifndef HYPERTABLE_STRING_H
#define HYPERTABLE_STRING_H

#include <string>
#include <sstream>

namespace Hypertable {
  /**
   * We might want to use something better later, as std::string always
   * causes a heap allocation, and is lacking in functionalities
   * cf. http://www.and.org/vstr/comparison
   */
  typedef std::string String;
  typedef long unsigned int Lu;         // shortcut for printf format
  typedef long long unsigned int Llu;   // ditto
  typedef long long int Lld;            // ditto

  /**
   * Return a String using printf like format facilities
   * vanilla snprintf is about 1.5x faster than this, which is about:
   *   10x faster than boost::format;
   *   1.5x faster than std::string append (operator+=);
   *   3.5x faster than std::string operator+;
   */
  String format(const char *fmt, ...) __attribute__((format (printf, 1, 2)));

  /**
   * Return decimal number string separated by a separator (default: comma)
   * for every 3 digits. Only 10-15% slower than sprintf("%lld", n);
   */
  String format_number(int64_t n, int sep = ',');

  /**
   * Return first n bytes of buffer with an optinal trailer if the
   * size of the buffer exceeds n.
   */
  String
  format_bytes(size_t n, const void *buf, size_t len,
               const char *trailer = "...");

  /**
   * Return a string presentation of a sequence. Is quite slow but versatile,
   * as it uses ostringstream.
   */
  template <class SequenceT>
  String format_list(const SequenceT &seq, const char *sep = ", ") {
    typedef typename SequenceT::const_iterator Iterator;
    Iterator it = seq.begin(), end = seq.end();
    std::ostringstream out;
    out <<'[';

    if (it != end) {
      out << *it;
      ++it;
    }
    for (; it != end; ++it)
      out << sep << *it;

    out <<']';
    return out.str();
  }

} // namespace Hypertable

#endif // HYPERTABLE_STRING_H
