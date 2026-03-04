import { useState } from "preact/hooks";

export function HttpsBanner() {
  const [dismissed, setDismissed] = useState(false);
  const isHTTP = typeof location !== "undefined" && location.protocol === "http:";

  if (!isHTTP || dismissed) return null;

  return (
    <div class="flex items-start gap-2 border-b border-danger/30 bg-danger/10 px-3 py-2 text-[10px] text-danger-bright">
      <div class="flex-1">
        <strong>HTTP mode</strong> — GPS features require HTTPS.{" "}
        <a href="/ca.cer" class="underline">
          Download CA Certificate
        </a>{" "}
        then install in iOS Settings &gt; Downloaded Profile &gt; Install &gt; Certificate Trust
        Settings &gt; Enable.
      </div>
      <button
        type="button"
        onClick={() => setDismissed(true)}
        class="cursor-pointer text-danger-bright hover:text-white"
      >
        X
      </button>
    </div>
  );
}
