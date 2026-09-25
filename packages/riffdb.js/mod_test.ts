import * as riffdb from "./mod.ts";
import { assert, assertEquals, assertExists, assertRejects } from "@std/assert";

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

async function withDb(
  fn: (sql: Awaited<ReturnType<typeof riffdb.connect>>) => Promise<void>,
) {
  const sql = await riffdb.connect({ host: "http://localhost:9889" });
  try {
    await fn(sql);
  } finally {
    // Static identifiers – do NOT interpolate table names as parameters
    const drops = [
      "DROP TABLE IF EXISTS order_items",
      "DROP TABLE IF EXISTS orders",
      "DROP TABLE IF EXISTS posts",
      "DROP TABLE IF EXISTS comments",
      "DROP TABLE IF EXISTS products",
      "DROP TABLE IF EXISTS logs",
      "DROP TABLE IF EXISTS documents",
      "DROP TABLE IF EXISTS fts_docs",
      "DROP TABLE IF EXISTS counters",
      "DROP TABLE IF EXISTS blobs",
      "DROP TABLE IF EXISTS events",
      "DROP TABLE IF EXISTS trees",
      "DROP TABLE IF EXISTS json_data",
      "DROP TABLE IF EXISTS numbers", // used by CTE test
      "DROP TABLE IF EXISTS sales", // used by window-functions test
      "DROP TABLE IF EXISTS users",
      "DROP VIEW IF EXISTS active_users",
    ];
    for (const stmt of drops) {
      try {
        // If your client only accepts a tagged template, use:
        // await sql.exec([stmt] as any);
        // or expose a raw/unsafe helper. The important part is that the
        // table/view name is *not* a bound parameter.
        await sql.exec(stmt); // adjust to your client's raw API
        // Safer alternative if the client supports plain strings:
        // await (sql as any).exec(stmt);
      } catch {
        // ignore
      }
    }
  }
}

// ---------------------------------------------------------------------------
// 1. Basic DDL / DML
// ---------------------------------------------------------------------------

Deno.test("create table + insert + select + drop", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE users (
        id   INTEGER PRIMARY KEY,
        name TEXT NOT NULL
      )
    `;

    const id = 42;
    const name = "alice";

    const rows = await sql<{ id: number; name: string }>`
      INSERT INTO users (id, name) VALUES (${id}, ${name})
      RETURNING id, name
    `;

    assertEquals(rows.length, 1);
    assertEquals(rows[0].id, id);
    assertEquals(rows[0].name, name);

    const selected = await sql<{ id: number; name: string }>`
      SELECT id, name FROM users WHERE id = ${id}
    `;
    assertEquals(selected[0].name, "alice");

    await sql.exec`DROP TABLE users`;
  });
});

Deno.test("insert or replace / upsert", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE users (
        id   INTEGER PRIMARY KEY,
        name TEXT NOT NULL
      )
    `;

    await sql.exec`INSERT INTO users (id, name) VALUES (1, 'first')`;

    const rows = await sql<{ id: number; name: string }>`
      INSERT OR REPLACE INTO users (id, name) VALUES (1, 'second')
      RETURNING id, name
    `;
    assertEquals(rows[0].name, "second");

    await sql.exec`DROP TABLE users`;
  });
});

