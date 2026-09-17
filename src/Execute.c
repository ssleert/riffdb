#include "Execute.h"
#include "yyjson.h"

#include "Any.h"
#include "Utils.h"
#include "XMalloc.h"
#include "HttpUtils.h"
#include "Log.h"

typedef struct
{
  char* Query; // q
  Any* Args;   // args
  uint32_t ArgsSize;
} Input;

static void InputFree(Input* In) {
  for (size_t i = 0; i < In->ArgsSize; ++i) {
    if (In->Args[i].Type == AnyTypeStr) {
      XFree(In->Args[i].Str);
    }
  }

  XFree(In->Args);
}

static int32_t
ParseInput(const Request* Req, Input* In)
{
  yyjson_doc* Doc =
    yyjson_read(Req->State.Parser.Body, Req->State.Parser.ContentLength, 0);

  yyjson_val* Root = yyjson_doc_get_root(Doc);

  yyjson_val* Q = yyjson_obj_get(Root, "q");
  yyjson_val* Args = yyjson_obj_get(Root, "args");

  yyjson_val* Element;

  yyjson_arr_iter ArgsIter = yyjson_arr_iter_with(Args);

  In->ArgsSize = ArgsIter.max;
  LogTrace("In->ArgsSize = %zu", ArgsIter.max);
  In->Args = XMalloc(In->ArgsSize * sizeof(Any));

  while ((Element = yyjson_arr_iter_next(&ArgsIter))) {
    size_t Idx = ArgsIter.idx-1;
    if (yyjson_is_str(Element)) {
      In->Args[Idx] =
        (Any){ .Type = AnyTypeStr, .Str = XStrdup(yyjson_get_str(Element)) };
      continue;
    }

    if (yyjson_is_int(Element)) {
      In->Args[Idx] =
        (Any){ .Type = AnyTypeInt, .Int = yyjson_get_sint(Element) };
      continue;
    }

    if (yyjson_is_real(Element)) {
      In->Args[Idx] =
        (Any){ .Type = AnyTypeDouble, .Double = yyjson_get_real(Element) };
      continue;
    }

    if (yyjson_is_bool(Element)) {
      In->Args[Idx] =
        (Any){ .Type = AnyTypeBool, .Bool = yyjson_get_bool(Element) };
      continue;
    }

    if (yyjson_is_null(Element)) {
      In->Args[Idx] = AnyNull;
      continue;
    }
  }

  In->Query = XStrdup(yyjson_get_str(Q));

  yyjson_doc_free(Doc);

  return 0;
}

void
Execute(Request* Req)
{
  HttpResponse* Res = &Req->State.Response;  
  Input In;

  int32_t Rc = ParseInput(Req, &In);
  {
    if (Rc != 0) {
      LogWarn("execute: failed to parse input");
      HttpUtilsResError(Res, 400, "bad request");
      goto exit;
    }

    LogInfo("execute: q=%s", In.Query);
    for (uint32_t i = 0; i < In.ArgsSize; ++i) {
      const Any* A = &In.Args[i];
      switch (A->Type) {
        case AnyTypeNull: LogInfo("args[%u]=null", i); break;
        case AnyTypeInt:  LogInfo("args[%u]=%lld", i, (long long)A->Int); break;
        case AnyTypeDouble: LogInfo("args[%u]=%f", i, A->Double); break;
        case AnyTypeBool: LogInfo("args[%u]=%s", i, A->Bool ? "true" : "false"); break;
        case AnyTypeStr:  LogInfo("args[%u]=%s", i, A->Str); break;
      }
    }

    const char body[] = "execute";
    HttpResponseStatusCode(Res, 200);
    HttpResponseBody(Res, sizeof(body) - 1, body);
  }

exit:
  InputFree(&In);
}
