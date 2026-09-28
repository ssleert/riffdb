#include "Service.h"
#include "HttpResponse.h"
#include "HttpUtils.h"
#include "Log.h"
#include "Protocol.h"
#include "Query.h"
#include "Request.h"
#include "XMalloc.h"
#include <sqlite3.h>
#include <stddef.h>
#include <stdint.h>
#include <yyjson.h>

static inline ServiceError
Prepare(_Atomic(bool)* Cancel,
        const char** Err,

        uint32_t PayloadLen,
        const char Payload[PayloadLen],

        sqlite3* Db,
        sqlite3_stmt** Stmt)
{
  if (*Cancel) {
    return ServiceErrorCancel;
  }
  yyjson_doc* Doc = yyjson_read(Payload, PayloadLen, 0);
  if (Doc == NULL) {
    *Err = "query len < 3";
    return ServiceErrorIncorrectJSON;
  }
  yyjson_val* Root = yyjson_doc_get_root(Doc);

  const yyjson_val* QueryObj = yyjson_obj_get(Root, "q");
  if (QueryObj == NULL) {
    *Err = "query is empty";
    return ServiceErrorQueryEmpty;
  }

  const char* Query = yyjson_get_str(QueryObj);
  size_t QueryLen = yyjson_get_len(QueryObj);
  if (QueryLen < 3) {
    *Err = "query len < 3";
    return ServiceErrorQueryLen;
  }

  yyjson_val* Args = yyjson_obj_get(Root, "args");

  int32_t Rc = sqlite3_prepare_v3(Db, Query, QueryLen, 0, Stmt, NULL);
  if (Rc != SQLITE_OK) {
    *Err = strdup(sqlite3_errmsg(Db));
    return ServiceErrorSqlite;
  }

  if (Args != NULL && yyjson_arr_size(Args) != 0) {
    Rc = ProtocolBindJsonArgsToStmt(Args, *Stmt);
    if (Rc != 0) {
      return ServiceErrorProtocol;
    }
  }

  return ServiceOK;
}

ServiceError
ServiceExecute(ServiceState* Self)
{
  ServiceError Ret = ServiceOK;

  sqlite3_stmt* Stmt = NULL;
  yyjson_doc* Doc = NULL;

  {
    Ret = Prepare(Self->Cancel,
                  &Self->Res,
                  Self->PayloadLen,
                  Self->Payload,
                  Self->Db,
                  &Stmt);
    if (Ret != ServiceOK) {
      goto cleanup;
    }

    int32_t Rc = sqlite3_step(Stmt);
    if (Rc != SQLITE_ROW && Rc != SQLITE_DONE) {
      Self->Res = strdup(sqlite3_errmsg(Self->Db));
      Ret = ServiceErrorSqlite;
      goto cleanup;
    }

    Self->Status = 200;
    Self->ResSize = 2;
    Self->Res = "ok";

    sqlite3_finalize(Stmt);
    yyjson_doc_free(Doc);
    return ServiceOK;
  }

cleanup:
  Self->Status = 500;
  sqlite3_finalize(Stmt);
  yyjson_doc_free(Doc);
  return Ret;
}

ServiceError
ServiceQuery(ServiceState* Self)
{
  ServiceError Ret = ServiceOK;

  sqlite3_stmt* Stmt = NULL;
  yyjson_doc* Doc = NULL;

  yyjson_mut_doc* ResDoc = yyjson_mut_doc_new(NULL);
  {
    Ret = Prepare(Self->Cancel,
                  &Self->Res,
                  Self->PayloadLen,
                  Self->Payload,
                  Self->Db,
                  &Stmt);
    if (Ret != ServiceOK) {
      goto cleanup;
    }

    int32_t Rc = ProtocolJsonFromStmt(ResDoc, Stmt);
    if (Rc != SQLITE_DONE) {
      Self->Res = strdup(sqlite3_errmsg(Self->Db));
      Ret = ServiceErrorSqlite;
      goto cleanup;
    }

    size_t JsonLen = 0;
    const char* Json = yyjson_mut_write(ResDoc, YYJSON_WRITE_NOFLAG, &JsonLen);
    if (Json == NULL) {
      Self->Res = "cant create json";
      Ret = ServiceErrorCantCreateJSON;
      goto cleanup;
    }

    Self->Status = 200;
    Self->ResSize = JsonLen;
    Self->Res = Json;

    sqlite3_finalize(Stmt);
    yyjson_doc_free(Doc);
    yyjson_mut_doc_free(ResDoc);
    return ServiceOK;
  }

cleanup:
  Self->Status = 500;
  sqlite3_finalize(Stmt);
  yyjson_doc_free(Doc);
  yyjson_mut_doc_free(ResDoc);
  return Ret;
}