Deno.test("update + delete + row count", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE users (
        id   INTEGER PRIMARY KEY,
        name TEXT NOT NULL,
        age  INTEGER
      )
    `;

    await sql.exec`
      INSERT INTO users (id, name, age) VALUES
        (1, 'a', 10),
        (2, 'b', 20),
        (3, 'c', 30)
    `;

    await sql.exec`UPDATE users SET age = age + 1 WHERE id = 2`;

    const updated = await sql<
      { age: number }
    >`SELECT age FROM users WHERE id = 2`;
    assertEquals(updated[0].age, 21);

    await sql.exec`DELETE FROM users WHERE age < 25`;

    const remaining = await sql<
      { id: number }
    >`SELECT id FROM users ORDER BY id`;
    assertEquals(remaining.map((r) => r.id), [3]);

    await sql.exec`DROP TABLE users`;
  });
});

// ---------------------------------------------------------------------------
// 2. Constraints
// ---------------------------------------------------------------------------

Deno.test("primary key uniqueness violation", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE users (
        id   INTEGER PRIMARY KEY,
        name TEXT NOT NULL
      )
    `;
    await sql.exec`INSERT INTO users (id, name) VALUES (1, 'a')`;

    await assertRejects(
      () => sql.exec`INSERT INTO users (id, name) VALUES (1, 'b')`,
      Error,
    );

    await sql.exec`DROP TABLE users`;
  });
});

Deno.test("NOT NULL violation", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE users (
        id   INTEGER PRIMARY KEY,
        name TEXT NOT NULL
      )
    `;

    await assertRejects(
      () => sql.exec`INSERT INTO users (id, name) VALUES (1, NULL)`,
      Error,
    );

    await sql.exec`DROP TABLE users`;
  });
});

Deno.test("UNIQUE constraint", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE users (
        id    INTEGER PRIMARY KEY,
        email TEXT UNIQUE
      )
    `;
    await sql.exec`INSERT INTO users (id, email) VALUES (1, 'a@x.com')`;

    await assertRejects(
      () => sql.exec`INSERT INTO users (id, email) VALUES (2, 'a@x.com')`,
      Error,
    );

    await sql.exec`DROP TABLE users`;
  });
});

Deno.test("CHECK constraint", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE products (
        id    INTEGER PRIMARY KEY,
        price REAL CHECK (price > 0)
      )
    `;

    await assertRejects(
      () => sql.exec`INSERT INTO products (id, price) VALUES (1, -5)`,
      Error,
    );

    await sql.exec`INSERT INTO products (id, price) VALUES (1, 9.99)`;
    await sql.exec`DROP TABLE products`;
  });
});

Deno.test("FOREIGN KEY constraint", async () => {
  await withDb(async (sql) => {
    await sql.exec`PRAGMA foreign_keys = ON`;

    await sql.exec`
      CREATE TABLE users (
        id   INTEGER PRIMARY KEY,
        name TEXT
      )
    `;
    await sql.exec`
      CREATE TABLE posts (
        id      INTEGER PRIMARY KEY,
        user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
        title   TEXT
      )
    `;

    await sql.exec`INSERT INTO users (id, name) VALUES (1, 'alice')`;
    await sql
      .exec`INSERT INTO posts (id, user_id, title) VALUES (10, 1, 'hello')`;

    await assertRejects(
      () =>
        sql
          .exec`INSERT INTO posts (id, user_id, title) VALUES (11, 999, 'orphan')`,
      Error,
    );

    await sql.exec`DELETE FROM users WHERE id = 1`;
    const posts = await sql`SELECT id FROM posts`;
    assertEquals(posts.length, 0);

    await sql.exec`DROP TABLE posts`;
    await sql.exec`DROP TABLE users`;
  });
});

// ---------------------------------------------------------------------------
// 3. Indexes & EXPLAIN
// ---------------------------------------------------------------------------

Deno.test("create index and query plan", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE logs (
        id   INTEGER PRIMARY KEY,
        ts   INTEGER,
        msg  TEXT
      )
    `;
    await sql.exec`CREATE INDEX idx_logs_ts ON logs(ts)`;

    for (let i = 0; i < 100; i++) {
      await sql.exec`INSERT INTO logs (id, ts, msg) VALUES (${i}, ${
        1000 + i
      }, ${"msg" + i})`;
    }

    const plan =
      await sql`EXPLAIN QUERY PLAN SELECT * FROM logs WHERE ts = 1050`;
    // Just ensure it returns something; exact plan text can vary
    assert(plan.length > 0);

    await sql.exec`DROP TABLE logs`;
  });
});

