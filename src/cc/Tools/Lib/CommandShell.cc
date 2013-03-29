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

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <queue>

extern "C" {
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <editline/readline.h>
#include <signal.h>
}

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/thread/exceptions.hpp>

#include "Common/Error.h"
#include "Common/FileUtils.h"
#include "Common/System.h"
#include "Common/Usage.h"
#include "Common/Logger.h"

#include "CommandShell.h"

using namespace Hypertable;
using namespace std;


String CommandShell::ms_history_file = "";

namespace {
  const char *help_text =
  "\n" \
  "Interpreter Meta Commands\n" \
  "-------------------------\n" \
  "?          (\\?) Synonym for `help'.\n" \
  "clear      (\\c) Clear command.\n" \
  "exit       (\\q) Exit program. Same as quit.\n" \
  "print      (\\p) Print current command.\n" \
  "quit       (\\q) Quit program.\n" \
  "source <f> (.)  Execute commands in file <f>.\n" \
  "system     (\\!) Execute a system shell command.\n" \
  "\n";

  char *find_char(const char *s, int c) {
    bool in_quotes = false;
    char quote_char = 0;

    for (const char *ptr = s; *ptr; ptr++) {
      if (in_quotes) {
        if (*ptr == quote_char && *(ptr - 1) != '\\')
          in_quotes = false;
      }
      else {
        if (*ptr == (char )c)
          return (char *)ptr;
        else if (*ptr == '\'' || *ptr == '"') {
          in_quotes = true;
          quote_char = *ptr;
        }
      }
    }
    return 0;
  }
}

CommandShell *CommandShell::ms_instance;

/**
 */
CommandShell::CommandShell(const String &program_name,
    CommandInterpreterPtr &interp_ptr, PropertiesPtr &props)
    : m_program_name(program_name), m_interp_ptr(interp_ptr), m_props(props),
      m_batch_mode(false), m_silent(false), m_test_mode(false),
      m_no_prompt(false), m_cont(false), m_line_read(0), m_notify(false),
      m_has_cmd_file(false), m_has_cmd_exec(false) {

  const char *home = getenv("HOME");
  if (home)
    ms_history_file = (String)home + "/." + m_program_name + "_history";
  else
    ms_history_file = (String)"." + m_program_name + "_history";

  m_batch_mode = m_props->has("batch");
  if (m_batch_mode)
    m_silent = true;
  else
    m_silent = m_props->get_bool("silent");
  m_test_mode = m_props->has("test-mode");
  if (m_test_mode)
    Logger::set_test_mode(System::exe_name);
  m_no_prompt = m_props->has("no-prompt");

  m_notify = m_props->has("notification-address");
  if (m_notify) {
    String notification_address = m_props->get_str("notification-address");
    m_notifier_ptr = new Notifier(notification_address.c_str());
  }

  if (m_props->has("execute")) {
    m_cmd_str = m_props->get_str("execute");
    m_has_cmd_exec = true;
    m_batch_mode = true;
  }
  else if (m_props->has("command-file")) {
    m_cmd_file = m_props->get_str("command-file");
    m_has_cmd_file = true;
    m_batch_mode = true;
  }

  setlocale(LC_ALL, "");

  /* initialize libedit */
  if (!m_batch_mode) {
    ms_instance = this;
    m_editline = el_init("hypertable", stdin, stdout, stderr);
    m_history = history_winit();
    history_w(m_history, &m_history_event, H_SETSIZE, 100);
    history_w(m_history, &m_history_event, H_LOAD, ms_history_file.c_str());

    el_wset(m_editline, EL_HIST, history_w, m_history);
    el_wset(m_editline, EL_PROMPT, prompt);
    el_wset(m_editline, EL_SIGNAL, 1);
    el_wset(m_editline, EL_EDITOR, L"emacs");

    /* Source the user's defaults file. */
    el_source(m_editline, NULL);
    m_tokenizer = tok_winit(NULL);
  }
  else {
    m_editline = 0;
    m_history = 0;
    m_tokenizer = 0;
  }

  /* initialize prompt string */
  wchar_t buf[64] = {0};
  const char *p = program_name.c_str();
  mbsrtowcs(buf, &p, 63, 0);
  m_prompt_str = buf;
  m_prompt_str += L"> ";
}

CommandShell::~CommandShell() {
  if (m_editline)
    el_end(m_editline);
  if (m_history)
    history_wend(m_history);
  if (m_tokenizer)
    tok_wend(m_tokenizer);
}

/**
 */
