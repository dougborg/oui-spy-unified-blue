#!/usr/bin/env swift
// Quick check: is oui-spy AP visible?
// Usage: swift debug/wifi-check.swift [ssid]

import CoreWLAN
import Foundation

let ssid = CommandLine.arguments.count > 1 ? CommandLine.arguments[1] : "oui-spy"
let iface = CWWiFiClient.shared().interface()!

do {
    let nets = try iface.scanForNetworks(withSSID: ssid.data(using: .utf8))
    if let net = nets.first {
        let ch = net.wlanChannel?.channelNumber ?? 0
        print("FOUND: \(net.ssid ?? "?") ch=\(ch) rssi=\(net.rssiValue)")
    } else {
        print("NOT FOUND: '\(ssid)'")
        exit(1)
    }
} catch {
    print("Scan error: \(error)")
    exit(1)
}
