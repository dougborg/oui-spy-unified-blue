import { useEffect, useState } from "preact/hooks";
import { postEmpty, postForm } from "../../api/client";
import type { APConfig, GPSData, Module, SystemStatus } from "../../api/client";
import { usePoll } from "../../hooks/use-poll";
import { Button } from "../shared/button";
import { Card } from "../shared/card";
import { StatCard } from "../shared/stat-card";
import { TextInput } from "../shared/text-input";
import { useToast } from "../shared/toast";
import { Toggle } from "../shared/toggle";

export function SystemPage() {
  const { toast } = useToast();
  const { data: status } = usePoll<SystemStatus>("/api/status", 5000);
  const { data: modules, refresh: refreshMods } = usePoll<Module[]>("/api/modules", 5000);
  const { data: gps } = usePoll<GPSData>("/api/gps", 5000);
  const { data: ap } = usePoll<APConfig>("/api/ap", 30000);

  const [ssid, setSsid] = useState("");
  const [pass, setPass] = useState("");

  // Sync AP ssid when loaded
  useEffect(() => {
    if (ap && ssid === "" && ap.ssid) {
      setSsid(ap.ssid);
    }
  }, [ap, ssid]);

  const formatUptime = (s: number) => {
    const m = Math.floor(s / 60);
    const h = Math.floor(m / 60);
    return h > 0 ? `${h}h${String(m % 60).padStart(2, "0")}m` : `${m}m`;
  };

  const toggleModule = async (name: string, enabled: boolean) => {
    await postForm("/api/modules/toggle", {
      name,
      enabled: String(enabled),
    });
    refreshMods();
  };

  const toggleBuzzer = async () => {
    if (!status) return;
    await postForm("/api/buzzer", { on: String(!status.buzzer) });
  };

  const saveAP = async () => {
    if (!ssid) {
      toast("SSID required", "error");
      return;
    }
    try {
      await postForm("/api/ap", {
        ssid: ssid,
        pass: pass,
      });
      toast("Saved! Rebooting...", "success");
    } catch {
      toast("Failed to save AP settings", "error");
    }
  };

  const restart = async () => {
    if (!confirm("Restart device?")) return;
    await postEmpty("/api/reset");
  };

  const gpsText = (() => {
    if (!gps) return "Loading...";
    if (gps.hw_fix) return `HW GPS: ${gps.sats} sats, ${gps.lat.toFixed(5)},${gps.lon.toFixed(5)}`;
    if (gps.fresh) return `Phone GPS: ${gps.lat.toFixed(5)},${gps.lon.toFixed(5)}`;
    if (gps.hw_detected) return `HW GPS: ${gps.sats} sats (no fix)`;
    return "No GPS";
  })();

  return (
    <div>
      <div class="mb-1.5 flex gap-1.5">
        <StatCard value={status ? Math.round(status.heap / 1024) : "-"} label="HEAP KB" />
        <StatCard value={status ? Math.round(status.psram / 1024) : "-"} label="PSRAM KB" />
        <StatCard value={status ? formatUptime(status.uptime) : "-"} label="UPTIME" />
      </div>

      <Card title="MODULES">
        {modules?.map((m) => (
          <Toggle
            key={m.name}
            label={m.name.toUpperCase()}
            enabled={m.enabled}
            onToggle={() => toggleModule(m.name, !m.enabled)}
          />
        ))}
      </Card>

      <Card title="BUZZER">
        <Toggle label="Buzzer Audio" enabled={status?.buzzer ?? false} onToggle={toggleBuzzer} />
      </Card>

      <Card title="GPS">
        <div class="text-xs text-text-bright">{gpsText}</div>
      </Card>

      <Card title="AP SETTINGS">
        <TextInput label="SSID" value={ssid} onInput={setSsid} maxLength={32} />
        <TextInput label="Password" value={pass} onInput={setPass} maxLength={63} />
        <Button onClick={saveAP}>SAVE & REBOOT</Button>
      </Card>

      <Button variant="danger" onClick={restart}>
        RESTART DEVICE
      </Button>
    </div>
  );
}
