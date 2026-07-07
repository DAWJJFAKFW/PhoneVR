import SwiftUI

struct SettingsView: View {
    @EnvironmentObject var appState: VRAppState
    @Environment(\.dismiss) var dismiss

    var body: some View {
        NavigationView {
            Form {
                connectionSection
                videoSection
                trackingSection
                calibrationSection
                aboutSection
            }
            .navigationTitle("Settings")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .confirmationAction) {
                    Button("Done") {
                        appState.settings.save()
                        withAnimation {
                            appState.showSettings = false
                        }
                    }
                }
            }
        }
    }

    private var connectionSection: some View {
        Section("Connection") {
            HStack {
                Label("Host", systemImage: "network")
                TextField("IP Address", text: $appState.settings.hostAddress)
                    .keyboardType(.numbersAndPunctuation)
                    .multilineTextAlignment(.trailing)
            }

            HStack {
                Label("Port", systemImage: "number")
                TextField("Port", value: $appState.settings.hostPort, format: .number)
                    .keyboardType(.numberPad)
                    .multilineTextAlignment(.trailing)
            }

            Toggle(isOn: $appState.settings.useWiFi) {
                Label("Wi-Fi Mode", systemImage: "wifi")
            }
        }
    }

    private var videoSection: some View {
        Section("Video") {
            Picker(selection: $appState.settings.preferredCodec) {
                Text("H.264").tag(VRVideoCodec.h264)
                Text("H.265 (HEVC)").tag(VRVideoCodec.h265)
            } label: {
                Label("Codec", systemImage: "video")
            }

            HStack {
                Label("Bitrate", systemImage: "gauge.medium")
                Slider(value: Binding(
                    get: { Double(appState.settings.targetBitrate) },
                    set: { appState.settings.targetBitrate = UInt32($0) }
                ), in: 10_000_000...200_000_000, step: 5_000_000)
                Text("\(appState.settings.targetBitrate / 1_000_000) Mbps")
                    .frame(width: 60)
                    .font(.caption)
            }

            HStack {
                Label("FPS", systemImage: "speedometer")
                Picker("", selection: $appState.settings.targetFPS) {
                    Text("60 FPS").tag(UInt32(60))
                    Text("90 FPS").tag(UInt32(90))
                    Text("120 FPS").tag(UInt32(120))
                }
                .pickerStyle(.segmented)
            }

            HStack {
                Label("Render Scale", systemImage: "rectangle.expand.vertical")
                Slider(value: $appState.settings.renderScale, in: 0.5...2.0, step: 0.25)
                Text(String(format: "%.2f", appState.settings.renderScale))
                    .frame(width: 40)
                    .font(.caption)
            }

            Toggle(isOn: $appState.settings.lowPersistenceMode) {
                Label("Low Persistence", systemImage: "eye")
            }

            Toggle(isOn: $appState.settings.enableFoveated) {
                Label("Foveated Rendering", systemImage: "eye.fill")
            }
        }
    }

    private var trackingSection: some View {
        Section("Tracking") {
            Toggle(isOn: $appState.settings.enableHandTracking) {
                Label("Hand Tracking", systemImage: "hand.raised")
            }

            HStack {
                Label("Sensitivity", systemImage: "dial.medium")
                Slider(value: $appState.settings.sensorSensitivity, in: 0.5...2.0, step: 0.1)
                Text(String(format: "%.1f", appState.settings.sensorSensitivity))
                    .frame(width: 30)
                    .font(.caption)
            }
        }
    }

    private var calibrationSection: some View {
        Section("Calibration") {
            HStack {
                Label("IPD (mm)", systemImage: "eye")
                Slider(value: $appState.settings.ipd, in: 50...80, step: 0.5)
                Text(String(format: "%.1f", appState.settings.ipd))
                    .frame(width: 40)
                    .font(.caption)
            }

            Button(action: calibrate) {
                Label("Calibrate Sensors", systemImage: "arrow.triangle.2.circlepath")
            }

            Button(action: recenter) {
                Label("Recenter View", systemImage: "target")
            }
        }
    }

    private var aboutSection: some View {
        Section("About") {
            HStack {
                Text("Version")
                Spacer()
                Text("1.0.0")
                    .foregroundColor(.secondary)
            }
        }
    }

    private func calibrate() {
        appState.settings.calibratedIPD = appState.settings.ipd
    }

    private func recenter() {
        // Recenter handled by streaming service
    }
}
