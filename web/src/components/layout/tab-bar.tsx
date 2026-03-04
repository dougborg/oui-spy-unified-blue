interface TabBarProps {
  tabs: readonly string[];
  active: number;
  onSelect: (index: number) => void;
}

export function TabBar({ tabs, active, onSelect }: TabBarProps) {
  return (
    <nav class="flex flex-shrink-0 overflow-x-auto border-b border-border-dim">
      {tabs.map((label, i) => (
        <button
          key={label}
          onClick={() => onSelect(i)}
          class={`min-w-0 flex-1 whitespace-nowrap px-1 py-2 text-center text-[11px] font-bold tracking-wider transition-colors ${
            i === active
              ? "border-b-2 border-accent bg-accent/5 text-accent"
              : "border-b-2 border-transparent text-text-secondary hover:text-accent"
          }`}
        >
          {label}
        </button>
      ))}
    </nav>
  );
}
