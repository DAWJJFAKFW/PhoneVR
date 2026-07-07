import SwiftUI

@main
struct PhoneVRApp: App {
    @StateObject private var appState = VRAppState()
    @StateObject private var streamingService = StreamingService()

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(appState)
                .environmentObject(streamingService)
                .preferredColorScheme(appState.colorScheme)
                .persistentSystemOverlays(.hidden)
                .statusBarHidden(true)
                .onAppear {
                    UIApplication.shared.isIdleTimerDisabled = true
                }
        }
    }
}
