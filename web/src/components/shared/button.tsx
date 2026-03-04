import type { ComponentChildren } from "preact";

interface ButtonProps {
  children: ComponentChildren;
  onClick?: () => void;
  variant?: "primary" | "danger" | "secondary";
  small?: boolean;
  class?: string;
}

const variants = {
  primary: "bg-accent-dim text-black hover:bg-accent active:bg-accent",
  danger: "bg-danger text-white hover:bg-danger-bright active:bg-danger-bright",
  secondary: "bg-bg-card text-accent border border-border-dim hover:border-accent/30",
};

export function Button({ children, onClick, variant = "primary", small, class: cls }: ButtonProps) {
  return (
    <button
      type="button"
      onClick={onClick}
      class={`cursor-pointer rounded-md font-mono font-bold transition-colors ${
        small ? "inline-block px-3 py-1 text-[11px]" : "mb-1.5 block w-full px-2 py-2 text-xs"
      } ${variants[variant]} ${cls ?? ""}`}
    >
      {children}
    </button>
  );
}
