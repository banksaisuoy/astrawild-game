#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AstrawildLANSessionSubsystem.generated.h"

class FSocket;

/** One discovered LAN game (client-side list entry). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildLanSessionInfo
{
    GENERATED_BODY()

    /** Sender IPv4 as text (from the discovery datagram's source address). */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|LAN")
    FString HostAddress;

    /** The host's gameplay listen port (from the packet payload — NOT the beacon port). */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|LAN")
    int32 HostPort = 7777;

    /** Players currently connected (from the packet payload). */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|LAN")
    int32 PlayerCount = 1;

    /** Session capacity — the personal-LAN build is 4 players total (PART 1). */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|LAN")
    int32 MaxPlayers = 4;

    bool operator==(const FAstrawildLanSessionInfo& Other) const
    {
        return HostAddress == Other.HostAddress && HostPort == Other.HostPort;
    }
};

/**
 * LCP-6 (LAN co-op): the session flow for the personal 4-player build.
 *
 * HOST:   pause menu → HostLANGame() → save world → ServerTravel("<map>?listen&autoload")
 *         → the beacon broadcasts presence on UDP 45861 (1 Hz) once the listen
 *         world is live.
 * CLIENT: pause menu → StartLanDiscovery() → beacon listener (poll 2 Hz) →
 *         JoinSession / ConnectDirect → ClientTravel("IP:7777").
 *
 * The beacon is DISCOVERY ONLY — gameplay networking is the engine's normal
 * server-authoritative IpConnection replication (PART 2/§1b: no parallel
 * architecture, no dedicated server, no external services). The protocol is a
 * pure encode/decode pair (world-free automation-tested) — a malformed or
 * foreign packet NEVER reaches the session list.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildLANSessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** Beacon magic — every packet starts with this exact token. */
    static constexpr const TCHAR* BeaconMagic = TEXT("AWLAN1");
    /** Discovery port: host broadcasts here, every client listens here (SO_REUSEADDR). */
    static constexpr int32 BeaconPort = 45861;
    /** Default gameplay listen port clients connect to. */
    static constexpr int32 DefaultGamePort = 7777;
    /** Max players (PART 1 scope guard — the beacon advertises the cap). */
    static constexpr int32 MaxLanPlayers = 4;

    // --- Host ---

    /**
     * Save the current world (host progress survives the rehost), then
     * ServerTravel the current map as a LISTEN SERVER with autoload. The
     * beacon starts once the listen world is live (core-ticker netmode check).
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|LAN|Host")
    bool HostLANGame();

    /** True while the beacon is broadcasting. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|LAN|Host")
    bool IsHostingLanBeacon() const { return BeaconSocket != nullptr; }

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|LAN|Host")
    void StopLanBeacon();

    // --- Discovery (client) ---

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|LAN|Discover")
    void StartLanDiscovery();

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|LAN|Discover")
    void StopLanDiscovery();

    /** Discovered sessions (deduped by host address; 8 s expiry). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|LAN|Discover")
    TArray<FAstrawildLanSessionInfo> GetDiscoveredSessions() const;

    // --- Join ---

    /** Join a discovered session (ClientTravel to the host's listen port). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|LAN|Join")
    bool JoinSession(const FAstrawildLanSessionInfo& Session);

    /** Direct-IP connect: "192.168.1.5" or "192.168.1.5:7777" (fallback port 7777). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|LAN|Join")
    bool ConnectDirect(const FString& Address);

    // --- Pure protocol helpers (world-free automation-tested) ---

    /** Encode one beacon payload: "AWLAN1|<version>|<listenPort>|<players>". */
    static FString EncodeBeaconPayload(int32 ListenPort, int32 PlayerCount);

    /**
     * Fail-closed decode: magic, version, port and player count must all be
     * valid. Returns false on ANY malformed input.
     */
    static bool DecodeBeaconPayload(const FString& Raw, int32& OutListenPort, int32& OutPlayerCount);

    /** Parse "host[:port]" (default port 7777). Fail-closed on empty/garbage. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|LAN|Join")
    static bool ParseDirectAddress(const FString& Address, FString& OutHost, int32& OutPort);

    /** The HUD mode line (PART 6: the mode must be obvious). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|LAN|UI")
    static FString DescribeSessionMode(const UObject* WorldContext);

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    /** Core ticker: beacon broadcast (1 Hz) + pending-beacon netmode check + discovery poll (2 Hz). */
    bool HandleTicker(float DeltaTime);
    void TickBeaconBroadcast(float DeltaTime);
    void TickDiscovery();
    void TickBeaconStartupCheck();

    /** Open/refresh the host beacon socket. */
    bool OpenBeaconSocket();

    /** Send the current player count in one broadcast datagram. */
    void BroadcastBeacon();

    class UWorld* GetGameWorld() const;

    /** Host beacon socket (null when not hosting). */
    FSocket* BeaconSocket = nullptr;

    /** Client discovery socket (null when not listening). */
    FSocket* DiscoverySocket = nullptr;

    /** Core ticker handle. */
    FTSTicker::FDelegateHandle TickerHandle;

    bool bPendingBeaconStart = false;

    /** Discovery bookkeeping: session -> last-seen time. */
    TMap<FAstrawildLanSessionInfo, double> DiscoveredSessions;

    /** Broadcast/poll cadence accumulators. */
    float BeaconBroadcastAccumulator = 0.0f;
    float DiscoveryPollAccumulator = 0.0f;
};
