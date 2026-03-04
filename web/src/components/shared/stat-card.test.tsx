import { render } from "@testing-library/preact";
import { StatCard } from "./stat-card";

describe("StatCard", () => {
  it("renders a numeric value and label", () => {
    const { getByText } = render(<StatCard value={42} label="Devices" />);
    expect(getByText("42")).toBeInTheDocument();
    expect(getByText("Devices")).toBeInTheDocument();
  });

  it("renders a string value", () => {
    const { getByText } = render(<StatCard value="N/A" label="Signal" />);
    expect(getByText("N/A")).toBeInTheDocument();
  });

  it("applies custom color", () => {
    const { getByText } = render(<StatCard value={7} label="Alerts" color="#ff0000" />);
    const valueEl = getByText("7");
    expect(valueEl).toHaveStyle({ color: "#ff0000" });
  });
});
