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

#include <map>
#include <fstream>

#include "Common/FailureInducer.h"
#include "Common/Init.h"
#include "Common/System.h"

#include "AsyncComm/Comm.h"
#include "AsyncComm/ConnectionHandlerFactory.h"
#include "AsyncComm/ReactorFactory.h"

#include "Hypertable/Lib/Config.h"
#include "Hypertable/Lib/MetaLogReader.h"
#include "Hypertable/Lib/RangeState.h"
#include "Hypertable/Lib/Types.h"

#include "DfsBroker/Lib/Client.h"

#include "Hypertable/Master/Context.h"
#include "Hypertable/Master/LoadBalancer.h"
#include "Hypertable/Master/MetaLogDefinitionMaster.h"
#include "Hypertable/Master/OperationCreateNamespace.h"
#include "Hypertable/Master/OperationCreateTable.h"
#include "Hypertable/Master/OperationDropNamespace.h"
#include "Hypertable/Master/OperationInitialize.h"
#include "Hypertable/Master/OperationProcessor.h"
#include "Hypertable/Master/OperationRenameTable.h"
#include "Hypertable/Master/OperationSystemUpgrade.h"
#include "Hypertable/Master/OperationMoveRange.h"
#include "Hypertable/Master/RangeServerConnectionManager.h"
#include "Hypertable/Master/ReferenceManager.h"
#include "Hypertable/Master/ResponseManager.h"
#include "Hypertable/Master/BalancePlanAuthority.h"

#include <boost/algorithm/string.hpp>

extern "C" {
#include <poll.h>
}

using namespace Hypertable;
using namespace Config;
using namespace Hyperspace;

namespace {

  struct AppPolicy : Config::Policy {
    static void init_options() {
      cmdline_desc("Usage: %s <test>\n\n"
                   "  This program tests failures and state transitions\n"
                   "  of various Master operations.  Valid tests include:\n"
                   "\n"
                   "  initialize\n"
                   "  system_upgrade\n"
                   "  create_namespace\n"
                   "  drop_namespace\n"
                   "  create_table\n"
                   "  create_table_with_index\n"
                   "  rename_table\n"
                   "  move_range\n"
                   "  balance_plan_authority\n"
                   "\nOptions");
      cmdline_hidden_desc().add_options()("test", str(), "test to run");
      cmdline_positional_desc().add("test", -1);
    }
    static void init() {
      if (!has("test")) {
        HT_ERROR_OUT <<"test name required\n"<< cmdline_desc() << HT_END;
        exit(1);
      }
    }
  };


  typedef Meta::list<GenericServerPolicy, DfsClientPolicy,
                     HyperspaceClientPolicy, DefaultCommPolicy, AppPolicy> Policies;

  void initialize_test(ContextPtr &context, const String &log_dir,
                       std::vector<MetaLog::EntityPtr> &entities,
                       const String &failure_point) {
    OperationPtr operation;

    FailureInducer::instance->clear();
    if (failure_point != "")
      FailureInducer::instance->parse_option(failure_point);

    context->op = new OperationProcessor(context, 4);
    context->mml_writer = new MetaLog::Writer(context->dfs, context->mml_definition,
                                              log_dir + "/" + context->mml_definition->name(),
                                              entities);
    for (size_t i=0; i<entities.size(); i++) {
      if ((operation = dynamic_cast<Operation *>(entities[i].get()))) {
	if (operation->remove_explicitly())
	  context->reference_manager->add(operation);
        context->op->add_operation(operation);
      }
    }
  }

  typedef std::multimap<String, int32_t> ExpectedResultsMap;