char *CommandShell::rl_gets () {
  if (m_line_read) {
    free(m_line_read);
    m_line_read = (char *)NULL;
  }

  /* Execute commands from command line string/file */
  if (m_has_cmd_exec || m_has_cmd_file) {
    static bool done = false;

    if (done)
      return 0;

    if (m_has_cmd_exec) {
      m_line_read = (char *)malloc(m_cmd_str.size() + 1);
      strcpy(m_line_read, m_cmd_str.c_str());
    }
    else {
      off_t len;
      char *tmp;
      // copy bcos readline uses malloc, FileUtils::file_to_buffer uses new
      tmp = FileUtils::file_to_buffer(m_cmd_file, &len);
      m_line_read = (char *)malloc(len);
      memcpy(m_line_read, tmp, len);
      delete[] tmp;
    }

    done = true;
    return m_line_read;
  }

  /* Get a line from the user. */
  if (m_batch_mode || m_no_prompt || m_silent || m_test_mode) {
    if (!getline(cin, m_input_str))
      return 0;
    boost::trim(m_input_str);
    if (m_input_str.find("quit", 0) != 0 && !m_silent)
      cout << m_input_str << endl;
    return (char *)m_input_str.c_str();
  }
  else {
    const wchar_t *wline = 0;
    int numc = 0;
    while (true) {
      wline = el_wgets(m_editline, &numc);
      if (wline == 0 || numc == 0)
        return (char *)"exit";

      if (!m_cont && numc == 1)
        continue;  /* Only got a linefeed */

      const LineInfoW *li = el_wline(m_editline);
      int ac = 0, cc = 0, co = 0;
      const wchar_t **av;
      int ncont = tok_wline(m_tokenizer, li, &ac, &av, &cc, &co);
      if (ncont < 0) {
        // (void) fprintf(stderr, "Internal error\n");
        m_cont = false;
        continue;
      }

      if (el_wparse(m_editline, ac, av) == -1)
        break;
    }

    char *buffer = (char *)malloc(1024 * 8);
    size_t len = 1024 * 8;
    while (1) {
      const wchar_t *wp = &wline[0];
      size_t l = wcsrtombs(buffer, &wp, len, 0);
      if (l > len) {
        buffer = (char *)realloc(buffer, l + 1);
        len = l + 1;
      }
      else
        break;
    }
    m_line_read = buffer;

    /* If the line has any text in it, save it on the history. */
    if (!m_batch_mode && !m_test_mode && m_line_read && *m_line_read)
      history_w(m_history, &m_history_event,
              m_cont ? H_APPEND : H_ENTER, wline);
  }

  return m_line_read;
}

void CommandShell::add_options(PropertiesDesc &desc) {
  desc.add_options()
    ("batch", "Disable interactive behavior")
    ("no-prompt", "Do not display an input prompt")
    ("test-mode", "Don't display anything that might change from run to run "
        "(e.g. timing statistics)")
    ("timestamp-format", Property::str(), "Output format for timestamp. "
        "Currently the only formats are 'default' and 'usecs'")
    ("notification-address", Property::str(), "[<host>:]<port> "
        "Send notification datagram to this address after each command.")
    ("execute,e", Property::str(), "Execute specified commands.")
    ("command-file", Property::str(), "Execute commands from file.")
    ;
}


