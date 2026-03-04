interface StatCardProps {
  value: string | number;
  label: string;
  color?: string;
  style?: Record<string, string>;
}

export function StatCard({ value, label, color, style }: StatCardProps) {
  return (
    <div class="flex-1 rounded-md border border-border-dim bg-bg-card p-2 text-center">
      <div class="text-xl font-bold text-accent" style={{ color, ...style }}>
        {value}
      </div>
      <div class="mt-0.5 text-[9px] uppercase text-text-secondary">{label}</div>
    </div>
  );
}