  void run_test(ContextPtr &context, const String &log_dir,
                std::vector<MetaLog::EntityPtr> &entities,
                const String &failure_point,
                ExpectedResultsMap &expected_operations,
                std::vector<String> &expected_servers) {
    OperationPtr operation;
    RangeServerConnectionPtr rsc;
    std::vector<OperationPtr> operations;
    std::vector<MetaLog::EntityPtr> tmp_entities;

    context->op = new OperationProcessor(context, 4);

    FailureInducer::instance->clear();
    if (failure_point != "")
      FailureInducer::instance->parse_option(failure_point);

    for (size_t i = 0; i < entities.size(); i++) {
      if ((operation = dynamic_cast<Operation *>(entities[i].get()))) {
        if (operation->remove_explicitly())
          context->reference_manager->add(operation);
        operations.push_back(operation);
      }
    }
    context->op->add_operations(operations);

    if (failure_point == "")
      context->op->wait_for_empty();
    else
      context->op->join();

    context->mml_writer = 0;
    MetaLog::ReaderPtr mml_reader = new MetaLog::Reader(context->dfs, context->mml_definition,
                                                        log_dir + "/" + context->mml_definition->name());
    entities.clear();
    mml_reader->get_entities(entities);

    // Remove the BalancePlanAuthority object
    foreach_ht (MetaLog::EntityPtr &entity, entities) {
      if (dynamic_cast<BalancePlanAuthority *>(entity.get()) == 0)
        tmp_entities.push_back(entity);
    }
    entities.swap(tmp_entities);

#if 0
    HT_INFO("Entities:");
    foreach_ht (MetaLog::EntityPtr &entity, entities)
      HT_INFOF("%s", entity->name().c_str());
    HT_INFO("Expected Entities:");
    for (ExpectedResultsMap::iterator iter = expected_operations.begin();
         iter != expected_operations.end(); ++iter) {
      HT_INFOF("%s", iter->first.c_str());
    }
    foreach_ht (String &server, expected_servers)
      HT_INFOF("%s", server.c_str());
#endif

    HT_ASSERT(entities.size() == expected_operations.size()
              + expected_servers.size());

    for (size_t i = 0; i < entities.size(); i++) {
      bool match = false;
      operation = dynamic_cast<Operation *>(entities[i].get());
      rsc = dynamic_cast<RangeServerConnection *>(entities[i].get());
      if (operation) {
        String name = operation->name();
        const char *ptr = strstr(name.c_str(), " ");
        String operation_name = ptr ? String(name.c_str(), ptr-name.c_str()) : name;
        for (ExpectedResultsMap::iterator iter = expected_operations.begin();
             iter != expected_operations.end(); ++iter) {
          if (operation_name == iter->first &&
              operation->get_state() == iter->second) {
            match = true;
            break;
          }
        }
        if (!match)
          std::cout << "Unable to find match for '" << operation_name << "'" << std::endl;
      }
      else if (rsc) {
        match = false;
        for (std::vector<String>::iterator iter = expected_servers.begin();
             iter != expected_servers.end(); ++iter) {
          if (rsc->location() == *iter) {
            match = true;
            break;
          }
        }
        if (!match)
          std::cout << "Unable to find match for '" << rsc->location() << "'" << std::endl;
      }

      if (!match) {
        std::cout << "ERROR - invalid state" << std::endl;

        std::cout << "expected:" << std::endl;
        for (ExpectedResultsMap::iterator iter = expected_operations.begin();
             iter != expected_operations.end(); ++iter)
          std::cout << iter->first << " -> " << iter->second << std::endl;
        for (std::vector<String>::iterator iter = expected_servers.begin();
             iter != expected_servers.end(); ++iter) {
          std::cout << "server " << *iter << std::endl;

          std::cout << "got:" << std::endl;
          for (size_t i=0; i<entities.size(); i++) {
            operation = dynamic_cast<Operation *>(entities[i].get());
            if (operation)
              std::cout << operation->name() << " -> "
                  << operation->get_state() << std::endl;
            else {
              rsc = dynamic_cast<RangeServerConnection *>(entities[i].get());
              std::cout << "server " << rsc->location() << std::endl;
            }
          }
          _exit(1);
        }
      }
    }

    context->mml_writer = new MetaLog::Writer(context->dfs, context->mml_definition,
                                              log_dir + "/" + context->mml_definition->name(),
                                              entities);
  }


} // local namespace

void create_namespace_test(ContextPtr &context);
void drop_namespace_test(ContextPtr &context);
void create_table_test(ContextPtr &context);
void create_table_with_index_test(ContextPtr &context);
void rename_table_test(ContextPtr &context);
void master_initialize_test(ContextPtr &context);
void system_upgrade_test(ContextPtr &context);
void move_range_test(ContextPtr &context);
void balance_plan_authority_test(ContextPtr &context);