// ---------------------------------------------------------------------------
// 4. Joins, aggregates, GROUP BY, HAVING
// ---------------------------------------------------------------------------

Deno.test("joins + aggregates", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)
    `;
    await sql.exec`
      CREATE TABLE posts (id INTEGER PRIMARY KEY, user_id INTEGER, title TEXT)
    `;

    await sql.exec`INSERT INTO users VALUES (1, 'alice'), (2, 'bob')`;
    await sql.exec`
      INSERT INTO posts VALUES
        (10, 1, 'post1'),
        (11, 1, 'post2'),
        (12, 2, 'post3')
    `;

    const rows = await sql<{ name: string; cnt: number }>`
      SELECT u.name, COUNT(p.id) AS cnt
      FROM users u
      LEFT JOIN posts p ON p.user_id = u.id
      GROUP BY u.id
      HAVING cnt >= 1
      ORDER BY cnt DESC
    `;

    assertEquals(rows.length, 2);
    assertEquals(rows[0].name, "alice");
    assertEquals(rows[0].cnt, 2);
    assertEquals(rows[1].name, "bob");
    assertEquals(rows[1].cnt, 1);

    await sql.exec`DROP TABLE posts`;
    await sql.exec`DROP TABLE users`;
  });
});

// ---------------------------------------------------------------------------
// 5. CTEs (WITH)
// ---------------------------------------------------------------------------

Deno.test("common table expression", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE numbers (n INTEGER)
    `;
    await sql.exec`INSERT INTO numbers VALUES (1),(2),(3),(4),(5)`;

    const rows = await sql<{ n: number; doubled: number }>`
      WITH doubled AS (
        SELECT n, n * 2 AS doubled FROM numbers WHERE n > 2
      )
      SELECT * FROM doubled ORDER BY n
    `;

    assertEquals(rows.map((r) => r.doubled), [6, 8, 10]);

    await sql.exec`DROP TABLE numbers`;
  });
});

// ---------------------------------------------------------------------------
// 6. Window functions
// ---------------------------------------------------------------------------

Deno.test("window functions", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE sales (
        id     INTEGER PRIMARY KEY,
        region TEXT,
        amount REAL
      )
    `;
    await sql.exec`
      INSERT INTO sales VALUES
        (1, 'east', 100),
        (2, 'east', 200),
        (3, 'west', 150),
        (4, 'west', 50)
    `;

    const rows = await sql<{
      region: string;
      amount: number;
      rank: number;
      total: number;
    }>`
      SELECT
        region,
        amount,
        RANK() OVER (PARTITION BY region ORDER BY amount DESC) AS rank,
        SUM(amount) OVER (PARTITION BY region) AS total
      FROM sales
      ORDER BY region, rank
    `;

    assertEquals(rows[0].region, "east");
    assertEquals(rows[0].rank, 1);
    assertEquals(rows[0].amount, 200);
    assertEquals(rows[0].total, 300);

    await sql.exec`DROP TABLE sales`;
  });
});

// ---------------------------------------------------------------------------
// 7. JSON support
// ---------------------------------------------------------------------------

Deno.test("JSON functions", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE json_data (
        id   INTEGER PRIMARY KEY,
        data TEXT
      )
    `;

    const payload = JSON.stringify({
      name: "widget",
      tags: ["a", "b"],
      meta: { v: 1 },
    });
    await sql.exec`INSERT INTO json_data (id, data) VALUES (1, ${payload})`;

    const rows = await sql<{ name: string; tag0: string; v: number }>`
      SELECT
        json_extract(data, '$.name') AS name,
        json_extract(data, '$.tags[0]') AS tag0,
        json_extract(data, '$.meta.v') AS v
      FROM json_data
    `;

    assertEquals(rows[0].name, "widget");
    assertEquals(rows[0].tag0, "a");
    assertEquals(rows[0].v, 1);

    await sql.exec`DROP TABLE json_data`;
  });
});

// ---------------------------------------------------------------------------
// 8. Full-text search (FTS5)
// ---------------------------------------------------------------------------

