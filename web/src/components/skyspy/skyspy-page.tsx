import { postEmpty } from "../../api/client";
import type { SkyspyDrone, SkyspyStatus } from "../../api/client";
import { usePoll } from "../../hooks/use-poll";
import { Button } from "../shared/button";
import { Card } from "../shared/card";
import { DeviceCard, Tag } from "../shared/device-card";
import { EmptyState } from "../shared/empty-state";
import { StatCard } from "../shared/stat-card";

export function SkyspyPage() {
  const { data: drones } = usePoll<SkyspyDrone[]>("/api/skyspy/drones", 2000);
  const { data: status } = usePoll<SkyspyStatus>("/api/skyspy/status", 2000);

  const startScan = async () => {
    if (
      !confirm(
        "Reboot into WiFi scan mode?\n\nThe device will restart in dedicated scan mode (no web dashboard). Hold BOOT button 1.5s to return to normal mode.",
      )
    )
      return;
    await postEmpty("/api/skyspy/start-scan");
  };

  return (
    <div>
      <Card title="WIFI SCAN MODE">
        <p class="mb-1.5 text-xs text-text-dim">
          Reboot into dedicated WiFi scan mode for full Remote ID detection (NAN action frames on
          channel 6). BLE detection is always active in normal mode. Hold BOOT button 1.5s to
          return.
        </p>
        <Button onClick={startScan}>START WIFI SCAN</Button>
      </Card>

      <div class="mb-1.5 flex gap-1.5">
        <StatCard value={drones?.length ?? 0} label="DRONES" />
        <StatCard
          value={status?.in_range ? "IN RANGE" : "CLEAR"}
          label="STATUS"
          color={status?.in_range ? "#00ff41" : "#00aa00"}
        />
      </div>

      {!drones || drones.length === 0 ? (
        <EmptyState message="Scanning for drone Remote ID..." />
      ) : (
        drones.map((d) => (
          <DeviceCard key={d.mac} mac={d.mac}>
            <Tag>RSSI:{d.rssi}</Tag>
            {d.uav_id && <Tag>ID:{d.uav_id}</Tag>}
            {d.drone_lat !== 0 && (
              <Tag>
                {d.drone_lat.toFixed(4)},{d.drone_long.toFixed(4)}
              </Tag>
            )}
            {d.altitude !== 0 && <Tag>Alt:{d.altitude}m</Tag>}
            {d.speed !== 0 && <Tag>{d.speed}km/h</Tag>}
          </DeviceCard>
        ))
      )}
    </div>
  );
}
