/*
 * SchemaFix.cpp
 *
 *  Created on: Jun 8, 2013
 *      Author: wangli
 */

//// this macro decides whether write DLOG message into log file.
//// Open means no DLOG message.
// #define NDEBUG
#include "SchemaFix.h"
#include <memory.h>
#include <glog/logging.h>
#include <string>
#include <algorithm>
#include <vector>

#include "../../loader/data_injector.h"
#include "../../utility/Timer.h"
#include "../common/error_define.h"
using claims::loader::DataInjector;
using claims::common::rTooFewColumn;
using claims::common::rSuccess;
using claims::common::rIncorrectData;
using claims::common::rIncorrectData;
using claims::common::rInvalidNullData;
using claims::common::rTooLongData;
using claims::common::rTooManyColumn;
using claims::common::rInvalidInsertData;

// #define SCHEMA_FIX_DEBUG
// #define SCHEMA_FIX_PERF

#ifdef CLAIMS_DEBUG_LOG
#ifdef SCHEMA_FIX_DEBUG
#define DLOG_SF(info) DLOG(INFO) << info << endl;
#else
#define DLOG_SF(info)
#endif

#ifdef SCHEMA_FIX_PERF
#define PLOG_SF(info) DLOG(INFO) << info << std::endl;
#define GET_TIME_SF(var) GETCURRENTTIME(var)
#else
#define PLOG_SF(info)
#define GET_TIME_SF(var)
#endif
#else
#define DLOG_SF(info)
#endif

#ifdef SCHEMA_FIX_PERF
#define ATOMIC_ADD_SF(var, value) __sync_add_and_fetch(&var, value);
#define GET_TIME_SF(var) GETCURRENTTIME(var);
#else
#define ATOMIC_ADD_SF(var, value)
#define GET_TIME_SF(var)
#endif

SchemaFix::SchemaFix(const std::vector<column_type>& col) : Schema(col) {
  // accum_offsets=new unsigned[columns.size()];  // new
  totalsize = 0;
  unsigned accumu = 0;
  for (unsigned i = 0; i < col.size(); i++) {
    totalsize += col[i].get_length();
    accum_offsets.push_back(accumu);
    accumu += col[i].get_length();
  }
}
SchemaFix::SchemaFix(const SchemaFix& r) : Schema(r) {
  this->accum_offsets = r.accum_offsets;
  this->totalsize = r.totalsize;
}
SchemaFix::~SchemaFix() {
  // accum_offsets.~vector();
}

unsigned SchemaFix::getTupleMaxSize() const { return totalsize; }

void SchemaFix::getColumnValue(unsigned index, void* src, void* desc) {
  assert(index < columns.size());
  assert(src != 0 && desc != 0);

  columns[index].operate->assignment(
      accum_offsets[index] + static_cast<char*>(src), desc);
}

unsigned SchemaFix::getColumnOffset(unsigned index) {
  return accum_offsets[index];
}
Schema* SchemaFix::getSubSchema(std::vector<unsigned> index) const {
  std::vector<column_type> col;
  for (unsigned i = 0; i < index.size(); i++) {
    col.push_back(columns[index[i]]);
  }
  return new SchemaFix(col);
}
Schema* SchemaFix::duplicateSchema() const { return new SchemaFix(columns); }

int SchemaFix::getColumnOffset(unsigned index) const {
  return accum_offsets[index];
}

/*
 * 检查源数据是否合法,如果来自kSQL,
 * 若出现error则直接返回,
 * 若只有warning,放入columns_validities,
 * 同时处理warning(字符串过长,截断;数字类型不在合法范围内设为默认值);
 * 如果来自kFile, 出现error将值设为默认值, 视为warning对待.
 */
