#include "Execute.h"
#include <sqlite3.h>
#include <yyjson.h>

#include "DataBase.h"
#include "HttpResponse.h"
#include "HttpUtils.h"

void
Execute(Request* Req)
{
  HttpResponse* Res = &Req->State.Response;

  yyjson_doc* Doc =
    yyjson_read(Req->State.Parser.Body, Req->State.Parser.ContentLength, 0);
  yyjson_val* Root = yyjson_doc_get_root(Doc);

  sqlite3_stmt* Stmt = NULL;
  {
    const yyjson_val* QueryObj = yyjson_obj_get(Root, "q");
    if (QueryObj == NULL) {
      HttpUtilsResError(Res, 400, "query is empty");
      goto cleanup;
    }

    const char* Query = yyjson_get_str(QueryObj);
    size_t QueryLen = yyjson_get_len(QueryObj);
    if (QueryLen < 3) {
      HttpUtilsResError(Res, 400, "query len < 3");
      goto cleanup;
    }

    yyjson_val* Args = yyjson_obj_get(Root, "args");

    int32_t Rc = sqlite3_prepare_v3(
      Req->Worker.Db, Query, QueryLen, SQLITE_PREPARE_PERSISTENT, &Stmt, NULL);
    if (Rc != SQLITE_OK) {
      HttpUtilsResError(Res, 400, sqlite3_errmsg(Req->Worker.Db));
      goto cleanup;
    }

    if (Args != NULL && yyjson_arr_size(Args) != 0) {
      DataBaseBindJsonArgsToStmt(Args, Stmt);
    }

    Rc = sqlite3_step(Stmt);
    if (Rc != SQLITE_ROW && Rc != SQLITE_DONE) {
      HttpUtilsResError(Res, 400, sqlite3_errmsg(Req->Worker.Db));
      goto cleanup;
    }

    HttpResponseStatusCode(Res, 200);
    HttpResponseBody(Res, sizeof("ok") - 1, "ok");
  }

cleanup:
  sqlite3_finalize(Stmt);
  yyjson_doc_free(Doc);
}
