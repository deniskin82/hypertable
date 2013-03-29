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

#ifndef HYPERTABLE_CHECKSUM_H
#define HYPERTABLE_CHECKSUM_H

namespace Hypertable {

/** Compute fletcher32 checksum for arbitary data
 *
 * @param data - input data
 * @param len - input data length in bytes
 */
extern uint32_t
fletcher32(const void *data, size_t len);

/** Compute fletcher32 checksum for 16-bit aligned and padded data
 *  slightly faster than fletcher32
 *
 * @param data - input data
 * @param len - input data length in bytes
 */
extern uint32_t
fletcher32a(const uint16_t *data, size_t len);

/** Compute adler32 checksum
 *
 * @param data - input data
 * @param len - input data length in bytes
 */
extern uint32_t
adler32(const void *data, size_t len);

/** Update adler32 checksum incrementally
 *
 * @param adler - current adler32 checksum
 * @param data - input data
 * @param len - input data length in bytes
 */
extern uint32_t
adler32_update(uint32_t adler, const void *data, size_t len);

/** Compute crc32 checksum
 *
 * @param data - input data
 * @param len - input data length in bytes
 */
extern uint32_t
crc32(const void *data, size_t len);

/** Update crc32 checksum incrementally
 *
 * @param crc - current crc32 checksum
 * @param data - input data
 * @param len - input data length in bytes
 */
extern uint32_t
crc32_update(uint32_t crc, const void *data, size_t len);

} // namespace Hypertable

#endif /* HYPERTABLE_CHECKSUM_H */