Deno.test("FTS5 full-text search", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE VIRTUAL TABLE fts_docs USING fts5(title, body)
    `;

    await sql.exec`
      INSERT INTO fts_docs (title, body) VALUES
        ('Hello World', 'This is a test document about SQLite'),
        ('Another Doc', 'Full text search is powerful'),
        ('Third', 'Nothing relevant here')
    `;

    const rows = await sql<{ title: string }>`
      SELECT title FROM fts_docs WHERE fts_docs MATCH 'SQLite OR powerful'
      ORDER BY rank
    `;

    assertEquals(rows.length, 2);
    assert(rows.some((r) => r.title === "Hello World"));
    assert(rows.some((r) => r.title === "Another Doc"));

    await sql.exec`DROP TABLE fts_docs`;
  });
});

// ---------------------------------------------------------------------------
// 9. Triggers
// ---------------------------------------------------------------------------

Deno.test("AFTER INSERT trigger", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE users (
        id   INTEGER PRIMARY KEY,
        name TEXT
      )
    `;
    await sql.exec`
      CREATE TABLE logs (
        id      INTEGER PRIMARY KEY AUTOINCREMENT,
        user_id INTEGER,
        action  TEXT
      )
    `;
    await sql.exec`
      CREATE TRIGGER log_user_insert
      AFTER INSERT ON users
      BEGIN
        INSERT INTO logs (user_id, action) VALUES (NEW.id, 'created');
      END
    `;

    await sql.exec`INSERT INTO users (id, name) VALUES (1, 'alice')`;

    const logs = await sql<{ user_id: number; action: string }>`
      SELECT user_id, action FROM logs
    `;
    assertEquals(logs.length, 1);
    assertEquals(logs[0].user_id, 1);
    assertEquals(logs[0].action, "created");

    await sql.exec`DROP TABLE logs`;
    await sql.exec`DROP TABLE users`;
  });
});

// ---------------------------------------------------------------------------
// 10. Views
// ---------------------------------------------------------------------------

Deno.test("create and query view", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, active INTEGER)
    `;
    await sql.exec`
      INSERT INTO users VALUES (1, 'alice', 1), (2, 'bob', 0), (3, 'carol', 1)
    `;

    await sql.exec`
      CREATE VIEW active_users AS
      SELECT id, name FROM users WHERE active = 1
    `;

    const rows = await sql<{ id: number; name: string }>`
      SELECT * FROM active_users ORDER BY id
    `;
    assertEquals(rows.map((r) => r.name), ["alice", "carol"]);

    await sql.exec`DROP VIEW active_users`;
    await sql.exec`DROP TABLE users`;
  });
});

// ---------------------------------------------------------------------------
// 11. Transactions
// ---------------------------------------------------------------------------

Deno.test("explicit transaction commit", async () => {
  await withDb(async (sql) => {
    await sql.exec`CREATE TABLE counters (id INTEGER PRIMARY KEY, val INTEGER)`;
    await sql.exec`INSERT INTO counters VALUES (1, 0)`;

    await sql.exec`BEGIN`;
    await sql.exec`UPDATE counters SET val = val + 10 WHERE id = 1`;
    await sql.exec`COMMIT`;

    const rows = await sql<{ val: number }>`SELECT val FROM counters`;
    assertEquals(rows[0].val, 10);

    await sql.exec`DROP TABLE counters`;
  });
});

Deno.test("transaction rollback", async () => {
  await withDb(async (sql) => {
    await sql.exec`CREATE TABLE counters (id INTEGER PRIMARY KEY, val INTEGER)`;
    await sql.exec`INSERT INTO counters VALUES (1, 0)`;

    await sql.exec`BEGIN`;
    await sql.exec`UPDATE counters SET val = val + 10 WHERE id = 1`;
    await sql.exec`ROLLBACK`;

    const rows = await sql<{ val: number }>`SELECT val FROM counters`;
    assertEquals(rows[0].val, 0);

    await sql.exec`DROP TABLE counters`;
  });
});

// ---------------------------------------------------------------------------
// 12. NULL handling & COALESCE
// ---------------------------------------------------------------------------

Deno.test("NULL handling", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE users (
        id   INTEGER PRIMARY KEY,
        name TEXT,
        age  INTEGER
      )
    `;
    await sql.exec`INSERT INTO users (id, name, age) VALUES (1, 'alice', NULL)`;

    const rows = await sql<{ age: number | null; coalesced: number }>`
      SELECT age, COALESCE(age, 0) AS coalesced FROM users
    `;
    assertEquals(rows[0].age, null);
    assertEquals(rows[0].coalesced, 0);

    await sql.exec`DROP TABLE users`;
  });
});