int main(int argc, char **argv) {
  ContextPtr context = new Context();
  BalancePlanAuthorityPtr bpa;
  std::vector<MetaLog::EntityPtr> entities;

  // Register ourselves as the Comm-layer proxy master
  ReactorFactory::proxy_master = true;

  context->test_mode = true;

  try {
    init_with_policies<Policies>(argc, argv);

    context->comm = Comm::instance();
    context->conn_manager = new ConnectionManager(context->comm);
    context->props = properties;
    context->hyperspace = new Hyperspace::Session(context->comm, context->props);
    context->dfs = new DfsBroker::Client(context->conn_manager, context->props);
    context->rsc_manager = new RangeServerConnectionManager();

    context->toplevel_dir = properties->get_str("Hypertable.Directory");
    boost::trim_if(context->toplevel_dir, boost::is_any_of("/"));
    context->toplevel_dir = String("/") + context->toplevel_dir;
    context->namemap = new NameIdMapper(context->hyperspace, context->toplevel_dir);
    context->monitoring = new Monitoring(context.get());

    context->mml_definition = new MetaLog::DefinitionMaster(context, "master");
    String log_dir = context->toplevel_dir + "/servers/master/log";
    context->mml_writer = new MetaLog::Writer(context->dfs, context->mml_definition,
                                              log_dir + "/" + context->mml_definition->name(),
                                              entities);

    bpa = context->get_balance_plan_authority();

    FailureInducer::instance = new FailureInducer();
    context->request_timeout = 600;

    context->balancer = new LoadBalancer(context);

    ResponseManagerContext *rmctx = new ResponseManagerContext(context->mml_writer);
    context->response_manager = new ResponseManager(rmctx);
    Thread response_manager_thread(*context->response_manager);

    context->reference_manager = new ReferenceManager();

    String testname = get_str("test");

    if (testname == "initialize")
      master_initialize_test(context);
    else if (testname == "system_upgrade")
      system_upgrade_test(context);
    else if (testname == "create_namespace")
      create_namespace_test(context);
    else if (testname == "drop_namespace")
      drop_namespace_test(context);
    else if (testname == "create_table")
      create_table_test(context);
    else if (testname == "create_table_with_index")
      create_table_with_index_test(context);
    else if (testname == "rename_table")
      rename_table_test(context);
    else if (testname == "move_range")
      move_range_test(context);
    else if (testname == "balance_plan_authority")
      balance_plan_authority_test(context);
    else {
      HT_ERRORF("Unrecognized test name: %s", testname.c_str());
      _exit(1);
    }

  }
  catch (Exception &e) {
    HT_ERROR_OUT << e << HT_END;
    _exit(1);
  }

  return 0;
}


void create_namespace_test(ContextPtr &context) {
  std::vector<MetaLog::EntityPtr> entities;
  ExpectedResultsMap expected_operations;
  std::vector<String> expected_servers;
  String log_dir = context->toplevel_dir + "/servers/master/log";

  context->mml_writer = new MetaLog::Writer(context->dfs, context->mml_definition,
                                            log_dir + "/" + context->mml_definition->name(),
                                            entities);

  entities.push_back( new OperationCreateNamespace(context, "foo", 0) );

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateNamespace", OperationState::ASSIGN_ID) );
  run_test(context, log_dir, entities, "create-namespace-INITIAL:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateNamespace", OperationState::ASSIGN_ID) );
  run_test(context, log_dir, entities, "create-namespace-ASSIGN_ID-a:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateNamespace", OperationState::ASSIGN_ID) );
  run_test(context, log_dir, entities, "create-namespace-ASSIGN_ID-b:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateNamespace", OperationState::COMPLETE) );
  run_test(context, log_dir, entities, "", expected_operations, expected_servers);

  context->op->shutdown();
  context->op->join();

  context = 0;
  _exit(0);
}


void drop_namespace_test(ContextPtr &context) {
  std::vector<MetaLog::EntityPtr> entities;
  ExpectedResultsMap expected_operations;
  std::vector<String> expected_servers;
  String log_dir = context->toplevel_dir + "/servers/master/log";

  context->mml_writer = new MetaLog::Writer(context->dfs, context->mml_definition,
                                            log_dir + "/" + context->mml_definition->name(),
                                            entities);

  entities.push_back( new OperationDropNamespace(context, "foo", false) );

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationDropNamespace", OperationState::STARTED) );
  run_test(context, log_dir, entities, "drop-namespace-INITIAL:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationDropNamespace", OperationState::STARTED) );
  run_test(context, log_dir, entities, "drop-namespace-STARTED-a:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationDropNamespace", OperationState::STARTED) );
  run_test(context, log_dir, entities, "drop-namespace-STARTED-b:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationDropNamespace", OperationState::COMPLETE) );
  run_test(context, log_dir, entities, "", expected_operations, expected_servers);

  context->op->shutdown();
  context->op->join();

  context = 0;
  _exit(0);
}

namespace {
  const char *schema_str = "<Schema>\n"
    "  <AccessGroup name=\"default\">\n"
    "    <ColumnFamily>\n"
    "      <Name>column</Name>\n"
    "    </ColumnFamily>\n"
    "  </AccessGroup>\n"
    "</Schema>";

  const char *index_schema_str = "<Schema>\n"
    "  <AccessGroup name=\"default\">\n"
    "    <ColumnFamily>\n"
    "      <Name>a</Name>\n"
    "      <Index>true</Index>\n"
    "    </ColumnFamily>\n"
    "    <ColumnFamily>\n"
    "      <Name>b</Name>\n"
    "      <QualifierIndex>true</QualifierIndex>\n"
    "    </ColumnFamily>\n"
    "  </AccessGroup>\n"
    "</Schema>";
}



