import { useState } from "preact/hooks";
import { useToast } from "./toast";

interface CopyButtonProps {
  text: string;
}

export function CopyButton({ text }: CopyButtonProps) {
  const { toast } = useToast();
  const [copied, setCopied] = useState(false);

  const copy = async () => {
    try {
      if (navigator.clipboard) {
        await navigator.clipboard.writeText(text);
      } else {
        // Fallback for non-secure contexts
        const ta = document.createElement("textarea");
        ta.value = text;
        ta.style.position = "fixed";
        ta.style.opacity = "0";
        document.body.appendChild(ta);
        ta.select();
        document.execCommand("copy");
        document.body.removeChild(ta);
      }
      setCopied(true);
      toast("Copied!", "success");
      setTimeout(() => setCopied(false), 1500);
    } catch {
      toast("Copy failed", "error");
    }
  };

  return (
    <button
      type="button"
      onClick={(e) => {
        e.stopPropagation();
        copy();
      }}
      class="ml-1 inline-flex cursor-pointer items-center rounded px-1 py-0.5 text-[10px] text-text-secondary hover:text-accent"
      title="Copy to clipboard"
    >
      {copied ? "OK" : "CP"}
    </button>
  );
}
