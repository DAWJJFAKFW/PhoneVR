import Foundation
import Network
import Combine

public class NetworkService {
    private var connection: NWConnection?
    private var listener: NWListener?
    private let queue = DispatchQueue(label: "com.phonevr.network", qos: .userInteractive)
    private var receiveBuffer = Data()
    private var sendSequence: UInt32 = 0
    private var heartbeatTimer: DispatchSourceTimer?
    private let secureChannel = VRSecureChannel()

    public var onTrackingFrame: ((VRTrackingFrame) -> Void)?
    public var onControlMessage: ((VRControlMessage) -> Void)?
    public var onStatsUpdate: ((VRStatsReport) -> Void)?
    public var onConnectionStateChange: ((ConnectionStatus) -> Void)?
    public var onVideoPacket: ((VRVideoPacket) -> Void)?

    public private(set) var isConnected = false
    public private(set) var roundTripTimeUs: UInt64 = 0

    public enum ConnectionStatus {
        case disconnected
        case connecting
        case connected
        case failed(Error)
    }

    public init() {}

    public func connect(host: String, port: UInt16, useTCP: Bool = true) {
        disconnect()

        let tcpOptions = NWProtocolTCP.Options()
        tcpOptions.connectionTimeout = 5
        tcpOptions.enableKeepalive = true
        tcpOptions.keepaliveIdle = 2
        tcpOptions.keepaliveInterval = 2
        tcpOptions.keepaliveCount = 3

        let params = NWParameters(tls: nil, tcp: tcpOptions)

        if !useTCP {
            let udpOptions = NWProtocolUDP.Options()
            params.defaultProtocolStack.applicationProtocols.insert(NWProtocolUDP.definition, at: 0)
        }

        let endpoint = NWEndpoint.hostPort(host: NWEndpoint.Host(host), port: NWEndpoint.Port(rawValue: port)!)
        connection = NWConnection(to: endpoint, using: params)

        connection?.stateUpdateHandler = { [weak self] state in
            DispatchQueue.main.async {
                switch state {
                case .ready:
                    self?.isConnected = true
                    self?.onConnectionStateChange?(.connected)
                    self?.startHeartbeat()
                    self?.receiveLoop()
                case .failed(let error):
                    self?.isConnected = false
                    self?.onConnectionStateChange?(.failed(error))
                case .cancelled:
                    self?.isConnected = false
                    self?.onConnectionStateChange?(.disconnected)
                default:
                    break
                }
            }
        }

        connection?.start(queue: queue)
        onConnectionStateChange?(.connecting)
    }

    public func disconnect() {
        stopHeartbeat()
        connection?.cancel()
        connection = nil
        isConnected = false
        sendSequence = 0
        receiveBuffer.removeAll()
        DispatchQueue.main.async { [weak self] in
            self?.onConnectionStateChange?(.disconnected)
        }
    }

    public func sendTrackingFrame(_ frame: VRTrackingFrame) {
        guard isConnected else { return }
        do {
            let data = try JSONEncoder().encode(frame)
            let packet = VRPacketBuilder.buildPacket(
                type: .tracking,
                payload: data,
                sequence: secureChannel.nextSendSequence()
            )
            sendData(packet, type: .tracking)
        } catch {
            print("Failed to encode tracking frame: \(error)")
        }
    }

    public func sendVideoPacket(_ packet: VRVideoPacket) {
        guard isConnected else { return }
        do {
            let data = try JSONEncoder().encode(packet)
            let flags = packet.isKeyframe ? VRPacketFlag.keyframe.rawValue : UInt16(0)
            let outPacket = VRPacketBuilder.buildPacket(
                type: .video,
                payload: data,
                sequence: secureChannel.nextSendSequence(),
                flags: flags
            )
            sendData(outPacket, type: .video)
        } catch {
            print("Failed to encode video packet: \(error)")
        }
    }

    public func sendControlMessage(_ message: VRControlMessage) {
        guard isConnected else { return }
        do {
            let data = try JSONEncoder().encode(message)
            let packet = VRPacketBuilder.buildPacket(
                type: .control,
                payload: data,
                sequence: secureChannel.nextSendSequence(),
                flags: VRPacketFlag.reliable.rawValue
            )
            sendData(packet, type: .control)
        } catch {
            print("Failed to encode control message: \(error)")
        }
    }

    private func sendData(_ data: Data, type: VRPacketType) {
        connection?.send(content: data, completion: .contentProcessed { [weak self] error in
            if let error = error {
                print("Send error (\(type)): \(error)")
                DispatchQueue.main.async {
                    self?.onConnectionStateChange?(.failed(error))
                }
            }
        })
    }

    private func receiveLoop() {
        connection?.receive(minimumIncompleteLength: 1, maximumLength: 65536) { [weak self] content, _, isComplete, error in
            guard let self = self else { return }

            if let content = content {
                self.receiveBuffer.append(content)
                self.processReceiveBuffer()
            }

            if isComplete {
                self.disconnect()
                return
            }

            if let error = error {
                DispatchQueue.main.async {
                    self.onConnectionStateChange?(.failed(error))
                }
                return
            }

            self.receiveLoop()
        }
    }

    private func processReceiveBuffer() {
        while receiveBuffer.count >= VRPacketHeader.encodedSize {
            guard let (header, payload) = VRPacketBuilder.parsePacket(receiveBuffer) else {
                return
            }

            let packetData = receiveBuffer[..<(VRPacketHeader.encodedSize + Int(header.payloadSize))]
            receiveBuffer.removeSubrange(..<packetData.count)

            processPacket(header: header, payload: payload)
        }
    }

    private func processPacket(header: VRPacketHeader, payload: Data) {
        let type = VRPacketType(rawValue: header.type)

        switch type {
        case .control:
            if let message = try? JSONDecoder().decode(VRControlMessage.self, from: payload) {
                DispatchQueue.main.async { [weak self] in
                    self?.onControlMessage?(message)
                }
            }
        case .tracking:
            break
        case .video:
            if let packet = try? JSONDecoder().decode(VRVideoPacket.self, from: payload) {
                DispatchQueue.main.async { [weak self] in
                    self?.onVideoPacket?(packet)
                }
            }
        default:
            break
        }
    }

    private func startHeartbeat() {
        heartbeatTimer = DispatchSource.makeTimerSource(queue: queue)
        heartbeatTimer?.schedule(deadline: .now(), repeating: 1.0)
        heartbeatTimer?.setEventHandler { [weak self] in
            self?.sendHeartbeat()
        }
        heartbeatTimer?.resume()
    }

    private func stopHeartbeat() {
        heartbeatTimer?.cancel()
        heartbeatTimer = nil
    }

    private func sendHeartbeat() {
        let message = VRControlMessage(
            type: .heartbeat,
            payload: Data(),
            timestampUs: UInt64(Date().timeIntervalSince1970 * 1_000_000),
            messageId: sendSequence
        )
        sendControlMessage(message)
    }
}
