interface ProgressBarProps {
  percent: number;
  height?: number;
}

export function ProgressBar({ percent, height = 20 }: ProgressBarProps) {
  const clamped = Math.min(100, Math.max(0, percent));
  return (
    <div
      class="overflow-hidden rounded border border-border-dim bg-[#001a00]"
      style={{ height: `${height}px` }}
    >
      <div
        class="h-full bg-accent transition-all duration-300"
        style={{ width: `${clamped}%` }}
      />
    </div>
  );
}
