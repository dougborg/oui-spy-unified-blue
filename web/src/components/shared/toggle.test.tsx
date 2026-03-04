import { fireEvent, render } from "@testing-library/preact";
import { Toggle } from "./toggle";

describe("Toggle", () => {
  it("renders label and enabled state", () => {
    const { getByText } = render(<Toggle label="Wi-Fi" enabled={true} onToggle={() => {}} />);
    expect(getByText("Wi-Fi")).toBeInTheDocument();
  });

  it("renders disabled state", () => {
    const { container } = render(<Toggle label="GPS" enabled={false} onToggle={() => {}} />);
    const track = container.querySelector(".bg-neutral-700");
    expect(track).toBeInTheDocument();
  });

  it("calls onToggle when clicked", () => {
    const onToggle = vi.fn();
    const { getByText } = render(<Toggle label="Buzzer" enabled={false} onToggle={onToggle} />);
    fireEvent.click(getByText("Buzzer"));
    expect(onToggle).toHaveBeenCalledTimes(1);
  });
});
