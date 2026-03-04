import { useState, useEffect } from "preact/hooks";
import { usePoll } from "../../hooks/use-poll";
import { postForm } from "../../api/client";
import type { DetectorFilter, DetectorDevice } from "../../api/client";
import { Card } from "../shared/card";
import { Button } from "../shared/button";
import { TextArea } from "../shared/text-input";
import { DeviceCard, Tag } from "../shared/device-card";
import { EmptyState } from "../shared/empty-state";
import { useToast } from "../shared/toast";

interface DevicesResponse {
  devices: DetectorDevice[];
}

export function DetectorPage() {
  const { toast } = useToast();
  const { data: filters } = usePoll<DetectorFilter[]>(
    "/api/detector/filters",
    10000,
  );
  const { data: devicesResp } =
    usePoll<DevicesResponse>("/api/detector/devices", 3000);

  const [ouis, setOuis] = useState("");
  const [macs, setMacs] = useState("");
  const [filtersLoaded, setFiltersLoaded] = useState(false);

  useEffect(() => {
    if (filters && !filtersLoaded) {
      const ouiList: string[] = [];
      const macList: string[] = [];
      filters.forEach((f) => {
        if (f.full) macList.push(f.id);
        else ouiList.push(f.id);
      });
      setOuis(ouiList.join("\n"));
      setMacs(macList.join("\n"));
      setFiltersLoaded(true);
    }
  }, [filters, filtersLoaded]);

  const saveFilters = async () => {
    try {
      const result = await postForm<{ saved: number }>(
        "/api/detector/filters",
        { ouis, macs },
      );
      toast(`Saved ${result.saved} filters`, "success");
    } catch {
      toast("Error saving filters", "error");
    }
  };

  const devices = devicesResp?.devices ?? [];

  return (
    <div>
      <Card title="FILTERS">
        <TextArea
          label="OUI Prefixes (one per line)"
          value={ouis}
          onInput={setOuis}
          placeholder="AA:BB:CC"
        />
        <TextArea
          label="Full MACs (one per line)"
          value={macs}
          onInput={setMacs}
          placeholder="AA:BB:CC:DD:EE:FF"
        />
        <Button onClick={saveFilters}>SAVE FILTERS</Button>
      </Card>

      <Card title="DETECTED DEVICES">
        {devices.length === 0 ? (
          <EmptyState message="No detections yet" />
        ) : (
          devices.map((d) => (
            <DeviceCard
              key={d.mac}
              mac={d.mac}
              subtitle={d.alias ? `(${d.alias})` : undefined}
            >
              <Tag>{d.rssi} dBm</Tag>
              <Tag>{d.filter}</Tag>
            </DeviceCard>
          ))
        )}
      </Card>
    </div>
  );
}