// ---------------------------------------------------------------------------
// 13. BLOB / binary data
// ---------------------------------------------------------------------------

Deno.test("BLOB round-trip", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE blobs (
        id   INTEGER PRIMARY KEY,
        data BLOB
      )
    `;

    const bytes = new Uint8Array([0x00, 0x01, 0x02, 0xff, 0xfe]);
    // Assuming the client accepts Uint8Array / ArrayBuffer for BLOB params
    await sql.exec`INSERT INTO blobs (id, data) VALUES (1, ${bytes})`;

    const rows = await sql<
      { data: Uint8Array }
    >`SELECT data FROM blobs WHERE id = 1`;
    assertExists(rows[0].data);
    // Depending on how the driver returns BLOBs (Uint8Array, ArrayBuffer, base64…)
    // adjust the assertion accordingly. Here we assume Uint8Array.
    assertEquals(Array.from(rows[0].data), Array.from(bytes));

    await sql.exec`DROP TABLE blobs`;
  });
});

// ---------------------------------------------------------------------------
// 14. Date / time functions
// ---------------------------------------------------------------------------

Deno.test("date and time functions", async () => {
  await withDb(async (sql) => {
    const rows = await sql<{
      now: string;
      date: string;
      unix: number;
    }>`
      SELECT
        datetime('now') AS now,
        date('now') AS date,
        unixepoch('now') AS unix
    `;

    assertExists(rows[0].now);
    assertExists(rows[0].date);
    assert(typeof rows[0].unix === "number");
  });
});

// ---------------------------------------------------------------------------
// 15. Parameter edge cases
// ---------------------------------------------------------------------------

Deno.test("many parameters + different types", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE events (
        id     INTEGER PRIMARY KEY,
        name   TEXT,
        score  REAL,
        active INTEGER,
        note   TEXT
      )
    `;

    const id = 7;
    const name = "test event";
    const score = 3.14159;
    const active = 1;
    const note = null;

    const rows = await sql<{
      id: number;
      name: string;
      score: number;
      active: number;
      note: string | null;
    }>`
      INSERT INTO events (id, name, score, active, note)
      VALUES (${id}, ${name}, ${score}, ${active}, ${note})
      RETURNING *
    `;

    assertEquals(rows[0].id, 7);
    assertEquals(rows[0].name, "test event");
    assertEquals(rows[0].score, 3.14159);
    assertEquals(rows[0].active, 1);
    assertEquals(rows[0].note, null);

    await sql.exec`DROP TABLE events`;
  });
});

// ---------------------------------------------------------------------------
// 16. Empty result sets
// ---------------------------------------------------------------------------

Deno.test("empty result set", async () => {
  await withDb(async (sql) => {
    await sql.exec`CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)`;
    const rows = await sql`SELECT * FROM users WHERE id = 999`;
    assertEquals(rows.length, 0);
    await sql.exec`DROP TABLE users`;
  });
});

// ---------------------------------------------------------------------------
// 17. Recursive CTE (tree / hierarchy)
// ---------------------------------------------------------------------------

