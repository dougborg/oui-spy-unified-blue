import type { SkyspyDrone, SkyspyStatus } from "../../api/client";
import { usePoll } from "../../hooks/use-poll";
import { DeviceCard, Tag } from "../shared/device-card";
import { EmptyState } from "../shared/empty-state";
import { StatCard } from "../shared/stat-card";

export function SkyspyPage() {
  const { data: drones } = usePoll<SkyspyDrone[]>("/api/skyspy/drones", 2000);
  const { data: status } = usePoll<SkyspyStatus>("/api/skyspy/status", 2000);

  return (
    <div>
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
