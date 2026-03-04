// Lightweight typed fetch wrapper for the OUI SPY API
// Types are inferred from the actual API responses since the device is the source of truth

export interface SystemStatus {
  uptime: number;
  heap: number;
  psram: number;
  buzzer: boolean;
}

export interface Module {
  name: string;
  enabled: boolean;
}

export interface GPSData {
  valid: boolean;
  lat: number;
  lon: number;
  acc: number;
  hardware: boolean;
  hw_detected: boolean;
  hw_fix: boolean;
  sats: number;
  fresh: boolean;
}

export interface APConfig {
  ssid: string;
}

export interface DetectorFilter {
  id: string;
  full: boolean;
  desc: string;
}

export interface DetectorDevice {
  mac: string;
  rssi: number;
  filter: string;
  alias: string;
  timeSince: number;
}

export interface FoxhunterStatus {
  target: string;
  detected: boolean;
  rssi: number;
  lastSeen: number;
}

export interface FoxhunterRSSI {
  rssi: number;
  detected: boolean;
}

export interface FlockyouDetection {
  mac: string;
  name: string;
  rssi: number;
  method: string;
  count: number;
  raven: boolean;
  fw: string;
  gps: boolean;
  last: number;
}

export interface FlockyouStats {
  total: number;
  raven: number;
  ble: string;
  gps_valid: boolean;
  gps_tagged: number;
  gps_src: string;
  gps_sats: number;
  gps_hw_detected: boolean;
}

export interface SkyspyDrone {
  mac: string;
  rssi: number;
  drone_lat: number;
  drone_long: number;
  altitude: number;
  height: number;
  speed: number;
  heading: number;
  pilot_lat: number;
  pilot_long: number;
  uav_id: string;
  op_id: string;
  last_seen: number;
}

export interface SkyspyStatus {
  active_drones: number;
  in_range: boolean;
}

async function api<T>(url: string, init?: RequestInit): Promise<T> {
  const res = await fetch(url, init);
  if (!res.ok) throw new Error(`API ${res.status}: ${url}`);
  return res.json();
}

export async function fetchJSON<T>(url: string): Promise<T> {
  return api<T>(url);
}

export async function postForm<T>(
  url: string,
  params: Record<string, string>,
): Promise<T> {
  return api<T>(url, {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: new URLSearchParams(params).toString(),
  });
}

export async function postEmpty<T>(url: string): Promise<T> {
  return api<T>(url, { method: "POST" });
}
