#!/usr/bin/env swift
// Scan for nearby WiFi networks and display channel usage
// Usage: swift debug/wifi-scan.swift [ssid-filter]

import CoreWLAN
import Foundation

let filter = CommandLine.arguments.count > 1 ? CommandLine.arguments[1].lowercased() : nil
let iface = CWWiFiClient.shared().interface()!

do {
    let nets = try iface.scanForNetworks(withSSID: nil)
    var channels: [Int: Int] = [:]

    let sorted = nets.sorted { ($0.ssid ?? "") < ($1.ssid ?? "") }
    for n in sorted {
        let ch = n.wlanChannel?.channelNumber ?? 0
        channels[ch, default: 0] += 1

        if let f = filter {
            guard n.ssid?.lowercased().contains(f) == true else { continue }
        }
        print(String(format: "%-30s ch=%-3d rssi=%d", (n.ssid ?? "?") as NSString, ch, n.rssiValue))
    }

    print("\n--- Channel usage ---")
    for ch in channels.keys.sorted() {
        print("  Channel \(ch): \(channels[ch]!) APs")
    }
} catch {
    print("Scan error: \(error)")
}
