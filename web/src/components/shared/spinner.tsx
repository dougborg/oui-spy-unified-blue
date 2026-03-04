export function Spinner({ size = 16 }: { size?: number }) {
  return (
    <span
      class="inline-block animate-spin rounded-full border-2 border-accent border-t-transparent"
      style={{ width: `${size}px`, height: `${size}px` }}
    />
  );
}

export function LoadingState() {
  return (
    <div class="flex items-center justify-center py-8">
      <Spinner size={24} />
      <span class="ml-2 text-xs text-text-secondary">Loading...</span>
    </div>
  );
}