int CommandShell::run() {
  const char *line;
  std::queue<string> command_queue;
  String command;
  String timestamp_format;
  String source_commands;
  String use_ns;
  const char *base, *ptr;

  if (m_props->has("timestamp-format"))
    timestamp_format = m_props->get_str("timestamp-format");

  if (timestamp_format != "")
    m_interp_ptr->set_timestamp_output_format(timestamp_format);

  if (!m_batch_mode && !m_silent) {
    read_history(ms_history_file.c_str());

    cout << endl;
    cout << "Welcome to the " << m_program_name << " command interpreter."
         << endl;
    cout << "For information about Hypertable, visit http://hypertable.com"
         << endl;
    cout << endl;
    cout << "Type 'help' for a list of commands, or 'help shell' for a" << endl;
    cout << "list of shell meta commands." << endl;
    cout << endl << flush;
  }

  m_accum = "";
  if (!m_batch_mode)
    using_history();

  trim_if(m_namespace, boost::is_any_of(" \t\n\r;"));
  if (m_namespace.size()) {
    use_ns = "USE \"" + m_namespace + "\";";
    line = use_ns.c_str();
    goto process_line;
  }

  while ((line = rl_gets()) != 0) {
process_line:
    try {
      if (*line == 0)
        continue;

      if (!strncasecmp(line, "help shell", 10)) {
        cout << help_text;
        continue;
      }
      else if (!strncasecmp(line, "help", 4)
               || !strncmp(line, "\\h", 2) || *line == '?') {
        command = line;
        std::transform(command.begin(), command.end(), command.begin(),
                       ::tolower);
        trim_if(command, boost::is_any_of(" \t\n\r;"));
        m_interp_ptr->execute_line(command);
        if (m_notify)
          m_notifier_ptr->notify();
        continue;
      }
      else if (!strncasecmp(line, "quit", 4) || !strncasecmp(line, "exit", 4)
          || !strcmp(line, "\\q")) {
        if (!m_batch_mode)
          history_w(m_history, &m_history_event, H_SAVE,
                  ms_history_file.c_str());
        return 0;
      }
      else if (!strncasecmp(line, "print", 5) || !strcmp(line, "\\p")) {
        cout << m_accum << endl;
        continue;
      }
      else if (!strncasecmp(line, "clear", 5) || !strcmp(line, "\\c")) {
        m_accum = "";
        m_cont = false;
        continue;
      }
      else if (!strncasecmp(line, "source", 6) || line[0] == '.') {
        if ((base = strchr(line, ' ')) == 0) {
          cout << "syntax error: source or '.' must be followed by a space "
              "character" << endl;
          continue;
        }
        String fname = base;
        trim_if(fname, boost::is_any_of(" \t\n\r;"));
        off_t flen;
        if ((base = FileUtils::file_to_buffer(fname.c_str(), &flen)) == 0)
          continue;
        source_commands = "";
        ptr = strtok((char *)base, "\n\r");
        while (ptr != 0) {
          command = ptr;
          boost::trim(command);
          if (command.find("#") != 0)
            source_commands += command + " ";
          ptr = strtok(0, "\n\r");
        }
        if (source_commands == "")
          continue;
        delete [] base;
        line = source_commands.c_str();
      }
      else if (!strncasecmp(line, "system", 6) || !strncmp(line, "\\!", 2)) {
        String command = line;
        size_t offset = command.find_first_of(' ');
        if (offset != String::npos) {
          command = command.substr(offset+1);
          trim_if(command, boost::is_any_of(" \t\n\r;"));
          HT_EXPECT(system(command.c_str()) == 0, Error::EXTERNAL);
        }
        continue;
      }
      else if (!strcasecmp(line, "status") || !strcmp(line, "\\s")) {
        cout << endl << "no status." << endl << endl;
        continue;
      }

      /**
       * Add commands to queue
       */
      base = line;
      ptr = find_char(base, ';');
      while (ptr) {
        m_accum += string(base, ptr-base);
        if (m_accum.size() > 0) {
          boost::trim(m_accum);
          if (m_accum.find("#") != 0)
            command_queue.push(m_accum);
          m_accum = "";
          m_cont = false;
        }
        base = ptr+1;
        ptr = find_char(base, ';');
      }
      command = string(base);
      boost::trim(command);
      if (command != "" && command.find("#") != 0) {
        m_accum += command;
        boost::trim(m_accum);
      }
      if (m_accum != "") {
        m_cont = true;
        m_accum += " ";
      }

      while (!command_queue.empty()) {
        if (command_queue.front() == "quit"
            || command_queue.front() == "exit") {
          if (!m_batch_mode)
            history_w(m_history, &m_history_event, H_SAVE,
                    ms_history_file.c_str());
          return 0;
        }
        command = command_queue.front();
        command_queue.pop();
        if (!strncmp(command.c_str(), "pause", 5)) {
          String sec_str = command.substr(5);
          boost::trim(sec_str);
          char *endptr;
          long secs = strtol(sec_str.c_str(), &endptr, 0);
          if ((secs == 0 && errno == EINVAL) || *endptr != 0) {
            cout << "error: invalid seconds specification" << endl;
            if (m_batch_mode)
              return 1;
          }
          else
            poll(0, 0, secs*1000);
        }
        else {
          m_interp_ptr->execute_line(command);
          if(m_notify)
            m_notifier_ptr->notify();
        }
      }
    }
    catch (Hypertable::Exception &e) {
      if (e.code() == Error::BAD_NAMESPACE)
        cerr << "ERROR: No namespace is open (see 'use' command)" << endl;
      else {
        if (m_test_mode)
          cerr << "Error: " << e.what() << " - " << Error::get_text(e.code())
              << endl;
        else
          cerr << "Error: " << e << " - " << Error::get_text(e.code()) << endl;
      }
      if(m_notify)
        m_notifier_ptr->notify();
      if (m_batch_mode)
        return 1;
      m_accum = "";
      while (!command_queue.empty())
        command_queue.pop();
      m_cont = false;
    }
  }

  if (!m_batch_mode)
    history_w(m_history, &m_history_event, H_SAVE, ms_history_file.c_str());

  return 0;
}

const wchar_t *
CommandShell::prompt(EditLine *el) {
  if (ms_instance->m_cont)
    return L"         -> ";
  else
    return ms_instance->m_prompt_str.c_str();
}