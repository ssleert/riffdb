import * as riffdb from "./mod.ts";

Deno.test("simple sql test", async () => {
  const sql = await riffdb.connect({
    host: "http://localhost:9889",
  });

  const id = 123;
  const name = "simon";

  await sql.exec`
    create table users (
      id integer primary key,
      name text not null
    )
  `;

  const rows = await sql`
    insert or replace into users (
      id, name
    ) values (
      ${id}, ${name}
    )
    returning id, name
  `;

  await sql.exec`
    drop table users
  `

  console.log(rows);
});
