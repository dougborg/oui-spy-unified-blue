interface ToggleProps {
  label: string;
  enabled: boolean;
  onToggle: () => void;
}

export function Toggle({ label, enabled, onToggle }: ToggleProps) {
  return (
    <div
      class="mb-1 flex cursor-pointer items-center justify-between rounded-md border border-border-dim px-2 py-1.5 transition-colors hover:border-accent/30"
      onClick={onToggle}
    >
      <span class="text-xs text-accent">{label}</span>
      <div
        class={`relative h-[18px] w-9 rounded-full transition-colors ${
          enabled ? "bg-accent" : "bg-neutral-700"
        }`}
      >
        <div
          class={`absolute top-0.5 h-3.5 w-3.5 rounded-full bg-white transition-all ${
            enabled ? "left-[20px]" : "left-0.5"
          }`}
        />
      </div>
    </div>
  );
}