void create_table_test(ContextPtr &context) {
  std::vector<MetaLog::EntityPtr> entities;
  ExpectedResultsMap expected_operations;
  std::vector<String> expected_servers;
  String log_dir = context->toplevel_dir + "/servers/master/log";
  RangeServerConnectionPtr rsc1, rsc2;

  context->mml_writer = new MetaLog::Writer(context->dfs, context->mml_definition,
                                            log_dir + "/" + context->mml_definition->name(),
                                            entities);

  rsc1 = new RangeServerConnection("rs1", "foo.hypertable.com", InetAddr("72.14.204.99", 38060));
  rsc2 = new RangeServerConnection("rs2", "bar.hypertable.com", InetAddr("69.147.125.65", 38060));

  context->rsc_manager->connect_server(rsc1, "foo.hypertable.com", InetAddr("72.14.204.99", 33567), InetAddr("72.14.204.99", 38060));
  context->rsc_manager->connect_server(rsc2, "bar.hypertable.com", InetAddr("69.147.125.65", 30569), InetAddr("69.147.125.65", 38060));

  {
    std::vector<MetaLog::Entity *> entities;
    entities.push_back(rsc1.get());
    entities.push_back(rsc2.get());
    context->mml_writer->record_state(entities);
  }

  expected_servers.push_back("rs1");
  expected_servers.push_back("rs2");

  entities.push_back( new OperationCreateTable(context, "tablefoo", schema_str) );

  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::ASSIGN_ID) );
  run_test(context, log_dir, entities, "create-table-INITIAL:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::ASSIGN_ID) );
  run_test(context, log_dir, entities, "Utility-create-table-in-hyperspace-1:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::ASSIGN_ID) );
  run_test(context, log_dir, entities, "Utility-create-table-in-hyperspace-2:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::ASSIGN_ID) );
  run_test(context, log_dir, entities, "create-table-ASSIGN_ID:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::ASSIGN_ID) );
  run_test(context, log_dir, entities, "create-table-WRITE_METADATA-a:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::ASSIGN_LOCATION) );
  run_test(context, log_dir, entities, "create-table-WRITE_METADATA-b:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::LOAD_RANGE) );
  run_test(context, log_dir, entities, "create-table-ASSIGN_LOCATION:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::LOAD_RANGE) );
  run_test(context, log_dir, entities, "create-table-LOAD_RANGE-a:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::ACKNOWLEDGE) );
  run_test(context, log_dir, entities, "create-table-LOAD_RANGE-b:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::ACKNOWLEDGE) );
  run_test(context, log_dir, entities, "create-table-ACKNOWLEDGE:throw:0",
           expected_operations, expected_servers);

  context->rsc_manager->disconnect_server(rsc1);
  initialize_test(context, log_dir, entities, "");
  poll(0,0,100);
  context->rsc_manager->connect_server(rsc1, "foo.hypertable.com", InetAddr("localhost", 30267),
                                       InetAddr("localhost", 38060));
  context->op->wait_for_empty();

  context->op->shutdown();
  context->op->join();

  context = 0;
  _exit(0);
}


void create_table_with_index_test(ContextPtr &context) {
  std::vector<MetaLog::EntityPtr> entities;
  ExpectedResultsMap expected_operations;
  std::vector<String> expected_servers;
  String log_dir = context->toplevel_dir + "/servers/master/log";
  RangeServerConnectionPtr rsc1, rsc2;

  context->mml_writer = new MetaLog::Writer(context->dfs, 
                                        context->mml_definition,
                                        log_dir + "/" + 
                                            context->mml_definition->name(),
                                        entities);

  rsc1 = new RangeServerConnection("rs1", 
          "foo.hypertable.com", InetAddr("72.14.204.99", 38060));
  rsc2 = new RangeServerConnection("rs2", 
          "bar.hypertable.com", InetAddr("69.147.125.65", 38060));

  context->rsc_manager->connect_server(rsc1, "foo.hypertable.com", 
          InetAddr("72.14.204.99", 33567), InetAddr("72.14.204.99", 38060));
  context->rsc_manager->connect_server(rsc2, "bar.hypertable.com", 
          InetAddr("69.147.125.65", 30569), InetAddr("69.147.125.65", 38060));

  {
    std::vector<MetaLog::Entity *> entities;
    entities.push_back(rsc1.get());
    entities.push_back(rsc2.get());
    context->mml_writer->record_state(entities);
  }

  expected_servers.push_back("rs1");
  expected_servers.push_back("rs2");

  entities.push_back(new OperationCreateTable(context, 
                            "tablefoo_index", index_schema_str));

  expected_operations.insert(std::pair<String, int32_t>("OperationCreateTable", OperationState::ASSIGN_ID) );
  run_test(context, log_dir, entities, "create-table-INITIAL:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::ASSIGN_ID) );
  run_test(context, log_dir, entities, "create-table-CREATE_INDEX:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::ASSIGN_ID) );
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::COMPLETE) );
  run_test(context, log_dir, entities, "create-table-CREATE_QUALIFIER_INDEX:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::ASSIGN_ID) );
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::COMPLETE) );
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::COMPLETE) );
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::ACKNOWLEDGE) );
  run_test(context, log_dir, entities, "create-table-FINALIZE:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::ASSIGN_ID) );
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::COMPLETE) );
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::COMPLETE) );
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::COMPLETE) );
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::COMPLETE) );
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::ACKNOWLEDGE) );
  run_test(context, log_dir, entities, "create-table-FINALIZE:throw:1",
           expected_operations, expected_servers);

  context->rsc_manager->disconnect_server(rsc1);
  initialize_test(context, log_dir, entities, "");
  poll(0,0,100);
  context->rsc_manager->connect_server(rsc1, "foo.hypertable.com", InetAddr("localhost", 30267),
                          InetAddr("localhost", 38060));
  context->op->wait_for_empty();

  context->op->shutdown();
  context->op->join();

  context = 0;
  _exit(0);
}


