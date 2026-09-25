#include "Query.h"
#include "HttpResponse.h"
#include "HttpUtils.h"
#include "Log.h"
#include "Protocol.h"
#include "Request.h"
#include "XMalloc.h"
#include <sqlite3.h>
#include <stddef.h>
#include <stdint.h>
#include <yyjson.h>

void
Query(Request* Req)
{
  HttpResponse* Res = &Req->State.Response;

  yyjson_mut_doc* ResDoc = yyjson_mut_doc_new(NULL);
  yyjson_doc* Doc =
    yyjson_read(Req->State.Parser.Body, Req->State.Parser.ContentLength, 0);
  yyjson_val* Root = yyjson_doc_get_root(Doc);

  sqlite3_stmt* Stmt = NULL;
  const char* Err = NULL;
  {
    const yyjson_val* QueryObj = yyjson_obj_get(Root, "q");
    if (QueryObj == NULL) {
      Err = "query is empty";
      goto cleanup;
    }

    const char* Query = yyjson_get_str(QueryObj);
    size_t QueryLen = yyjson_get_len(QueryObj);
    if (QueryLen < 3) {
      Err = "query len < 3";
      goto cleanup;
    }

    yyjson_val* Args = yyjson_obj_get(Root, "args");

    int32_t Rc =
      sqlite3_prepare_v3(Req->Worker.Db, Query, QueryLen, 0, &Stmt, NULL);
    if (Rc != SQLITE_OK) {
      Err = sqlite3_errmsg(Req->Worker.Db);
      goto cleanup;
    }

    if (Args != NULL && yyjson_arr_size(Args) != 0) {
      ProtocolBindJsonArgsToStmt(Args, Stmt);
    }

    Rc = ProtocolJsonFromStmt(ResDoc, Stmt);
    if (Rc != SQLITE_DONE) {
      Err = sqlite3_errmsg(Req->Worker.Db);
      goto cleanup;
    }

    size_t JsonLen = 0;
    const char* Json = yyjson_mut_write(ResDoc, YYJSON_WRITE_NOFLAG, &JsonLen);
    if (Json == NULL) {
      Err = "cant create json";
      goto cleanup;
    }

    if (Req->Cancel) {
      LogWarn("request canceled: fd = %d", Req->ClientFd);
      goto cancel;
    }

    HttpResponseStatusCode(Res, 200);
    HttpResponseBody(Res, JsonLen, Json);
    XFree((void*)Json);

    yyjson_doc_free(Doc);
    yyjson_mut_doc_free(ResDoc);
    return;
  }

cleanup:
  HttpUtilsResError(Res, 400, Err);
  LogErr(Err);
cancel:
  sqlite3_finalize(Stmt);
  yyjson_doc_free(Doc);
  yyjson_mut_doc_free(ResDoc);
}
