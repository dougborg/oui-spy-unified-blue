import { fetchJSON, postEmpty, postForm } from "./client";

const MOCK_URL = "/api/test";

function mockFetch(body: unknown, status = 200) {
  globalThis.fetch = vi.fn().mockResolvedValue({
    ok: status >= 200 && status < 300,
    status,
    json: () => Promise.resolve(body),
  });
}

afterEach(() => {
  vi.restoreAllMocks();
});

describe("fetchJSON", () => {
  it("returns parsed JSON on success", async () => {
    mockFetch({ hello: "world" });
    const data = await fetchJSON(MOCK_URL);
    expect(data).toEqual({ hello: "world" });
    expect(fetch).toHaveBeenCalledWith(MOCK_URL);
  });

  it("throws on non-OK response", async () => {
    mockFetch(null, 500);
    await expect(fetchJSON(MOCK_URL)).rejects.toThrow("API 500");
  });
});

describe("postForm", () => {
  it("sends URL-encoded form body", async () => {
    mockFetch({ ok: true });
    await postForm(MOCK_URL, { key: "val" });

    expect(fetch).toHaveBeenCalledWith(MOCK_URL, {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: "key=val",
    });
  });

  it("throws on non-OK response", async () => {
    mockFetch(null, 404);
    await expect(postForm(MOCK_URL, {})).rejects.toThrow("API 404");
  });
});

describe("postEmpty", () => {
  it("sends POST with no body", async () => {
    mockFetch({ ok: true });
    await postEmpty(MOCK_URL);

    expect(fetch).toHaveBeenCalledWith(MOCK_URL, { method: "POST" });
  });

  it("throws on non-OK response", async () => {
    mockFetch(null, 403);
    await expect(postEmpty(MOCK_URL)).rejects.toThrow("API 403");
  });
});
