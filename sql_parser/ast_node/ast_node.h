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
 * ./sql_parser/ast_node/ast_node.h
 *  Created on: May 21, 2015 4:10:35 PM
 *      Author: fzh
 *       Email: fzhedu@gmail.com
 *
 */

#ifndef AST_NODE_H_  // NOLINT
#define AST_NODE_H_
#include <string>
#include <map>
#include <set>
#include <vector>
#include <utility>
using std::vector;
using std::pair;
using std::map;
using std::multimap;
using std::set;
using std::string;
typedef int ErrorNo;
enum AstNodeType {
  AST_NODE,
  AST_STMT_LIST,
  AST_SELECT_LIST,
  AST_SELECT_EXPR,
  AST_FROM_LIST,
  AST_TABLE,
  AST_SUBQUERY,
  AST_JOIN_CONDITION,
  AST_JOIN,
  AST_WHERE_CLAUSE,
  AST_GROUPBY_LIST,
  AST_GROUPBY_CLAUSE,
  AST_ORDERBY_LIST,
  AST_ORDERBY_CLAUSE,
  AST_HAVING_CLAUSE,
  AST_LIMIT_CLAUSE,
  AST_SELECT_INTO_CLAUSE,
  AST_COLUMN,
  AST_COLUMN_ALL,      // tb.*
  AST_COLUMN_ALL_ALL,  // *
  AST_SELECT_STMT,
  AST_EXPR_LIST,
  AST_EXPR_CONST,
  AST_EXPR_UNARY,
  AST_EXPR_FUNC,
  // NOTE: couldn't change the sequence of the underlying 3
  AST_EXPR_CMP_BINARY,
  AST_EXPR_BOOL_BINARY,
  AST_EXPR_CAL_BINARY,

  AST_CREATE_DATABASE,
  AST_CREATE_SCHEMA,
  AST_CREATE_TABLE_LIST,
  AST_CREATE_TABLE_LIST_SEL,
  AST_CREATE_TABLE_SEL,
  AST_CREATE_COL_LIST,
  AST_CREATE_DEF_NAME,
  AST_CREATE_DEF_PR_KEY,
  AST_CREATE_DEF_KEY,
  AST_CREATE_DEF_INDEX,
  AST_CREATE_SEL,
  AST_CREATE_DEF_FTEXT_INDEX,
  AST_CREATE_DEF_FTEXT_KEY,
  AST_CREATE_INDEX,
  AST_LOAD_TABLE,
  AST_INDEX_COL,
  AST_INDEX_COL_LIST,
  AST_COLUMN_ATTS,
  AST_DATA_TYPE,
  AST_ENUM,
  AST_ENUM_LIST,
  AST_OPT_LENGTH,
  AST_OPT_CSC,
  AST_CREATE_PROJECTION,
  AST_CREATE_PROJECTION_NUM,
  AST_DROP_INDEX,
  AST_DROP_DB,
  AST_DROP_SCHEMA,
  AST_DROP_TABLE,
  AST_DROP_TABLE_LIST,
  AST_INSERT_STMT,
  AST_INSERT_VALUE_LIST,
  AST_INSERT_VALUE,
  AST_INSERT_ASSIGN_LIST,
  AST_INTNUM,
  AST_APPROXNUM,
  AST_STRINGVAL,
  AST_BOOL,
};
enum ErrorNoType {
  eOK = 10000,
  eTableNotExist,
  eTableillegal,
  eTableAliasEqualExistedTable,
  eTableAliasEqualLowerAlias,
  eColumnNotExist,
  eColumnIsAmbiguous,
  eTabelHaveNotColumn,
  eColumnAliasIsAmbiguous,
  eColumnIsAmbiguousAfterAlias,
  eColumnIsAmbiguousToExistedColumn,
  eUnaryArgNotExist,
  eFromClauseIsNULL,
  eSelectClauseIsNULL,
  eGroupbyListIsNULL,
  eGroupByNotSupportColumn,
  eAggNodeExprStrIsNULL,
  eGroupbyNodeExprStrIsNULL,
  eSelectNodeExprStrIsNULL,
  eAliasConfictInSelectNode,
  eAggHaveAgg,
  eAggCouldNotInWhereClause,
  eAggCouldNotInGroupByClause,
  eAggSelectExprHaveOtherColumn,
  eHavingNotAgg,
  eTableNotExistInTableColumnALL,
  eColumnAllShouldNotInOtherClause,
  eMoreColumnsInSelectHaveALLALL,
};
// the order should be keep
enum SubExprType {
  kSingleTable,
  kIsEqualCondition,
  kMoreTable,
  kNoneTable,
};
const int TAB_SIZE = 4;
class AstNode;
/**
 * @brief The Semantic analysis middle scheme context.
 */
