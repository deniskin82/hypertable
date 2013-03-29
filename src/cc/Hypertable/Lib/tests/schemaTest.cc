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

#include "Common/Compat.h"
extern "C" {
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
}

#include "Common/FileUtils.h"
#include "Common/Logger.h"
#include "Common/TestHarness.h"
#include "Common/Usage.h"

#include "Hypertable/Lib/Schema.h"

using namespace Hypertable;

namespace {
  const char *usage[] = {
    "usage: schemaTest [--golden]",
    "",
    "Validates Schema class.  If --golden is supplied, a new golden",
    "output file 'schemaTest.golden' will be generated",
    0
  };
  const char *bad_schemas[] = {
    "bad-schema-1.xml",
    "bad-schema-2.xml",
    "bad-schema-3.xml",
    "bad-schema-4.xml",
    "bad-schema-5.xml",
    "bad-schema-6.xml",
    "bad-schema-7.xml",
    "bad-schema-8.xml",
    "bad-schema-9.xml",
    "good-schema-1.xml",
    0
  };
}


int main(int argc, char **argv) {
  off_t len;
  const char *buf;
  bool golden = false;
  TestHarness harness("schemaTest");
  Schema *schema;

  if (argc > 1) {
    if (!strcmp(argv[1], "--golden"))
      golden = true;
    else
      Usage::dump_and_exit(usage);
  }

  for (int i=0; bad_schemas[i] != 0; ++i) {
    if ((buf = FileUtils::file_to_buffer(bad_schemas[i], &len)) == 0)
      harness.display_error_and_exit();
    schema = Schema::new_instance(buf, len);
    if (!schema->is_valid()) {
      HT_ERRORF("Schema Parse Error: %s", schema->get_error_string());
    }
    delete schema;
    delete [] buf;
  }

  schema = new Schema();

  schema->open_access_group();
  schema->set_access_group_parameter("name", "default");
  schema->open_column_family();
  schema->set_column_family_parameter("Name", "default");
  schema->set_column_family_parameter("ttl", "2592000");
  schema->set_column_family_parameter("MaxVersions", "3");
  schema->close_column_family();
  schema->close_access_group();

  schema->open_access_group();
  schema->set_access_group_parameter("name", "meta");
  schema->open_column_family();
  schema->set_column_family_parameter("Name", "language");
  schema->close_column_family();
  schema->open_column_family();
  schema->set_column_family_parameter("Name", "checksum");
  schema->close_column_family();
  schema->close_access_group();

  std::string schemastr;
  schema->render(schemastr);
  FileUtils::write(harness.get_log_file_descriptor(), schemastr.c_str(),
                   schemastr.length());

  schemastr = "";
  schema->assign_ids();
  schema->render(schemastr);
  FileUtils::write(harness.get_log_file_descriptor(), schemastr.c_str(),
                   schemastr.length());

  delete schema;

  if (!golden)
    harness.validate_and_exit("schemaTest.golden");

  harness.regenerate_golden_file("schemaTest.golden");

  return 0;
}
