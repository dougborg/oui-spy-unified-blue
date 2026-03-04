interface EmptyStateProps {
  message: string;
  hint?: string;
}

export function EmptyState({ message, hint }: EmptyStateProps) {
  return (
    <div class="py-5 text-center text-xs text-text-muted">
      <div>{message}</div>
      {hint && <div class="mt-1 text-[10px] text-text-secondary">{hint}</div>}
    </div>
  );
}
