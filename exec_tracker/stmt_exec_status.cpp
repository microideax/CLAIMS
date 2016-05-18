/*
 * Copyright [2012-2015] DaSE@ECNU
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * /Claims/exec_tracker/stmt_exec_status.cpp
 *
 *  Created on: Apr 3, 2016
 *      Author: fzh
 *		   Email: fzhedu@gmail.com
 *
 * Description:
 *
 */

#include "./stmt_exec_status.h"

#include <boost/pool/pool.hpp>
#include <string>
#include <iostream>
#include <utility>

#include "../Environment.h"
#include "../node_manager/base_node.h"
#include "caf/all.hpp"

#include "caf/io/all.hpp"
using boost::chrono::seconds;
using caf::io::remote_actor;
using std::make_pair;
using std::string;
namespace claims {

StmtExecStatus::StmtExecStatus(string sql_stmt)
    : sql_stmt_(sql_stmt),
      query_result_(NULL),
      exec_info_("OK"),
      exec_status_(ExecStatus::kOk),
      segment_id_gen_(1) {}

StmtExecStatus::~StmtExecStatus() {
  // due to the query result should return, so shouldn't be deleted here
  // if (NULL != query_result_) delete query_result_;
  lock_.acquire();
  for (auto it = node_seg_id_to_seges_.begin();
       it != node_seg_id_to_seges_.end(); ++it) {
    delete it->second;
    it->second = NULL;
  }
  node_seg_id_to_seges_.clear();
  lock_.release();
}

RetCode StmtExecStatus::CancelStmtExec() {
  if (ExecStatus::kCancelled == exec_status_) {
    return 0;
  }
  exec_status_ = kCancelled;
  //  LOG(INFO) << query_id_ << " query should be cancelled!";
  //  for (auto it = node_seg_id_to_seges_.begin();
  //       it != node_seg_id_to_seges_.end(); ++it) {
  //    it->second->set_exec_status(SegmentExecStatus::ExecStatus::kCancelled);
  //  }
  return 0;
}
// check every segment status
bool StmtExecStatus::CouldBeDeleted() {
  // exec_status_ is set kDone or kError when the stmt is over in
  // select_exec.cpp, and then it could be deleted
  if (!(exec_status_ == kDone || exec_status_ == kError)) {
    return false;
  }
  // then check every segment of this stmt, if one is kOk (e.t. not kCancel and
  // kDone), then shouldn't delete this stmt
  for (auto it = node_seg_id_to_seges_.begin();
       it != node_seg_id_to_seges_.end(); ++it) {
    if (it->second->get_exec_status() == SegmentExecStatus::ExecStatus::kOk) {
      return false;
    }
  }
  LOG(INFO) << query_id_
            << " query can be deleted and status= " << exec_status_;
  return true;
}
bool StmtExecStatus::HaveErrorCase(u_int64_t logic_time) {
  for (auto it = node_seg_id_to_seges_.begin();
       it != node_seg_id_to_seges_.end(); ++it) {
    if (it->second->HaveErrorCase(logic_time)) {
      return true;
    }
  }
  return false;
}
RetCode StmtExecStatus::RegisterToTracker() {
  return Environment::getInstance()->get_stmt_exec_tracker()->RegisterStmtES(
      this);
}

RetCode StmtExecStatus::UnRegisterFromTracker() {
  return Environment::getInstance()->get_stmt_exec_tracker()->UnRegisterStmtES(
      query_id_, false);
}
void StmtExecStatus::AddSegExecStatus(SegmentExecStatus* seg_exec_status) {
  lock_.acquire();
  node_seg_id_to_seges_.insert(
      make_pair(seg_exec_status->get_node_segment_id(), seg_exec_status));
  lock_.release();
}

bool StmtExecStatus::UpdateSegExecStatus(
    NodeSegmentID node_segment_id, SegmentExecStatus::ExecStatus exec_status,
    string exec_info, u_int64_t logic_time) {
  if (SegmentExecStatus::ExecStatus::kError == exec_status) {
    CancelStmtExec();
  }
  lock_.acquire();
  auto it = node_seg_id_to_seges_.find(node_segment_id);
  assert(it != node_seg_id_to_seges_.end());

  lock_.release();
  if (ExecStatus::kCancelled == exec_status_ ||
      ExecStatus::kError == exec_status_) {
    it->second->UpdateStatus(SegmentExecStatus::ExecStatus::kCancelled,
                             exec_info, logic_time);
    return false;
  } else {
    it->second->UpdateStatus(exec_status, exec_info, logic_time);
  }
  //  lock_.release();
  return true;
}

}  // namespace claims
