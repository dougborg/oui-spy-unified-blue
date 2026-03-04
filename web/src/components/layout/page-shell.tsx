import type { ComponentChildren } from "preact";

interface PageShellProps {
  children: ComponentChildren;
}

export function PageShell({ children }: PageShellProps) {
  return <main class="flex-1 overflow-y-auto p-2">{children}</main>;
}
