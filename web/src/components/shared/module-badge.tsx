interface ModuleBadgeProps {
  moduleName: string;
  enabled: boolean;
}

export function ModuleBadge({ moduleName, enabled }: ModuleBadgeProps) {
  if (enabled) return null;

  return (
    <div class="mb-1.5 rounded-md border border-danger/30 bg-danger/10 px-3 py-2 text-center text-[11px] font-bold text-danger-bright">
      {moduleName.toUpperCase()} MODULE DISABLED — Enable in System
    </div>
  );
}