void rename_table_test(ContextPtr &context) {
  std::vector<MetaLog::EntityPtr> entities;
  ExpectedResultsMap expected_operations;
  std::vector<String> expected_servers;
  String log_dir = context->toplevel_dir + "/servers/master/log";

  context->mml_writer = new MetaLog::Writer(context->dfs, context->mml_definition,
                                            log_dir + "/" + context->mml_definition->name(),
                                            entities);

  entities.push_back( new OperationRenameTable(context, "tablefoo", "tablebar") );

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationRenameTable", OperationState::STARTED) );
  run_test(context, log_dir, entities, "rename-table-STARTED:throw:0",
           expected_operations, expected_servers);

  run_test(context, log_dir, entities, "", expected_operations, expected_servers);

  context->op->shutdown();
  context->op->join();

  context = 0;
  _exit(0);
}


void master_initialize_test(ContextPtr &context) {
  std::vector<MetaLog::EntityPtr> entities;
  ExpectedResultsMap expected_operations;
  std::vector<String> expected_servers;
  String log_dir = context->toplevel_dir + "/servers/master/log";
  RangeServerConnectionPtr rsc1, rsc2, rsc3, rsc4;

  context->mml_writer = new MetaLog::Writer(context->dfs, context->mml_definition,
                                            log_dir + "/" + context->mml_definition->name(),
                                            entities);

  rsc1 = new RangeServerConnection("rs1", "foo.hypertable.com", InetAddr("72.14.204.99", 38060));
  rsc2 = new RangeServerConnection("rs2", "bar.hypertable.com", InetAddr("69.147.125.65", 38060));
  rsc3 = new RangeServerConnection("rs3", "how.hypertable.com", InetAddr("72.14.204.98", 38060));
  rsc4 = new RangeServerConnection("rs4", "cow.hypertable.com", InetAddr("69.147.125.62", 38060));

  context->rsc_manager->connect_server(rsc1, "foo.hypertable.com", InetAddr("72.14.204.99", 33567), InetAddr("72.14.204.99", 38060));
  context->rsc_manager->connect_server(rsc2, "bar.hypertable.com", InetAddr("69.147.125.65", 30569), InetAddr("69.147.125.65", 38060));
  context->rsc_manager->connect_server(rsc3, "how.hypertable.com", InetAddr("72.14.204.98", 33572), InetAddr("72.14.204.98", 38060));
  context->rsc_manager->connect_server(rsc4, "cow.hypertable.com", InetAddr("69.147.125.62", 30569), InetAddr("69.147.125.62", 38060));

  {
    std::vector<MetaLog::Entity *> entities;
    entities.push_back(rsc1.get());
    entities.push_back(rsc2.get());
    entities.push_back(rsc3.get());
    entities.push_back(rsc4.get());
    context->mml_writer->record_state(entities);
  }

  expected_servers.push_back("rs1");
  expected_servers.push_back("rs2");
  expected_servers.push_back("rs3");
  expected_servers.push_back("rs4");

  entities.push_back( new OperationInitialize(context) );

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationInitialize", OperationState::STARTED) );
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateNamespace", OperationState::INITIAL) );
  run_test(context, log_dir, entities, "initialize-INITIAL:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateNamespace", OperationState::COMPLETE) );
  expected_operations.insert( std::pair<String, int32_t>("OperationInitialize", OperationState::STARTED) );
  run_test(context, log_dir, entities, "initialize-STARTED:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateNamespace", OperationState::COMPLETE) );
  expected_operations.insert( std::pair<String, int32_t>("OperationInitialize", OperationState::LOAD_ROOT_METADATA_RANGE) );
  run_test(context, log_dir, entities, "initialize-ASSIGN_METADATA_RANGES:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateNamespace", OperationState::COMPLETE) );
  expected_operations.insert( std::pair<String, int32_t>("OperationInitialize", OperationState::LOAD_ROOT_METADATA_RANGE) );
  run_test(context, log_dir, entities, "initialize-LOAD_ROOT_METADATA_RANGE:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateNamespace", OperationState::COMPLETE) );
  expected_operations.insert( std::pair<String, int32_t>("OperationInitialize", OperationState::LOAD_SECOND_METADATA_RANGE) );
  run_test(context, log_dir, entities, "initialize-LOAD_SECOND_METADATA_RANGE:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateNamespace", OperationState::COMPLETE) );
  expected_operations.insert( std::pair<String, int32_t>("OperationInitialize", OperationState::WRITE_METADATA) );
  run_test(context, log_dir, entities, "initialize-WRITE_METADATA:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateNamespace", OperationState::COMPLETE) );
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateNamespace", OperationState::INITIAL) );
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::INITIAL) );
  expected_operations.insert( std::pair<String, int32_t>("OperationInitialize", OperationState::FINALIZE) );
  run_test(context, log_dir, entities, "initialize-CREATE_RS_METRICS:throw:0",
           expected_operations, expected_servers);

  expected_operations.clear();
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateNamespace", OperationState::COMPLETE) );
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateNamespace", OperationState::INITIAL) );
  expected_operations.insert( std::pair<String, int32_t>("OperationCreateTable", OperationState::COMPLETE) );
  expected_operations.insert( std::pair<String, int32_t>("OperationInitialize", OperationState::COMPLETE) );
  run_test(context, log_dir, entities, "", expected_operations, expected_servers);

  context->op->shutdown();
  context->op->join();

  context = 0;
  _exit(0);
}


