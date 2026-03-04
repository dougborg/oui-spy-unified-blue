import type { ComponentChildren } from "preact";

interface CardProps {
  title?: string;
  children: ComponentChildren;
}

export function Card({ title, children }: CardProps) {
  return (
    <div class="mb-1.5 rounded-md border border-border-dim bg-bg-card p-2">
      {title && <h3 class="mb-1 text-[13px] font-bold text-accent">{title}</h3>}
      {children}
    </div>
  );
}
