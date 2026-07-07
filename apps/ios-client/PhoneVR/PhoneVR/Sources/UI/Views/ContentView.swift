import SwiftUI

struct ContentView: View {
    @EnvironmentObject var appState: VRAppState
    @EnvironmentObject var streamingService: StreamingService

    var body: some View {
        ZStack {
            VRDisplayView()
                .edgesIgnoringSafeArea(.all)

            VStack {
                HStack {
                    ConnectionStatusView()
                        .padding(8)
                    Spacer()
                    if appState.showDebugOverlay {
                        StatsOverlayView()
                    }
                }
                .padding(.top, 8)

                Spacer()

                if appState.connectionState == .disconnected {
                    ConnectButtonView()
                }
            }

            if appState.showSettings {
                SettingsView()
                    .transition(.move(edge: .bottom))
                    .zIndex(100)
            }
        }
        .onTapGesture(count: 3) {
            withAnimation {
                appState.showSettings.toggle()
            }
        }
        .onTapGesture(count: 2) {
            withAnimation {
                appState.showDebugOverlay.toggle()
            }
        }
    }
}

struct VRDisplayView: UIViewRepresentable {
    @EnvironmentObject var appState: VRAppState

    func makeUIView(context: Context) -> MTKView {
        let mtkView = MTKView()
        mtkView.device = MTLCreateSystemDefaultDevice()
        mtkView.framebufferOnly = true
        mtkView.clearColor = MTLClearColor(red: 0, green: 0, blue: 0, alpha: 1)
        mtkView.drawableSize = UIScreen.main.nativeBounds.size
        mtkView.isPaused = false
        mtkView.enableSetNeedsDisplay = false
        mtkView.preferredFramesPerSecond = Int(appState.settings.targetFPS)

        let renderer = VRRenderer()
        renderer.ipd = appState.settings.ipd / 1000.0
        renderer.renderScale = appState.settings.renderScale
        mtkView.delegate = renderer

        objc_setAssociatedObject(mtkView, "renderer", renderer, .OBJC_ASSOCIATION_RETAIN)

        return mtkView
    }

    func updateUIView(_ uiView: MTKView, context: Context) {}
}

struct ConnectionStatusView: View {
    @EnvironmentObject var appState: VRAppState

    var body: some View {
        HStack(spacing: 6) {
            Circle()
                .fill(statusColor)
                .frame(width: 8, height: 8)
            Text(statusText)
                .font(.caption)
                .foregroundColor(.white)
                .shadow(radius: 1)
        }
        .padding(.horizontal, 10)
        .padding(.vertical, 5)
        .background(.ultraThinMaterial)
        .cornerRadius(8)
    }

    private var statusColor: Color {
        switch appState.connectionState {
        case .connected: return .green
        case .connecting: return .yellow
        case .error: return .red
        case .disconnected: return .gray
        }
    }

    private var statusText: String {
        switch appState.connectionState {
        case .connected: return "Connected"
        case .connecting: return "Connecting..."
        case .error(let msg): return "Error: \(msg)"
        case .disconnected: return "Disconnected"
        }
    }
}

struct StatsOverlayView: View {
    @EnvironmentObject var appState: VRAppState
    @EnvironmentObject var streamingService: StreamingService

    var body: some View {
        VStack(alignment: .leading, spacing: 2) {
            StatRow(label: "FPS", value: String(format: "%.1f", streamingService.currentFPS))
            StatRow(label: "Latency", value: String(format: "%.1f ms", streamingService.currentLatency))
            StatRow(label: "Bitrate", value: String(format: "%.1f Mbps", Double(appState.stats.bitrateMbps)))
            StatRow(label: "Battery", value: "\(appState.batteryLevel)%")
        }
        .font(.system(size: 10, design: .monospaced))
        .foregroundColor(.green)
        .padding(6)
        .background(Color.black.opacity(0.6))
        .cornerRadius(6)
    }
}

struct StatRow: View {
    let label: String
    let value: String

    var body: some View {
        HStack {
            Text(label)
                .frame(width: 50, alignment: .leading)
            Text(value)
        }
    }
}

struct ConnectButtonView: View {
    @EnvironmentObject var appState: VRAppState
    @EnvironmentObject var streamingService: StreamingService

    var body: some View {
        VStack(spacing: 16) {
            Image(systemName: "antenna.radiowaves.left.and.right")
                .font(.system(size: 48))
                .foregroundColor(.blue)

            Text("PhoneVR")
                .font(.largeTitle)
                .fontWeight(.bold)
                .foregroundColor(.white)

            Button(action: connect) {
                Label("Connect to PC", systemImage: "link")
                    .font(.headline)
                    .foregroundColor(.white)
                    .padding(.horizontal, 32)
                    .padding(.vertical, 14)
                    .background(Color.blue)
                    .cornerRadius(12)
            }
        }
        .padding(40)
        .background(.ultraThinMaterial)
        .cornerRadius(20)
    }

    private func connect() {
        streamingService.connect(
            host: appState.settings.hostAddress,
            port: appState.settings.hostPort,
            settings: appState.settings
        )
        appState.connectionState = .connecting(appState.settings.hostAddress)
    }
}