namespace {
  const char *old_schema_str = \
    "<Schema generation=\"1\">\n"
    "  <AccessGroup name=\"default\">\n"
    "    <ColumnFamily id=\"1\">\n"
    "      <Generation>1</Generation>\n"
    "      <Name>LogDir</Name>\n"
    "      <Counter>false</Counter>\n"
    "      <deleted>false</deleted>\n"
    "    </ColumnFamily>\n"
    "  </AccessGroup>\n"
    "</Schema>\n";
}



void system_upgrade_test(ContextPtr &context) {
  std::vector<MetaLog::EntityPtr> entities;
  ExpectedResultsMap expected_operations;
  std::vector<String> expected_servers;
  String log_dir = context->toplevel_dir + "/servers/master/log";

  // Write "old" schema
  String tablefile = context->toplevel_dir + "/tables/0/0";
  uint64_t handle = context->hyperspace->open(tablefile, OPEN_FLAG_READ|OPEN_FLAG_WRITE);
  context->hyperspace->attr_set(handle, "schema", old_schema_str, strlen(old_schema_str));
  context->hyperspace->close(handle);

  context->mml_writer = new MetaLog::Writer(context->dfs, context->mml_definition,
                                            log_dir + "/" + context->mml_definition->name(),
                                            entities);

  entities.push_back( new OperationSystemUpgrade(context) );

  expected_operations.clear();
  run_test(context, log_dir, entities, "", expected_operations, expected_servers);

  context->op->shutdown();
  context->op->join();

  context = 0;
  _exit(0);
}


