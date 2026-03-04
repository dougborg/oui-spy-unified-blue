interface TextInputProps {
  label?: string;
  value: string;
  onInput: (val: string) => void;
  placeholder?: string;
  maxLength?: number;
}

export function TextInput({
  label,
  value,
  onInput,
  placeholder,
  maxLength,
}: TextInputProps) {
  return (
    <div class="mb-1">
      {label && (
        <label class="mb-0.5 block text-[11px] text-text-secondary">{label}</label>
      )}
      <input
        type="text"
        value={value}
        onInput={(e) => onInput((e.target as HTMLInputElement).value)}
        placeholder={placeholder}
        maxLength={maxLength}
        class="w-full rounded border border-border-dim bg-[#001a00] p-1.5 font-mono text-xs text-accent outline-none focus:border-accent"
      />
    </div>
  );
}

interface TextAreaProps {
  label?: string;
  value: string;
  onInput: (val: string) => void;
  placeholder?: string;
  rows?: number;
}

export function TextArea({
  label,
  value,
  onInput,
  placeholder,
  rows = 3,
}: TextAreaProps) {
  return (
    <div class="mb-1">
      {label && (
        <label class="mb-0.5 block text-[11px] text-text-secondary">{label}</label>
      )}
      <textarea
        value={value}
        onInput={(e) => onInput((e.target as HTMLTextAreaElement).value)}
        placeholder={placeholder}
        rows={rows}
        class="w-full resize-y rounded border border-border-dim bg-[#001a00] p-1.5 font-mono text-[11px] text-accent outline-none focus:border-accent"
      />
    </div>
  );
}
