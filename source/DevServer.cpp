#include "DevServer.h"
#include <cstring>
#include <cstdint>

//==============================================================================
// SHA-1 (RFC 3174) — required for the WebSocket handshake accept key
// Self-contained, no external dependencies.
//==============================================================================

namespace {

static uint32_t rotl32(uint32_t v, int n) { return (v << n) | (v >> (32 - n)); }

struct SHA1Ctx {
    uint32_t h[5];
    uint32_t lo, hi;
    uint8_t  buf[64];
    int      bufLen;
};

static void sha1Block(SHA1Ctx& ctx, const uint8_t b[64]) {
    uint32_t w[80];
    for (int i = 0; i < 16; ++i)
        w[i] = (uint32_t(b[i*4+0])<<24)|(uint32_t(b[i*4+1])<<16)
              |(uint32_t(b[i*4+2])<<8) | uint32_t(b[i*4+3]);
    for (int i = 16; i < 80; ++i)
        w[i] = rotl32(w[i-3]^w[i-8]^w[i-14]^w[i-16], 1);

    uint32_t a=ctx.h[0],b2=ctx.h[1],c=ctx.h[2],d=ctx.h[3],e=ctx.h[4];
    for (int i = 0; i < 80; ++i) {
        uint32_t f, k;
        if      (i<20){f=(b2&c)|(~b2&d);k=0x5A827999u;}
        else if (i<40){f=b2^c^d;        k=0x6ED9EBA1u;}
        else if (i<60){f=(b2&c)|(b2&d)|(c&d);k=0x8F1BBCDCu;}
        else          {f=b2^c^d;        k=0xCA62C1D6u;}
        uint32_t t=rotl32(a,5)+f+e+k+w[i];
        e=d;d=c;c=rotl32(b2,30);b2=a;a=t;
    }
    ctx.h[0]+=a;ctx.h[1]+=b2;ctx.h[2]+=c;ctx.h[3]+=d;ctx.h[4]+=e;
}

static void sha1(const uint8_t* data, size_t len, uint8_t out[20]) {
    SHA1Ctx ctx;
    ctx.h[0]=0x67452301u;ctx.h[1]=0xEFCDAB89u;ctx.h[2]=0x98BADCFEu;
    ctx.h[3]=0x10325476u;ctx.h[4]=0xC3D2E1F0u;
    ctx.lo=ctx.hi=0;ctx.bufLen=0;

    while (len > 0) {
        size_t space = 64 - (size_t)ctx.bufLen;
        size_t copy  = len < space ? len : space;
        memcpy(ctx.buf + ctx.bufLen, data, copy);
        ctx.bufLen += (int)copy;
        ctx.lo += (uint32_t)(copy << 3);
        if (ctx.lo < (uint32_t)(copy<<3)) ctx.hi++;
        ctx.hi += (uint32_t)(copy >> 29);
        data += copy; len -= copy;
        if (ctx.bufLen == 64) { sha1Block(ctx, ctx.buf); ctx.bufLen = 0; }
    }

    ctx.buf[ctx.bufLen++] = 0x80;
    if (ctx.bufLen > 56) {
        while (ctx.bufLen < 64) ctx.buf[ctx.bufLen++] = 0;
        sha1Block(ctx, ctx.buf);
        ctx.bufLen = 0;
    }
    while (ctx.bufLen < 56) ctx.buf[ctx.bufLen++] = 0;
    for (int i=0;i<4;++i) ctx.buf[56+i]=(ctx.hi>>(24-i*8))&0xFF;
    for (int i=0;i<4;++i) ctx.buf[60+i]=(ctx.lo>>(24-i*8))&0xFF;
    sha1Block(ctx, ctx.buf);

    for (int i=0;i<5;++i)
        for (int j=0;j<4;++j)
            out[i*4+j]=(ctx.h[i]>>(24-j*8))&0xFF;
}

static juce::String base64(const uint8_t* d, size_t len) {
    static const char T[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    juce::String r;
    for (size_t i=0;i<len;i+=3) {
        uint32_t g=(uint32_t(d[i])<<16)
                  |(i+1<len?(uint32_t(d[i+1])<<8):0)
                  |(i+2<len? uint32_t(d[i+2])   :0);
        r+=T[(g>>18)&63]; r+=T[(g>>12)&63];
        r+=(i+1<len)?T[(g>>6)&63]:'=';
        r+=(i+2<len)?T[(g>>0)&63]:'=';
    }
    return r;
}

static juce::String wsAcceptKey(const juce::String& key) {
    juce::String combined = key.trim() + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    auto utf8 = combined.toRawUTF8();
    uint8_t hash[20];
    sha1(reinterpret_cast<const uint8_t*>(utf8), strlen(utf8), hash);
    return base64(hash, 20);
}

} // namespace

//==============================================================================
// DevServer
//==============================================================================

DevServer::DevServer() : juce::Thread("WebPlug DevServer") {}

DevServer::~DevServer() { stop(); }

void DevServer::start(int p) {
    port = p;
    startThread();
}

void DevServer::stop() {
    signalThreadShouldExit();
    serverSocket.close();
    waitForThreadToExit(2000);
}

void DevServer::sendMessage(const juce::String& json) {
    juce::ScopedLock lk(clientLock);
    if (activeClient && activeClient->isConnected())
        sendFrame(*activeClient, json);
}

void DevServer::run() {
    if (!serverSocket.createListener(port, "127.0.0.1")) {
        juce::Logger::writeToLog("[webplug] Failed to bind port " + juce::String(port));
        return;
    }
    juce::Logger::writeToLog("[webplug] Listening on ws://localhost:" + juce::String(port));

    while (!threadShouldExit()) {
        if (serverSocket.waitUntilReady(true, 200) != 1) continue;

        std::unique_ptr<juce::StreamingSocket> client(serverSocket.waitForNextConnection());
        if (!client) continue;

        {
            juce::ScopedLock lk(clientLock);
            activeClient = std::move(client);
        }
        handleClient(*activeClient);
        {
            juce::ScopedLock lk(clientLock);
            activeClient.reset();
        }
    }
}

void DevServer::handleClient(juce::StreamingSocket& client) {
    if (!performHandshake(client)) return;
    juce::Logger::writeToLog("[webplug] Browser connected");

    while (!threadShouldExit() && client.isConnected()) {
        if (client.waitUntilReady(true, 100) != 1) continue;

        juce::String text;
        bool closed = false;
        if (!readFrame(client, text, closed) || closed) break;
        if (text.isNotEmpty() && onMessage) onMessage(text);
    }

    juce::Logger::writeToLog("[webplug] Browser disconnected");
}

bool DevServer::performHandshake(juce::StreamingSocket& client) {
    juce::String request;
    char ch;
    while (client.isConnected()) {
        if (client.read(&ch, 1, true) != 1) return false;
        request += ch;
        if (request.endsWith("\r\n\r\n")) break;
    }

    juce::String key;
    for (auto& line : juce::StringArray::fromLines(request)) {
        if (line.startsWithIgnoreCase("Sec-WebSocket-Key:"))
            key = line.fromFirstOccurrenceOf(":", false, false).trim();
    }
    if (key.isEmpty()) return false;

    juce::String response =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " + wsAcceptKey(key) + "\r\n\r\n";

    auto utf8 = response.toRawUTF8();
    return client.write(utf8, (int)strlen(utf8)) > 0;
}

bool DevServer::sendFrame(juce::StreamingSocket& client, const juce::String& text) {
    auto payload    = text.toRawUTF8();
    size_t plen     = strlen(payload);
    juce::MemoryBlock frame;

    frame.append("\x81", 1); // FIN + TEXT

    if (plen < 126) {
        uint8_t b = (uint8_t)plen;
        frame.append(&b, 1);
    } else if (plen < 65536) {
        uint8_t hdr[3] = { 0x7E, uint8_t(plen>>8), uint8_t(plen) };
        frame.append(hdr, 3);
    } else {
        uint8_t hdr[9] = { 0x7F };
        for (int i = 7; i >= 0; --i) { hdr[1+i] = plen & 0xFF; plen >>= 8; }
        frame.append(hdr, 9);
    }
    frame.append(payload, strlen(payload));
    return client.write(frame.getData(), (int)frame.getSize()) > 0;
}

bool DevServer::readFrame(juce::StreamingSocket& client,
                           juce::String& outText, bool& outClosed) {
    outClosed = false;

    uint8_t hdr[2];
    if (client.read(hdr, 2, true) != 2) return false;

    uint8_t  opcode     = hdr[0] & 0x0F;
    bool     masked     = (hdr[1] >> 7) & 1;
    uint64_t payloadLen = hdr[1] & 0x7F;

    if (payloadLen == 126) {
        uint8_t ext[2];
        if (client.read(ext, 2, true) != 2) return false;
        payloadLen = (uint64_t(ext[0])<<8)|ext[1];
    } else if (payloadLen == 127) {
        uint8_t ext[8];
        if (client.read(ext, 8, true) != 8) return false;
        payloadLen = 0;
        for (int i=0;i<8;++i) payloadLen=(payloadLen<<8)|ext[i];
    }

    uint8_t mask[4] = {};
    if (masked && client.read(mask, 4, true) != 4) return false;

    juce::MemoryBlock payload((size_t)payloadLen);
    if (payloadLen > 0 &&
        client.read(payload.getData(), (int)payloadLen, true) != (int)payloadLen)
        return false;

    if (masked) {
        auto* d = static_cast<uint8_t*>(payload.getData());
        for (size_t i = 0; i < payloadLen; ++i) d[i] ^= mask[i%4];
    }

    if (opcode == 8) { outClosed = true; return true; }
    if (opcode == 1 || opcode == 0)
        outText = juce::String::fromUTF8(
            static_cast<const char*>(payload.getData()), (int)payloadLen);
    return true;
}
