#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "sqlite3.h"
#include "yyjson.h"

int32_t ProtocolBindJsonArgsToStmt(const yyjson_val* Args, sqlite3_stmt* Stmt);

int32_t ProtocolJsonFromStmt(yyjson_mut_doc* Doc, sqlite3_stmt* Stmt);

#endif
