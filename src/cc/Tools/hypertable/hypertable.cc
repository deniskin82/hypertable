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
#include "Common/Init.h"
#include "Tools/Lib/CommandShell.h"

#include "Hypertable/Lib/Config.h"
#include "Hypertable/Lib/Client.h"
#include "Hypertable/Lib/HqlCommandInterpreter.h"

using namespace Hypertable;
using namespace Config;

namespace {

  struct AppPolicy : Config::Policy {
    static void init_options() {
      cmdline_desc().add_options()
        ("no-log-sync", boo()->default_value(false),
         "Don't sync rangeserver commit logs on autoflush")
        ("namespace", str()->default_value(""),
         "Automatically use specified namespace when starting")
        ;
      alias("no-log-sync", "Hypertable.HqlInterpreter.Mutator.NoLogSync");
    }
  };

}

typedef Meta::list<AppPolicy, CommandShellPolicy, DefaultCommPolicy> Policies;


int main(int argc, char **argv) {

  CommandShellPtr shell;
  CommandInterpreterPtr interp;
  Client *hypertable;
  int status = 0;

  try {
    init_with_policies<Policies>(argc, argv);

    hypertable = new Hypertable::Client();
    interp = new HqlCommandInterpreter(hypertable);
    shell = new CommandShell("hypertable", interp, properties);
    shell->set_namespace(get_str("namespace"));
    interp->set_silent(shell->silent());
    interp->set_test_mode(shell->test_mode());

    status = shell->run();
  }
  catch(Exception &e) {
    HT_ERROR_OUT << e << HT_END;
    status = e.code();
  }
  _exit(status);
}
