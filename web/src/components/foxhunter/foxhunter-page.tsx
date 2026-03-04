import { useState, useEffect } from "preact/hooks";
import { usePoll } from "../../hooks/use-poll";
import { postForm, postEmpty } from "../../api/client";
import type { FoxhunterStatus } from "../../api/client";
import { StatCard } from "../shared/stat-card";
import { Card } from "../shared/card";
import { Button } from "../shared/button";
import { TextInput } from "../shared/text-input";
import { ProgressBar } from "../shared/progress-bar";
import { useToast } from "../shared/toast";

export function FoxhunterPage() {
  const { toast } = useToast();
  const { data, refresh } = usePoll<FoxhunterStatus>(
    "/api/foxhunter/status",
    500,
  );

  const [mac, setMac] = useState("");
  const [macLoaded, setMacLoaded] = useState(false);

  useEffect(() => {
    if (data?.target && !macLoaded) {
      setMac(data.target);
      setMacLoaded(true);
    }
  }, [data, macLoaded]);

  const setTarget = async () => {
    const trimmed = mac.trim();
    if (!trimmed) {
      toast("Enter MAC address", "error");
      return;
    }
    try {
      await postForm("/api/foxhunter/target", { mac: trimmed });
      refresh();
    } catch {
      toast("Invalid MAC format", "error");
    }
  };

  const clearTarget = async () => {
    await postEmpty("/api/foxhunter/clear");
    setMac("");
    setMacLoaded(false);
    refresh();
  };

  const rssiPercent = data?.detected
    ? Math.min(100, Math.max(0, ((data.rssi + 100) / 70) * 100))
    : 0;

  return (
    <div>
      <div class="mb-1.5 flex gap-1.5">
        <StatCard
          value={data?.detected ? data.rssi : "-"}
          label="RSSI dBm"
        />
        <StatCard
          value={data?.detected ? "TRACKING" : "SEARCHING"}
          label="STATUS"
          color={data?.detected ? "#00ff41" : "#00aa00"}
        />
      </div>

      <Card title="TARGET MAC">
        <TextInput
          value={mac}
          onInput={setMac}
          placeholder="AA:BB:CC:DD:EE:FF"
          maxLength={17}
        />
        <div class="flex gap-1.5">
          <Button small onClick={setTarget}>
            SET TARGET
          </Button>
          <Button small variant="danger" onClick={clearTarget}>
            CLEAR
          </Button>
        </div>
      </Card>

      <Card title="RSSI METER">
        <ProgressBar percent={rssiPercent} />
      </Card>
    </div>
  );
}
