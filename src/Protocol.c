#include "Protocol.h"
#include "Log.h"

int32_t
ProtocolBindJsonArgsToStmt(const yyjson_val* Args, sqlite3_stmt* Stmt)
{
  yyjson_val* Element = NULL;
  yyjson_arr_iter ArgsIter = yyjson_arr_iter_with(Args);

  while ((Element = yyjson_arr_iter_next(&ArgsIter))) {
    int32_t Idx = ArgsIter.idx;
    if (yyjson_is_str(Element)) {
      const char* Str = yyjson_get_str(Element);
      size_t Len = yyjson_get_len(Element);

      sqlite3_bind_text(Stmt, Idx, Str, Len, NULL);
      continue;
    }

    if (yyjson_is_int(Element)) {
      int64_t Int = yyjson_get_sint(Element);

      sqlite3_bind_int64(Stmt, Idx, Int);
      continue;
    }

    if (yyjson_is_real(Element)) {
      double Double = yyjson_get_real(Element);

      sqlite3_bind_double(Stmt, Idx, Double);
      continue;
    }

    if (yyjson_is_bool(Element)) {
      bool Bool = yyjson_get_bool(Element);

      sqlite3_bind_int(Stmt, Idx, (int32_t)Bool);
      continue;
    }

    if (yyjson_is_null(Element)) {
      sqlite3_bind_null(Stmt, Idx);
      continue;
    }
  }

  return 0;
}

int32_t
ProtocolJsonFromStmt(yyjson_mut_doc* Doc, sqlite3_stmt* Stmt)
{
  int32_t Rc = 0;

  yyjson_mut_val* Root = yyjson_mut_arr(Doc);
  yyjson_mut_doc_set_root(Doc, Root);

  int ColumnCount = sqlite3_column_count(Stmt);

  while ((Rc = sqlite3_step(Stmt)) == SQLITE_ROW) {
    yyjson_mut_val* Obj = yyjson_mut_obj(Doc);

    for (int i = 0; i < ColumnCount; i++) {
      const char* ColumnName = sqlite3_column_name(Stmt, i);
      int ColumnType = sqlite3_column_type(Stmt, i);

      switch (ColumnType) {
        case SQLITE_INTEGER:
          yyjson_mut_obj_add_int(
            Doc, Obj, ColumnName, sqlite3_column_int64(Stmt, i));
          break;

        case SQLITE_FLOAT:
          yyjson_mut_obj_add_real(
            Doc, Obj, ColumnName, sqlite3_column_double(Stmt, i));
          break;

        case SQLITE_TEXT: {
          const char* text = (const char*)sqlite3_column_text(Stmt, i);
          if (text) {
            yyjson_mut_obj_add_strcpy(Doc, Obj, ColumnName, text);
          } else {
            yyjson_mut_obj_add_null(Doc, Obj, ColumnName);
          }
          break;
        }

        case SQLITE_BLOB: {
          LogWarn("blobs unsupported");
          yyjson_mut_obj_add_null(Doc, Obj, ColumnName);
          break;
        }

        case SQLITE_NULL:
        default:
          yyjson_mut_obj_add_null(Doc, Obj, ColumnName);
          break;
      }
    }

    yyjson_mut_arr_append(Root, Obj);
  }

  return Rc;
}

int32_t
ProtocolBindMsgpackArgsToStmt(cw_unpack_context* uc, sqlite3_stmt* Stmt)
{
  cw_unpack_next(uc);
  if (uc->return_code != CWP_RC_OK || uc->item.type != CWP_ITEM_ARRAY) {
    return -1;
  }

  uint32_t n = uc->item.as.array.size;
  for (uint32_t i = 0; i < n; ++i) {
    int32_t Idx = (int32_t)(i + 1);

    cw_unpack_next(uc);
    if (uc->return_code != CWP_RC_OK) {
      return uc->return_code;
    }

    switch (uc->item.type) {
      case CWP_ITEM_STR:
        sqlite3_bind_text(Stmt,
                          Idx,
                          (const char*)uc->item.as.str.start,
                          (int)uc->item.as.str.length,
                          NULL);
        break;

      case CWP_ITEM_POSITIVE_INTEGER:
        if (uc->item.as.u64 <= (uint64_t)INT64_MAX) {
          sqlite3_bind_int64(Stmt, Idx, (int64_t)uc->item.as.u64);
        } else {
          sqlite3_bind_double(Stmt, Idx, (double)uc->item.as.u64);
        }
        break;

      case CWP_ITEM_NEGATIVE_INTEGER:
        sqlite3_bind_int64(Stmt, Idx, uc->item.as.i64);
        break;

      case CWP_ITEM_FLOAT:
        sqlite3_bind_double(Stmt, Idx, (double)uc->item.as.real);
        break;

      case CWP_ITEM_DOUBLE:
        sqlite3_bind_double(Stmt, Idx, uc->item.as.long_real);
        break;

      case CWP_ITEM_BOOLEAN:
        sqlite3_bind_int(Stmt, Idx, (int32_t)uc->item.as.boolean);
        break;

      case CWP_ITEM_NIL:
        sqlite3_bind_null(Stmt, Idx);
        break;

      case CWP_ITEM_BIN:
        sqlite3_bind_blob(
          Stmt, Idx, uc->item.as.bin.start, (int)uc->item.as.bin.length, NULL);
        break;

      default:
        break;
    }
  }

  return 0;
}
