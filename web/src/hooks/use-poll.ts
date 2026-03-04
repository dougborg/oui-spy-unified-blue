import { useState, useEffect, useRef, useCallback } from "preact/hooks";
import { fetchJSON } from "../api/client";

interface UsePollResult<T> {
  data: T | null;
  error: Error | null;
  refresh: () => void;
}

export function usePoll<T>(url: string, intervalMs: number): UsePollResult<T> {
  const [data, setData] = useState<T | null>(null);
  const [error, setError] = useState<Error | null>(null);
  const timerRef = useRef<ReturnType<typeof setInterval>>(0 as unknown as ReturnType<typeof setInterval>);

  const doFetch = useCallback(() => {
    fetchJSON<T>(url)
      .then((d) => {
        setData(d);
        setError(null);
      })
      .catch((e) => setError(e));
  }, [url]);

  useEffect(() => {
    doFetch();
    timerRef.current = setInterval(doFetch, intervalMs);

    const onVisibility = () => {
      clearInterval(timerRef.current);
      if (!document.hidden) {
        doFetch();
        timerRef.current = setInterval(doFetch, intervalMs);
      }
    };

    document.addEventListener("visibilitychange", onVisibility);

    return () => {
      clearInterval(timerRef.current);
      document.removeEventListener("visibilitychange", onVisibility);
    };
  }, [url, intervalMs, doFetch]);

  return { data, error, refresh: doFetch };
}