void move_range_test(ContextPtr &context) {
  std::vector<MetaLog::EntityPtr> entities;
  ExpectedResultsMap expected_operations;
  std::vector<String> expected_servers;
  String log_dir = context->toplevel_dir + "/servers/master/log";
  RangeServerConnectionPtr rsc1, rsc2, rsc3, rsc4;
  OperationMoveRangePtr move_range_operation;

  context->mml_writer = new MetaLog::Writer(context->dfs,
          context->mml_definition,
          log_dir + "/" + context->mml_definition->name(),
          entities);

  rsc1 = new RangeServerConnection("rs1", "foo.hypertable.com", InetAddr("72.14.204.99", 38060));
  rsc2 = new RangeServerConnection("rs2", "bar.hypertable.com", InetAddr("69.147.125.65", 38060));
  rsc3 = new RangeServerConnection("rs3", "how.hypertable.com", InetAddr("72.14.204.98", 38060));
  rsc4 = new RangeServerConnection("rs4", "cow.hypertable.com", InetAddr("69.147.125.62", 38060));

  context->rsc_manager->connect_server(rsc1, "foo.hypertable.com",
          InetAddr("72.14.204.99", 33567), InetAddr("72.14.204.99", 38060));
  context->rsc_manager->connect_server(rsc2, "bar.hypertable.com",
          InetAddr("69.147.125.65", 30569), InetAddr("69.147.125.65", 38060));
  context->rsc_manager->connect_server(rsc3, "how.hypertable.com",
          InetAddr("72.14.204.98", 33572), InetAddr("72.14.204.98", 38060));
  context->rsc_manager->connect_server(rsc4, "cow.hypertable.com",
          InetAddr("69.147.125.62", 30569), InetAddr("69.147.125.62", 38060));

  {
    std::vector<MetaLog::Entity *> entities;
    entities.push_back(rsc1.get());
    entities.push_back(rsc2.get());
    entities.push_back(rsc3.get());
    entities.push_back(rsc4.get());
    context->mml_writer->record_state(entities);
  }

  expected_servers.push_back("rs1");
  expected_servers.push_back("rs2");
  expected_servers.push_back("rs3");
  expected_servers.push_back("rs4");

  TableIdentifier table;
  RangeSpec range;
  String transfer_log = "/hypertable/servers/log/xferlog";
  uint64_t soft_limit = 100000000;

  table.id = "3/6/3";
  table.generation = 2;
  range.start_row = "bar";
  range.end_row = "foo";

  move_range_operation = new OperationMoveRange(context, "rs1", table, range,
          transfer_log, soft_limit, true);

  entities.push_back(move_range_operation.get());
  entities.push_back(context->get_balance_plan_authority());

  expected_operations.clear();
  expected_operations.insert(std::pair<String, int32_t>("OperationMoveRange",
            OperationState::STARTED));
  run_test(context, log_dir, entities, "move-range-INITIAL-b:throw:0",
           expected_operations, expected_servers);

  String initial_location = move_range_operation->get_location();

  expected_operations.clear();
  expected_operations.insert(std::pair<String, int32_t>("OperationMoveRange",
              OperationState::COMPLETE));
  run_test(context, log_dir, entities, "",
          expected_operations, expected_servers);

  String final_location = move_range_operation->get_location();

  HT_ASSERT(initial_location == final_location);

  context->op->shutdown();
  context->op->join();

  context = 0;
  _exit(0);
}

void fill_ranges(vector<QualifiedRangeSpec> &root_specs,
                 vector<RangeState> &root_states,
                 vector<QualifiedRangeSpec> &metadata_specs,
                 vector<RangeState> &metadata_states,
                 vector<QualifiedRangeSpec> &system_specs,
                 vector<RangeState> &system_states,
                 vector<QualifiedRangeSpec> &user_specs,
                 vector<RangeState> &user_states)
{
  QualifiedRangeSpec spec;

  root_specs.clear();
  root_states.clear();
  spec = QualifiedRangeSpec(TableIdentifier("0/0"), RangeSpec("start_root0", "end_root0"));
  root_specs.push_back(spec);
  root_states.push_back(RangeState());
  spec = QualifiedRangeSpec(TableIdentifier("0/0"), RangeSpec("start_root1", "end_root1"));
  root_specs.push_back(spec);
  root_states.push_back(RangeState());

  metadata_specs.clear();
  metadata_states.clear();
  spec = QualifiedRangeSpec(TableIdentifier("1/0"), RangeSpec("start_meta1", "end_meta1"));
  metadata_specs.push_back(spec);
  metadata_states.push_back(RangeState());
  spec = QualifiedRangeSpec(TableIdentifier("1/0"), RangeSpec("start_meta2", "end_meta2"));
  metadata_specs.push_back(spec);
  metadata_states.push_back(RangeState());

  system_specs.clear();
  system_states.clear();

  user_specs.clear();
  user_states.clear();
  spec = QualifiedRangeSpec(TableIdentifier("2/0"), RangeSpec("start_user1", "end_user1"));
  user_specs.push_back(spec);
  user_states.push_back(RangeState());
  spec = QualifiedRangeSpec(TableIdentifier("2/0"), RangeSpec("start_user2", "end_user2"));
  user_specs.push_back(spec);
  user_states.push_back(RangeState());
  spec = QualifiedRangeSpec(TableIdentifier("2/0"), RangeSpec("start_user3", "end_user3"));
  user_specs.push_back(spec);
  user_states.push_back(RangeState());
}