RetCode SchemaFix::CheckAndToValue(std::string text_tuple, void* binary_tuple,
                                   const string attr_separator,
                                   RawDataSource raw_data_source,
                                   vector<Validity>& columns_validities) {
  int ret = rSuccess;
  string::size_type prev_pos = 0;
  string::size_type pos = 0;
  string text_column;
  columns_validities.clear();

  /**
   * let's think : '|' is column separator, '\n' is line separator
   * data format is always: xxx|xxx|xxx|......xxx|\n
   */
  GET_TIME_SF(to_value_func_time);
  for (int i = 0; i < columns.size(); ++i) {
    GET_TIME_SF(get_substr_time);

    DLOG_SF("tuple size is " << text_tuple.length());
    if (pos != string::npos && text_tuple.length() == prev_pos) {
      // meet the first column without data
      pos = string::npos;
      ret = rTooFewColumn;
      columns_validities.push_back(std::move(Validity(-1, ret)));
      if (kSQL == raw_data_source) {  // treated as error
        ELOG(ret, "Data from File is lost from column whose index is " << i);
        return ret;
      } else {  // treated as warning
        WLOG(ret, "Data from File is lost from column whose index is " << i);
        columns[i].operate->SetDefault(text_column);  // no more need to check
        DLOG_SF("after setting default, the value is "
                << text_column << "  operate's nullable is "
                << columns[i].operate->nullable);
        ret = rSuccess;
      }
    } else {
      pos = text_tuple.find(attr_separator,
                            prev_pos);  // VCLINE --- 7.6s| LINE --- 7.7 s

      if (string::npos == pos) {  // not the first column without data
        columns[i].operate->SetDefault(text_column);  // no more need to check
        ret = rSuccess;
      } else {  // correct
        // VCLINE --- 13.71-7.6 = 6.11 s| LINE  --- 16.58 - 7.7 = 8.88 s
        text_column = text_tuple.substr(prev_pos, pos - prev_pos);
        // VCLINE --- 18.0 - 13.71 = 4.29 s| LINE --- 18.82-16.58=2.24 s
        prev_pos = pos + attr_separator.length();
        DLOG_SF("after prev_pos adding, prev_pos :" << prev_pos
                                                    << " pos:" << pos);

        //  GET_TIME_SF(check_string_time);
        // VCLINE --- 20.55 - 18.0 = 2.55 s| LINE --- 27.05 - 18.82 = 8.23s
        ret = columns[i].operate->CheckSet(text_column);
        if (rIncorrectData == ret || rInvalidNullData == ret ||
            rInvalidInsertData == ret) {  // error
          if (kSQL == raw_data_source) {  // treated as error
            columns_validities.push_back(std::move(Validity(i, ret)));
            ELOG(ret, "Data from SQL is for column whose index is " << i);
            return ret;
          } else {  // treated as warning and set default
            columns_validities.push_back(std::move(Validity(i, ret)));
            columns[i].operate->SetDefault(text_column);
            ret = rSuccess;
          }
        } else if (rTooLongData == ret) {  // data truncate warning
          columns_validities.push_back(std::move(Validity(i, ret)));
          ret = rSuccess;
        } else if (rSuccess != ret) {  // other warnings
          columns_validities.push_back(std::move(Validity(i, ret)));
          columns[i].operate->SetDefault(text_column);
          ret = rSuccess;
        }
        //        ATOMIC_ADD_SF(DataInjector::total_check_string_time_,
        //                      GetElapsedTimeInUs(check_string_time));
      }
    }
    //    ATOMIC_ADD_SF(DataInjector::total_get_substr_time_,
    //                  GetElapsedTimeInUs(get_substr_time));
    //    PLOG_SF("get_substr time:" << GetElapsedTimeInUs(get_substr_time));

    //    GET_TIME_SF(to_value_time);
    DLOG_SF("Before toValue, column data is " << text_column);
    // VCLINE --- 21.87 - 20.55 = 1.32 s|LINE --- 77.84 - 27.05 = 50.79s
    columns[i].operate->toValue(
        static_cast<char*>(binary_tuple) + accum_offsets[i],
        text_column.c_str());
    DLOG_SF("Original: " << text_tuple.substr(prev_pos, pos - prev_pos).c_str()
                         << "\t Transfer: "
                         << columns[i].operate->toString(binary_tuple +
                                                         accum_offsets[i]));
    //    ATOMIC_ADD_SF(DataInjector::total_to_value_time_,
    //                  GetElapsedTimeInUs(to_value_time));
    //    PLOG_SF("just to_value time:" << GetElapsedTimeInUs(to_value_time));
    //    PLOG_SF("inner loop time:" << GetElapsedTimeInUs(get_substr_time));
  }

  //  PLOG_SF("while loop time:" << GetElapsedTimeInUs(to_value_func_time));

  ATOMIC_ADD_SF(DataInjector::total_check_and_to_value_func_time_,
                GetElapsedTimeInUs(to_value_func_time));
  DLOG_SF("after all tovalue, prev_pos :"
          << (prev_pos == string::npos) << "prev_pos+1 :"
          << (prev_pos + 1 == string::npos) << "npos :" << string::npos
          << " pos:" << pos << " prev_pos:" << prev_pos
          << " text_tuple's length:" << text_tuple.length());
  if (text_tuple.length() != prev_pos) {  // too many column data
    ret = rTooManyColumn;
    columns_validities.push_back(std::move(Validity(-1, ret)));
    if (kSQL == raw_data_source) {
      ELOG(ret, "");
      return ret;
    } else {
      //      WLOG(ret, "");  // ATTENTION: A big performance drag when
      //      loading!!!
      ret = rSuccess;
    }
  }
  //  PLOG_SF("check_and_to_value func time:" << temp << " us");
  return ret;
}

// TODO(ANYONE): implement this method, which is used when it is sure that the
// data is correct and needn't check
/*
 void SchemaFix::toValue(std::string text_tuple, void* binary_tuple,
 const char attr_separator) {
 }*/

void SchemaFix::addColumn(column_type ct, unsigned size) {
  accum_offsets.push_back(totalsize);
  columns.push_back(ct);
  totalsize += size;
}