class SemanticContext {
 public:
  enum SQLClauseType {
    kNone,
    kFromClause,
    kWhereClause,
    kGroupByClause,
    kSelectClause,
    kHavingClause,
    kOrderByClause,
    kLimitClause
  };
  SemanticContext();
  ~SemanticContext();
  ErrorNo IsTableExist(const string table);
  ErrorNo IsColumnExist(string& table, const string column);
  ErrorNo AddTable(string table);
  ErrorNo AddTable(set<string> table);
  ErrorNo AddTableColumn(const string& table, const string& column);
  ErrorNo AddTableColumn(const multimap<string, string>& column_to_table);
  ErrorNo RebuildTableColumn(set<AstNode*>& aggregation);
  ErrorNo RebuildTableColumn();
  ErrorNo AddNewTableColumn(set<AstNode*>& new_set, bool need_clear);
  ErrorNo GetAliasColumn(const string& alias,
                         multimap<string, string>& column_to_table);

  ErrorNo AddAggregation(AstNode* agg_node);
  ErrorNo AddGroupByAttrs(AstNode* groupby_node);
  ErrorNo AddSelectAttrs(AstNode* select_node);
  void GetTableAllColumn(const string table,
                         multimap<string, string>& new_columns);
  void RemoveMore(set<AstNode*>& new_set);
  set<AstNode*> get_aggregation();
  set<AstNode*> get_groupby_attrs();
  set<AstNode*> get_select_attrs();
  multimap<string, string> get_column_to_table();
  set<string> get_tables();
  void ClearColumn();
  void ClearTable();
  AstNode* agg_upper_;
  void PrintContext();
  SQLClauseType clause_type_;
  bool have_agg;

 private:
  set<AstNode*> aggregation_;
  set<AstNode*> groupby_attrs_;
  set<AstNode*> select_attrs_;
  multimap<string, string> column_to_table_;
  set<string> tables_;
};
class PushDownConditionContext {
 public:
  struct SubExprInfo {
    AstNode* sub_expr_;
    set<string> ref_table_;
    SubExprType sub_expr_type_;
    bool is_set;
    SubExprInfo(AstNode* sub_expr, set<string> ref_table,
                SubExprType sub_expr_type)
        : sub_expr_(sub_expr),
          ref_table_(ref_table),
          sub_expr_type_(sub_expr_type),
          is_set(false) {}
  };
  PushDownConditionContext();
  void GetSubExprInfo(AstNode* expr);
  SubExprType GetSubExprType(AstNode* sub_expr, int ref_table_num);
  bool IsEqualJoinCondition(AstNode* sub_expr);
  bool IsTableSubSet(set<string>& expr_tables, set<string>& from_tables);
  void SetCondition(set<AstNode*>& equal_join_condi,
                    set<AstNode*>& normal_condi);
  std::vector<SubExprInfo*> sub_expr_info_;
  set<string> from_tables_;
};

/**
 * @brief The basic data structure of other AST nodes.
 */
class AstNode {
 public:
  explicit AstNode(AstNodeType ast_node_type);
  virtual ~AstNode();
  virtual void Print(int level = 0) const;
  virtual ErrorNo SemanticAnalisys(SemanticContext* sem_cnxt);
  virtual void RecoverExprName(string& name);
  // replace aggregation expression with one single column, and collect it if
  // aggregation in select expression
  virtual void ReplaceAggregation(AstNode*& agg_column, set<AstNode*>& agg_node,
                                  bool is_select);
  AstNodeType ast_node_type();
  virtual void GetSubExpr(vector<AstNode*>& sub_expr, bool is_top_and);
  virtual void GetRefTable(set<string>& ref_table);
  virtual void GetJoinedRoot(map<string, AstNode*> table_joined_root,
                             AstNode* joined_root){};

  virtual ErrorNo PushDownCondition(PushDownConditionContext* pdccnxt){};

  AstNodeType ast_node_type_;
  string expr_str_;
};
struct ParseResult {
  void* yyscan_info_;
  AstNode* ast;
  const char* sql_clause;
  int error_number;
};

/**
 * @brief AstStmtList is used to link every statement in one sql contains
 * multiple statement.
 * @details For example: select a from tb;select max(a) from tb2;
 */
class AstStmtList : public AstNode {
 public:
  AstStmtList(AstNodeType ast_node_type, AstNode* stmt, AstNode* next);
  ~AstStmtList();
  void Print(int level = 0) const;
  ErrorNo SemanticAnalisys(SemanticContext* sem_cnxt);
  ErrorNo PushDownCondition(PushDownConditionContext* pdccnxt);
  AstNode* stmt_;
  AstNode* next_;
};

#endif  // SQL_PARSER_AST_NODE_AST_NODE_H_