void balance_plan_authority_test(ContextPtr &context) {
  std::ofstream out("balance_plan_authority_test.output");
  std::vector<MetaLog::EntityPtr> entities;
  String log_dir = context->toplevel_dir + "/servers/master/log";

  context->mml_writer = new MetaLog::Writer(context->dfs,
          context->mml_definition,
          log_dir + "/" + context->mml_definition->name(), entities);

  RangeServerConnectionPtr rsc1, rsc2, rsc3, rsc4, rsc5;
  rsc1 = new RangeServerConnection("rs1",
          "foo.hypertable.com", InetAddr("72.14.204.99", 38060));
  rsc2 = new RangeServerConnection("rs2",
          "bar.hypertable.com", InetAddr("69.147.125.65", 38060));
  rsc3 = new RangeServerConnection("rs3",
          "how.hypertable.com", InetAddr("72.14.204.98", 38060));
  rsc4 = new RangeServerConnection("rs4",
          "cow.hypertable.com", InetAddr("69.147.125.62", 38060));
  rsc5 = new RangeServerConnection("rs5",
          "boo.hypertable.com", InetAddr("70.147.125.62", 38060));

  context->rsc_manager->connect_server(rsc1, "foo.hypertable.com",
          InetAddr("72.14.204.99", 33567), InetAddr("72.14.204.99", 38060));
  context->rsc_manager->connect_server(rsc2, "bar.hypertable.com",
          InetAddr("69.147.125.65", 30569), InetAddr("69.147.125.65", 38060));
  context->rsc_manager->connect_server(rsc3, "how.hypertable.com",
          InetAddr("72.14.204.98", 33572), InetAddr("72.14.204.98", 38060));
  context->rsc_manager->connect_server(rsc4, "cow.hypertable.com",
          InetAddr("69.147.125.62", 30569), InetAddr("69.147.125.62", 38060));
  context->rsc_manager->connect_server(rsc5, "boo.hypertable.com",
          InetAddr("70.147.125.62", 30569), InetAddr("70.147.125.62", 38060));

  {
    std::vector<MetaLog::Entity *> entities;
    entities.push_back(rsc1.get());
    entities.push_back(rsc2.get());
    entities.push_back(rsc3.get());
    entities.push_back(rsc4.get());
    entities.push_back(rsc5.get());
    context->mml_writer->record_state(entities);
  }

  BalancePlanAuthority *bpa = context->get_balance_plan_authority();

  vector<QualifiedRangeSpec> root_specs;
  vector<RangeState> root_states;
  vector<QualifiedRangeSpec> metadata_specs;
  vector<RangeState> metadata_states;
  vector<QualifiedRangeSpec> system_specs;
  vector<RangeState> system_states;
  vector<QualifiedRangeSpec> user_specs;
  vector<RangeState> user_states;

  // populate ranges
  fill_ranges(root_specs, root_states, metadata_specs, metadata_states,
              system_specs, system_states, user_specs, user_states);
  rsc1->set_removed();
  bpa->create_recovery_plan("rs1", root_specs, root_states, metadata_specs, metadata_states,
          system_specs, system_states, user_specs, user_states);
  out << "Recovered rsc1: " << *bpa << std::endl << std::endl;

  fill_ranges(root_specs, root_states, metadata_specs, metadata_states,
              system_specs, system_states, user_specs, user_states);
  rsc2->set_removed();
  bpa->create_recovery_plan("rs2", root_specs, root_states, metadata_specs, metadata_states,
          system_specs, system_states, user_specs, user_states);
  out << "Recovered rsc2: " << *bpa << std::endl << std::endl;

  fill_ranges(root_specs, root_states, metadata_specs, metadata_states,
              system_specs, system_states, user_specs, user_states);
  rsc3->set_removed();
  bpa->create_recovery_plan("rs3", root_specs, root_states, metadata_specs, metadata_states,
          system_specs, system_states, user_specs, user_states);
  out << "Recovered rsc3: " << *bpa << std::endl << std::endl;

  fill_ranges(root_specs, root_states, metadata_specs, metadata_states,
              system_specs, system_states, user_specs, user_states);
  rsc4->set_removed();
  bpa->create_recovery_plan("rs4", root_specs, root_states, metadata_specs, metadata_states,
          system_specs, system_states, user_specs, user_states);
  out << "Recovered rsc4: " << *bpa << std::endl;
  out.flush();

  // remove the timestamp, then compare against the golden file
  String cmd = "cat balance_plan_authority_test.output | perl -e 'while (<>) { s/timestamp=.*?201\\d,/timestamp=0,/g; print; }' > output; diff output balance_plan_authority_test.golden";
  if (system(cmd.c_str()) != 0) {
    std::cout << "balance_plan_authority_test.output differs from golden file\n";
    _exit(1);
  }

  context = 0;
  _exit(0);
}
