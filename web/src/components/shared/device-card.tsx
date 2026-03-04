import type { ComponentChildren } from "preact";

interface DeviceCardProps {
  mac: string;
  subtitle?: string;
  children?: ComponentChildren;
}

export function DeviceCard({ mac, subtitle, children }: DeviceCardProps) {
  return (
    <div class="mb-1 rounded-md border border-border-dim bg-bg-card p-1.5">
      <div class="text-xs font-bold text-accent">
        {mac}
        {subtitle && <span class="font-normal text-text-secondary"> {subtitle}</span>}
      </div>
      {children && <div class="mt-1 flex flex-wrap gap-1 text-[10px]">{children}</div>}
    </div>
  );
}

export function Tag({ children, color }: { children: ComponentChildren; color?: string }) {
  return (
    <span class="rounded bg-accent/10 px-1.5 py-0.5" style={color ? { color } : undefined}>
      {children}
    </span>
  );
}
