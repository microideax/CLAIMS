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
 * /Claims/loader/test/table_file_connector_test.cpp
 *
 *  Created on: Oct 29, 2015
 *      Author: yukai
 *		   Email: yukai2014@gmail.com
 *
 * Description:
 *
 */
#include "./table_file_connector_test.h"

#include <gtest/gtest.h>
#include <stdlib.h>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

#include "../../common/file_handle/file_handle_imp_factory.h"
#include "../../common/memory_handle.h"
#include "../file_connector.h"
#include "../table_file_connector.h"
#include "../single_file_connector.h"

namespace claims {

namespace loader {

using std::vector;
using std::string;
using std::endl;
using std::ostringstream;
using namespace claims::common;
using claims::common::FileOpenFlag;
using claims::common::FilePlatform;
using claims::loader::FileConnector;
using claims::loader::SingleFileConnector;

TableFileConnectorTest::TableFileConnectorTest() {
  connector_ = NULL;
  data_ = "fafasfffffffffffffffdfsfsffsfsfsfs  a.";
  data_length_ = 38;
  //  snprintf(data_, 38, "fafasfffffffffffffffdfsfsffsfsfsfs  a.");
  LOG(INFO) << "data_: " << data_ << std::endl;

  for (int i = 0; i < 3; ++i) {
    vector<string> temp;
    for (int j = 0; j < 3; ++j) {
      ostringstream oss;
      oss << "TableFileConnectorP" << i << "G" << j;
      temp.push_back(oss.str());
    }
    paths_.push_back(temp);
  }
}

void TableFileConnectorTest::SetUpTestCase() {
  std::cout << "=============" << std::endl;
  LOG(INFO) << "=============" << std::endl;
}
void TableFileConnectorTest::TearDownTestCase() {}

void TableFileConnectorTest::WriteOrAppendFile(
    common::FilePlatform file_platform, common::FileOpenFlag open_flag,
    char* expect, int expect_length) {
  connector_ = new TableFileConnector(file_platform, paths_);

  if (rSuccess != connector_->Open(open_flag)) FAIL();
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      if (rSuccess != connector_->Flush(i, j, data_, data_length_)) {
        LOG(ERROR) << "failed to flush (" << i << "," << j << ")" << endl;
        FAIL();
      }
    }
  }
  if (rSuccess != connector_->Close()) FAIL();
  DELETE_PTR(connector_);

  FileConnector* reader = NULL;
  void* read_buffer = NULL;
  uint64_t length = 0;
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      reader = new SingleFileConnector(file_platform, paths_[i][j]);
      reader->Open(kReadFile);
      if (rSuccess != reader->LoadTotalFile(read_buffer, &length)) {
        FAIL();
      }
      EXPECT_EQ(expect_length, length);
      EXPECT_STREQ(expect, static_cast<char*>(read_buffer));
      reader->Close();
      DELETE_PTR(reader);
    }
  }
  SUCCEED();
}

TEST_F(TableFileConnectorTest, DiskWrite) {
  WriteOrAppendFile(kDisk, kCreateFile, data_, data_length_);
}
TEST_F(TableFileConnectorTest, DiskAppend) {
  char double_data[] =
      "fafasfffffffffffffffdfsfsffsfsfsfs  "
      "a.fafasfffffffffffffffdfsfsffsfsfsfs  a.";
  WriteOrAppendFile(kDisk, kAppendFile, double_data, sizeof(double_data) - 1);
}
TEST_F(TableFileConnectorTest, DiskOverWrite) {
  WriteOrAppendFile(kDisk, kCreateFile, data_, data_length_);
}
TEST_F(TableFileConnectorTest, HdfsWrite) {
  WriteOrAppendFile(kHdfs, kCreateFile, data_, data_length_);
}
TEST_F(TableFileConnectorTest, HdfsAppend) {
  char double_data[] =
      "fafasfffffffffffffffdfsfsffsfsfsfs  "
      "a.fafasfffffffffffffffdfsfsffsfsfsfs  a.";
  WriteOrAppendFile(kHdfs, kAppendFile, double_data, sizeof(double_data) - 1);
}
TEST_F(TableFileConnectorTest, HdfsOverWrite) {
  WriteOrAppendFile(kHdfs, kCreateFile, data_, data_length_);
}

}  // namespace loader

}  // namespace claims