Deno.test("recursive CTE", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE trees (
        id        INTEGER PRIMARY KEY,
        parent_id INTEGER,
        name      TEXT
      )
    `;
    await sql.exec`
      INSERT INTO trees VALUES
        (1, NULL, 'root'),
        (2, 1, 'child1'),
        (3, 1, 'child2'),
        (4, 2, 'grandchild')
    `;

    const rows = await sql<{ id: number; name: string; depth: number }>`
      WITH RECURSIVE walk(id, name, depth) AS (
        SELECT id, name, 0 FROM trees WHERE parent_id IS NULL
        UNION ALL
        SELECT t.id, t.name, w.depth + 1
        FROM trees t
        JOIN walk w ON t.parent_id = w.id
      )
      SELECT * FROM walk ORDER BY depth, id
    `;

    assertEquals(rows.map((r) => r.name), [
      "root",
      "child1",
      "child2",
      "grandchild",
    ]);
    assertEquals(rows[3].depth, 2);

    await sql.exec`DROP TABLE trees`;
  });
});

// ---------------------------------------------------------------------------
// 18. Parallel reads
// ---------------------------------------------------------------------------

Deno.test("parallel reads", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE users (
        id   INTEGER PRIMARY KEY,
        name TEXT NOT NULL
      )
    `;

    // Seed
    for (let i = 1; i <= 50; i++) {
      await sql.exec`INSERT INTO users (id, name) VALUES (${i}, ${"user" + i})`;
    }

    // Fire many concurrent SELECTs
    const promises = Array.from(
      { length: 20 },
      (_, i) =>
        sql<{ id: number; name: string }>`
        SELECT id, name FROM users WHERE id = ${i + 1}
      `,
    );

    const results = await Promise.all(promises);

    for (let i = 0; i < results.length; i++) {
      assertEquals(results[i].length, 1);
      assertEquals(results[i][0].id, i + 1);
      assertEquals(results[i][0].name, `user${i + 1}`);
    }

    await sql.exec`DROP TABLE users`;
  });
});

// ---------------------------------------------------------------------------
// 19. Parallel inserts
// ---------------------------------------------------------------------------

Deno.test("parallel inserts", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE users (
        id   INTEGER PRIMARY KEY,
        name TEXT NOT NULL
      )
    `;

    const N = 30;
    const promises = Array.from({ length: N }, (_, i) =>
      sql.exec`
        INSERT INTO users (id, name) VALUES (${i + 1}, ${"user" + (i + 1)})
      `);

    await Promise.all(promises);

    const count = await sql<{ cnt: number }>`SELECT COUNT(*) AS cnt FROM users`;
    assertEquals(count[0].cnt, N);

    const names = await sql<{ name: string }>`
      SELECT name FROM users ORDER BY id
    `;
    assertEquals(
      names.map((r) => r.name),
      Array.from({ length: N }, (_, i) => `user${i + 1}`),
    );

    await sql.exec`DROP TABLE users`;
  });
});

// ---------------------------------------------------------------------------
// 20. Parallel mixed read + insert (the one you asked for)
// ---------------------------------------------------------------------------

Deno.test("parallel read and insert", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE users (
        id   INTEGER PRIMARY KEY,
        name TEXT NOT NULL
      )
    `;

    // Initial seed
    await sql.exec`INSERT INTO users (id, name) VALUES (0, 'seed')`;

    const readers = Array.from(
      { length: 15 },
      () => sql<{ cnt: number }>`SELECT COUNT(*) AS cnt FROM users`,
    );

    const writers = Array.from({ length: 15 }, (_, i) =>
      sql.exec`
        INSERT INTO users (id, name) VALUES (${i + 1}, ${"parallel-" + (i + 1)})
      `);

    // Run everything concurrently
    const [readResults, ..._] = await Promise.all([
      Promise.all(readers),
      ...writers,
    ]);

    // All readers should have succeeded (count may vary depending on interleaving)
    for (const rows of readResults) {
      assert(rows[0].cnt >= 1);
      assert(rows[0].cnt <= 16);
    }

    // Final state must contain all inserts
    const final = await sql<{ cnt: number }>`SELECT COUNT(*) AS cnt FROM users`;
    assertEquals(final[0].cnt, 16); // 1 seed + 15 inserts

    await sql.exec`DROP TABLE users`;
  });
});

