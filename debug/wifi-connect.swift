#!/usr/bin/env swift
// Attempt to connect to the oui-spy AP and verify connectivity
// Usage: swift debug/wifi-connect.swift [ssid] [password]

import CoreWLAN
import Foundation

let ssid = CommandLine.arguments.count > 1 ? CommandLine.arguments[1] : "oui-spy"
let pass = CommandLine.arguments.count > 2 ? CommandLine.arguments[2] : "ouispy123"

let iface = CWWiFiClient.shared().interface()!

do {
    print("Scanning for '\(ssid)'...")
    let nets = try iface.scanForNetworks(withSSID: ssid.data(using: .utf8))

    guard let net = nets.first else {
        print("ERROR: '\(ssid)' not found in scan")
        exit(1)
    }

    let ch = net.wlanChannel?.channelNumber ?? 0
    print("Found: \(net.ssid ?? "?") ch=\(ch) rssi=\(net.rssiValue)")
    print("Connecting with password '\(pass)'...")

    try iface.associate(to: net, password: pass)
    print("SUCCESS - connected to \(ssid)!")

    print("Connected successfully.")
} catch {
    print("Connection FAILED: \(error)")
    exit(1)
}
