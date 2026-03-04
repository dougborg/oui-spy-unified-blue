interface EmptyStateProps {
  message: string;
}

export function EmptyState({ message }: EmptyStateProps) {
  return (
    <div class="py-5 text-center text-xs text-text-muted">{message}</div>
  );
}
