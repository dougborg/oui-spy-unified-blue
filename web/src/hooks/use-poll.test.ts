import { act, renderHook, waitFor } from "@testing-library/preact";
import { usePoll } from "./use-poll";

const MOCK_URL = "/api/status";

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

describe("usePoll", () => {
  it("fetches on mount", async () => {
    mockFetch({ value: 1 });
    const { result } = renderHook(() => usePoll(MOCK_URL, 60_000));

    await waitFor(() => {
      expect(result.current.data).toEqual({ value: 1 });
    });
    expect(result.current.error).toBeNull();
    expect(fetch).toHaveBeenCalledTimes(1);
  });

  it("polls at the given interval", async () => {
    mockFetch({ value: 1 });
    // Use a very short interval so the second tick fires quickly
    const { result } = renderHook(() => usePoll(MOCK_URL, 50));

    await waitFor(() => {
      expect(result.current.data).toEqual({ value: 1 });
    });

    // Wait for at least one more poll tick
    await waitFor(() => {
      expect((fetch as ReturnType<typeof vi.fn>).mock.calls.length).toBeGreaterThanOrEqual(2);
    });
  });

  it("pauses polling when document is hidden", async () => {
    mockFetch({ value: 1 });
    renderHook(() => usePoll(MOCK_URL, 50));

    // Wait for initial fetch
    await waitFor(() => {
      expect(fetch).toHaveBeenCalled();
    });

    // Simulate tab hidden — clears the interval
    Object.defineProperty(document, "hidden", { value: true, configurable: true });
    act(() => {
      document.dispatchEvent(new Event("visibilitychange"));
    });

    const callsAfterHide = (fetch as ReturnType<typeof vi.fn>).mock.calls.length;

    // Wait a bit and verify no new fetches
    await new Promise((r) => setTimeout(r, 150));
    expect(fetch).toHaveBeenCalledTimes(callsAfterHide);

    // Restore
    Object.defineProperty(document, "hidden", { value: false, configurable: true });
  });

  it("resumes polling when document becomes visible", async () => {
    mockFetch({ value: 1 });
    renderHook(() => usePoll(MOCK_URL, 60_000));

    await waitFor(() => {
      expect(fetch).toHaveBeenCalledTimes(1);
    });

    // Hide
    Object.defineProperty(document, "hidden", { value: true, configurable: true });
    act(() => {
      document.dispatchEvent(new Event("visibilitychange"));
    });

    // Show again — triggers immediate fetch + new interval
    Object.defineProperty(document, "hidden", { value: false, configurable: true });
    act(() => {
      document.dispatchEvent(new Event("visibilitychange"));
    });

    await waitFor(() => {
      expect((fetch as ReturnType<typeof vi.fn>).mock.calls.length).toBeGreaterThanOrEqual(2);
    });
  });

  it("handles fetch errors", async () => {
    mockFetch(null, 500);
    const { result } = renderHook(() => usePoll(MOCK_URL, 60_000));

    await waitFor(() => {
      expect(result.current.error).toBeInstanceOf(Error);
    });
    expect(result.current.data).toBeNull();
  });

  it("cleans up interval on unmount", async () => {
    const clearSpy = vi.spyOn(globalThis, "clearInterval");
    mockFetch({ value: 1 });
    const { unmount } = renderHook(() => usePoll(MOCK_URL, 60_000));

    await waitFor(() => {
      expect(fetch).toHaveBeenCalled();
    });

    unmount();
    expect(clearSpy).toHaveBeenCalled();
  });
});
