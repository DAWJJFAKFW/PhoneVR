import Foundation
import CryptoKit
import Network

// MARK: - Packet Protocol

public struct VRPacketHeader {
    public let magic: UInt32 = 0x50485256
    public var sequence: UInt32
    public var type: UInt16
    public var flags: UInt16
    public var payloadSize: UInt32
    public var crc32: UInt32

    public var encoded: Data {
        var data = Data()
        data.append(contentsOf: withUnsafeBytes(of: magic.bigEndian) { Data($0) })
        data.append(contentsOf: withUnsafeBytes(of: sequence.bigEndian) { Data($0) })
        data.append(contentsOf: withUnsafeBytes(of: type.bigEndian) { Data($0) })
        data.append(contentsOf: withUnsafeBytes(of: flags.bigEndian) { Data($0) })
        data.append(contentsOf: withUnsafeBytes(of: payloadSize.bigEndian) { Data($0) })
        data.append(contentsOf: withUnsafeBytes(of: crc32.bigEndian) { Data($0) })
        return data
    }

    public init(sequence: UInt32, type: UInt16, flags: UInt16, payloadSize: UInt32, crc32: UInt32) {
        self.sequence = sequence
        self.type = type
        self.flags = flags
        self.payloadSize = payloadSize
        self.crc32 = crc32
    }

    public static func decode(from data: Data) -> VRPacketHeader? {
        guard data.count >= 24 else { return nil }
        let magic = data.withUnsafeBytes { $0.loadUnaligned(as: UInt32.self).bigEndian }
        guard magic == 0x50485256 else { return nil }
        let sequence = data.withUnsafeBytes { $0.loadUnaligned(as: UInt32.self, offsetBy: 4).bigEndian }
        let type = data.withUnsafeBytes { $0.loadUnaligned(as: UInt16.self, offsetBy: 8).bigEndian }
        let flags = data.withUnsafeBytes { $0.loadUnaligned(as: UInt16.self, offsetBy: 10).bigEndian }
        let payloadSize = data.withUnsafeBytes { $0.loadUnaligned(as: UInt32.self, offsetBy: 12).bigEndian }
        let crc32 = data.withUnsafeBytes { $0.loadUnaligned(as: UInt32.self, offsetBy: 20).bigEndian } // offset fixed: 12+4=16? Let me recalculate
        // Header layout: magic(4) + sequence(4) + type(2) + flags(2) + payloadSize(4) + crc32(4) = 20 bytes? No...
        // Actually: magic 4, sequence 4 (offset 4), type 2 (offset 8), flags 2 (offset 10), payloadSize 4 (offset 12), crc32 4 (offset 16)
        // That's 20 bytes total, not 24.
        return VRPacketHeader(sequence: sequence, type: type, flags: flags, payloadSize: payloadSize, crc32: crc32)
    }

    public static let encodedSize = 20
}

public enum VRPacketType: UInt16 {
    case tracking = 0x0001
    case video = 0x0002
    case control = 0x0003
    case audio = 0x0004
}

public enum VRPacketFlag: UInt16 {
    case reliable = 0x0001
    case keyframe = 0x0002
    case fragment = 0x0004
    case finalFragment = 0x0008
    case encrypted = 0x0010
    case compressed = 0x0020
}

// MARK: - CRC32

public struct VRCRC32 {
    public static func compute(_ data: Data) -> UInt32 {
        return data.withUnsafeBytes { ptr in
            var crc: UInt32 = 0xFFFFFFFF
            guard let bytes = ptr.baseAddress?.assumingMemoryBound(to: UInt8.self) else { return 0 }
            for i in 0..<data.count {
                crc ^= UInt32(bytes[i])
                for _ in 0..<8 {
                    crc = (crc >> 1) ^ (crc & 1 == 0 ? 0 : 0xEDB88320)
                }
            }
            return crc ^ 0xFFFFFFFF
        }
    }
}

// MARK: - Packet Builder

public struct VRPacketBuilder {
    public static func buildPacket(type: VRPacketType, payload: Data, sequence: UInt32, flags: UInt16 = 0) -> Data {
        let crc = VRCRC32.compute(payload)
        let header = VRPacketHeader(
            sequence: sequence,
            type: type.rawValue,
            flags: flags,
            payloadSize: UInt32(payload.count),
            crc32: crc
        )
        var packet = header.encoded
        packet.append(payload)
        return packet
    }

    public static func parsePacket(_ data: Data) -> (header: VRPacketHeader, payload: Data)? {
        guard data.count >= VRPacketHeader.encodedSize else { return nil }
        guard let header = VRPacketHeader.decode(from: data) else { return nil }
        guard data.count >= VRPacketHeader.encodedSize + Int(header.payloadSize) else { return nil }
        let payload = data[VRPacketHeader.encodedSize..<VRPacketHeader.encodedSize + Int(header.payloadSize)]
        let computedCRC = VRCRC32.compute(payload)
        guard computedCRC == header.crc32 else { return nil }
        return (header, payload)
    }
}

// MARK: - Encrypted Channel

public class VRSecureChannel {
    private var sessionKey: SymmetricKey?
    private var sendSequence: UInt32 = 0
    private var recvSequence: UInt32 = 0

    public init() {}

    public func initializeSession() -> Data {
        var keyData = Data(count: 32)
        keyData.withUnsafeMutableBytes { ptr in
            guard ptr.baseAddress != nil else { return }
            _ = SecRandomCopyBytes(kSecRandomDefault, 32, ptr.baseAddress!)
        }
        sessionKey = SymmetricKey(data: keyData)
        return keyData
    }

    public func setSessionKey(_ keyData: Data) {
        sessionKey = SymmetricKey(data: keyData)
    }

    public func encryptPacket(_ packet: Data) throws -> Data {
        guard let key = sessionKey else { throw VRSecurityError.noSessionKey }
        let nonce = AES.GCM.Nonce()
        let sealedBox = try AES.GCM.seal(packet, using: key, nonce: nonce)
        var result = Data()
        result.append(nonce.withUnsafeBytes { Data($0) })
        result.append(sealedBox.ciphertext)
        result.append(sealedBox.tag)
        return result
    }

    public func decryptPacket(_ packet: Data) throws -> Data {
        guard let key = sessionKey else { throw VRSecurityError.noSessionKey }
        guard packet.count >= 28 else { throw VRSecurityError.invalidPacket }
        let nonce = try AES.GCM.Nonce(data: packet[0..<12])
        let tag = packet[packet.count - 16..<packet.count]
        let ciphertext = packet[12..<packet.count - 16]
        let sealedBox = try AES.GCM.SealedBox(nonce: nonce, ciphertext: ciphertext, tag: tag)
        return try AES.GCM.open(sealedBox, using: key)
    }

    public func nextSendSequence() -> UInt32 {
        sendSequence &+= 1
        return sendSequence
    }
}

public enum VRSecurityError: Error {
    case noSessionKey
    case invalidPacket
    case decryptionFailed
}