// ---------------------------------------------------------------------------
// 21. Concurrent writers with conflict (last-write-wins / error)
// ---------------------------------------------------------------------------

Deno.test("concurrent updates on same row", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE counters (
        id  INTEGER PRIMARY KEY,
        val INTEGER NOT NULL
      )
    `;
    await sql.exec`INSERT INTO counters (id, val) VALUES (1, 0)`;

    // Many concurrent increments – depending on isolation / locking,
    // some may serialize, some may conflict. We just check final value
    // is consistent (between 1 and N) and no crash.
    const N = 20;
    const promises = Array.from(
      { length: N },
      () => sql.exec`UPDATE counters SET val = val + 1 WHERE id = 1`,
    );

    // Some drivers surface SQLITE_BUSY; we swallow those for this test
    const results = await Promise.allSettled(promises);
    const fulfilled = results.filter((r) => r.status === "fulfilled").length;
    assert(fulfilled >= 1);

    const final = await sql<
      { val: number }
    >`SELECT val FROM counters WHERE id = 1`;
    assert(final[0].val >= 1 && final[0].val <= N);

    await sql.exec`DROP TABLE counters`;
  });
});

// ---------------------------------------------------------------------------
// 22. Large batch insert
// ---------------------------------------------------------------------------

Deno.test("large batch insert", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE users (
        id   INTEGER PRIMARY KEY,
        name TEXT NOT NULL
      )
    `;

    const BATCH = 500;
    // Build a multi-row INSERT (or loop if the driver prefers single-row)
    const values = Array.from(
      { length: BATCH },
      (_, i) => `(${i + 1}, 'user${i + 1}')`,
    ).join(",");

    await sql.exec`INSERT INTO users (id, name) VALUES ${values}`;

    const count = await sql<{ cnt: number }>`SELECT COUNT(*) AS cnt FROM users`;
    assertEquals(count[0].cnt, BATCH);

    await sql.exec`DROP TABLE users`;
  });
});

// ---------------------------------------------------------------------------
// 23. PRAGMA settings
// ---------------------------------------------------------------------------

Deno.test("PRAGMA read / write", async () => {
  await withDb(async (sql) => {
    await sql.exec`PRAGMA foreign_keys = ON`;
    const fk = await sql<{ foreign_keys: number }>`PRAGMA foreign_keys`;
    assertEquals(fk[0].foreign_keys, 1);

    await sql.exec`PRAGMA journal_mode = WAL`;
    // journal_mode returns a string
    const jm = await sql`PRAGMA journal_mode`;
    assertExists(jm[0]);
  });
});

// ---------------------------------------------------------------------------
// 24. Error: syntax error
// ---------------------------------------------------------------------------

Deno.test("syntax error is rejected", async () => {
  await withDb(async (sql) => {
    await assertRejects(
      () => sql.exec`SELCT * FROM nowhere`,
      Error,
    );
  });
});

// ---------------------------------------------------------------------------
// 25. Returning clause with multiple rows
// ---------------------------------------------------------------------------

Deno.test("RETURNING with multi-row insert", async () => {
  await withDb(async (sql) => {
    await sql.exec`
      CREATE TABLE users (
        id   INTEGER PRIMARY KEY,
        name TEXT NOT NULL
      )
    `;

    const rows = await sql<{ id: number; name: string }>`
      INSERT INTO users (id, name) VALUES
        (1, 'a'),
        (2, 'b'),
        (3, 'c')
      RETURNING id, name
    `;

    assertEquals(rows.length, 3);
    assertEquals(rows.map((r) => r.name), ["a", "b", "c"]);

    await sql.exec`DROP TABLE users`;
  });
});
