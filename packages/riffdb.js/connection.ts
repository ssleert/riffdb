export interface Options {
  host: string;
}

export type Args = number | string | null | Uint8Array<ArrayBuffer>;

export const connect = (options: Options) => {
  const c = async function <T = unknown>(
    strs: TemplateStringsArray,
    ...args: Args[]
  ): Promise<T[]> {
    const sql = strs.join("?");

    const req = JSON.stringify({
      q: sql,
      args: args,
    });
    const response = await fetch(options.host + "/query", {
      method: "POST",
      body: req,
      keepalive: true,
    });

    if (!response.ok) {
      let errorBody = "";
      try {
        errorBody = await response.text();
      } catch {
        void 0;
      }

      if (errorBody == "") {
        throw new TypeError(`HTTP Error: ${response.status}`);
      }

      throw new Error(errorBody);
    }

    const data = await response.json();
    return data;
  };

  c.exec = async function (
    strs: TemplateStringsArray | string,
    ...args: Args[]
  ): Promise<void> {
    let sql = "";
    if (typeof strs == "string") {
      sql = strs;
    } else {
      sql = strs.join("?");
    }

    const req = JSON.stringify({
      q: sql,
      args: args,
    });
    const response = await fetch(options.host + "/query", {
      method: "POST",
      body: req,
      keepalive: true,
    });

    if (!response.ok) {
      let errorBody = "";
      try {
        errorBody = await response.text();
      } catch {
        void 0;
      }

      if (errorBody == "") {
        throw new TypeError(`HTTP Error: ${response.status}`);
      }

      throw new Error(errorBody);
    }
    await response.text();
  };

  return c;
};
