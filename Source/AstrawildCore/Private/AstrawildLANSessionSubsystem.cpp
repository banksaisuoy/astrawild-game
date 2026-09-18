#include "AstrawildLANSessionSubsystem.h"

#include "AstrawildCore.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerController.h"
#include "AstrawildSaveSubsystem.h"
#include "Containers/Ticker.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "SocketSubsystem.h"
#include "Sockets.h"

namespace
{
    /** Discovery expiry — stale hosts drop off the list after this many seconds. */
    constexpr double SessionExpirySeconds = 8.0;

    /** Broadcast datagram target: the whole local subnet. */
    constexpr const TCHAR* LanBroadcastAddress = TEXT("255.255.255.255");
}

void UAstrawildLANSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // One lightweight core ticker drives the beacon + discovery (0.5 s cadence
    // — the broadcast itself is 1 Hz, the discovery poll 2 Hz). Survives map
    // travel (GameInstance lifetime) and dies with the process.
    TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UAstrawildLANSessionSubsystem::HandleTicker), 0.5f);
}

void UAstrawildLANSessionSubsystem::Deinitialize()
{
    StopLanBeacon();
    StopLanDiscovery();
    if (TickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
        TickerHandle.Reset();
    }
    Super::Deinitialize();
}

UWorld* UAstrawildLANSessionSubsystem::GetGameWorld() const
{
    const UGameInstance* GI = GetGameInstance();
    return GI ? GI->GetWorld() : nullptr;
}

// ---------------------------------------------------------------------------
// Pure protocol codec (world-free, automation-tested)
// ---------------------------------------------------------------------------

FString UAstrawildLANSessionSubsystem::EncodeBeaconPayload(const int32 ListenPort, const int32 PlayerCount)
{
    // "AWLAN1|1|7777|2" — magic, protocol version, listen port, player count.
    return FString::Printf(TEXT("%s|%d|%d|%d"), BeaconMagic, 1, ListenPort, PlayerCount);
}

bool UAstrawildLANSessionSubsystem::DecodeBeaconPayload(const FString& Raw, int32& OutListenPort, int32& OutPlayerCount)
{
    OutListenPort = 0;
    OutPlayerCount = 0;

    TArray<FString> Parts;
    Raw.ParseIntoArray(Parts, TEXT("|"), true /*bCullEmpty*/);
    if (Parts.Num() != 4)
    {
        return false;
    }
    if (Parts[0] != BeaconMagic)
    {
        return false;
    }
    const int32 Version = TCString<TCHAR>::Atoi(*Parts[1]);
    if (Version != 1)
    {
        return false; // protocol mismatch — never mix LAN builds
    }
    OutListenPort = TCString<TCHAR>::Atoi(*Parts[2]);
    OutPlayerCount = TCString<TCHAR>::Atoi(*Parts[3]);
    if (OutListenPort <= 0 || OutListenPort > 65535)
    {
        return false;
    }
    if (OutPlayerCount < 1 || OutPlayerCount > MaxLanPlayers)
    {
        return false; // a foreign/hostile beacon cannot advertise a full lie
    }
    return true;
}

bool UAstrawildLANSessionSubsystem::ParseDirectAddress(const FString& Address, FString& OutHost, int32& OutPort)
{
    OutHost.Reset();
    OutPort = DefaultGamePort;

    FString Trimmed = Address;
    Trimmed.TrimStartAndEnd();
    if (Trimmed.IsEmpty())
    {
        return false;
    }

    const int32 ColonIndex = Trimmed.Find(TEXT(":"), ESearchCase::CaseSensitive, ESearchDir::FromStart);
    if (ColonIndex == INDEX_NONE)
    {
        OutHost = Trimmed;
        return true;
    }
    if (ColonIndex == 0 || ColonIndex == Trimmed.Len() - 1)
    {
        return false; // ":port" / "host:" — malformed
    }
    OutHost = Trimmed.Left(ColonIndex);
    const FString PortPart = Trimmed.RightChop(ColonIndex + 1);
    OutPort = TCString<TCHAR>::Atoi(*PortPart);
    if (OutPort <= 0 || OutPort > 65535)
    {
        return false;
    }
    return true;
}

FString UAstrawildLANSessionSubsystem::DescribeSessionMode(const UObject* WorldContext)
{
    // PART 6: the active mode must be OBVIOUS. One HUD line, three states.
    const UWorld* World = GEngine && WorldContext ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World)
    {
        return TEXT("");
    }
    const ENetMode Mode = World->GetNetMode();
    if (Mode == NM_Client)
    {
        return TEXT("LAN CLIENT — connected to the host's world");
    }
    if (Mode == NM_ListenServer)
    {
        int32 Players = 0;
        for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
        {
            if (Cast<APlayerController>(*It))
            {
                ++Players;
            }
        }
        return FString::Printf(TEXT("LAN HOST — you hold the authoritative world (%d/%d players)"), Players, MaxLanPlayers);
    }
    return TEXT("SINGLE PLAYER");
}

// ---------------------------------------------------------------------------
// Host
// ---------------------------------------------------------------------------

bool UAstrawildLANSessionSubsystem::HostLANGame()
{
    UWorld* World = GetGameWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        UE_LOG(LogAstrawildNetwork, Warning, TEXT("HostLANGame: refused — no world or already a client."));
        return false;
    }

    // Save first (PART 7): host progress survives the rehost; the autoload
    // option restores it in the listen world's BeginPlay.
    UGameInstance* GI = GetGameInstance();
    if (UAstrawildSaveSubsystem* Save = GI ? GI->GetSubsystem<UAstrawildSaveSubsystem>() : nullptr)
    {
        Save->SaveWorld(World);
    }

    // Rehost the CURRENT map as a listen server with autoload. RemovePIEPrefix
    // keeps PIE map names valid.
    const FString MapName = World->RemovePIEPrefix(World->GetMapName());
    const FString TravelURL = FString::Printf(TEXT("%s?listen?autoload=1"), *MapName);
    UE_LOG(LogAstrawildNetwork, Log, TEXT("LCP-6: hosting LAN game — ServerTravel('%s')."), *TravelURL);
    World->ServerTravel(TravelURL, false);

    bPendingBeaconStart = true;
    return true;
}

bool UAstrawildLANSessionSubsystem::OpenBeaconSocket()
{
    if (BeaconSocket)
    {
        return true;
    }

    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        return false;
    }

    BeaconSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("AstrawildLanBeacon"), false);
    if (!BeaconSocket)
    {
        return false;
    }
    BeaconSocket->SetBroadcast(true);
    BeaconSocket->SetReuseAddress(true);
    BeaconSocket->SetNonBlocking(true);

    // Bind to any local address/port — we only SEND broadcasts.
    TSharedRef<FInternetAddr> BindAddr = SocketSubsystem->GetLocalBindAddr(*GLog);
    BindAddr->SetPort(0);
    if (!BeaconSocket->Bind(*BindAddr))
    {
        UE_LOG(LogAstrawildNetwork, Warning, TEXT("LCP-6: beacon bind failed."));
        StopLanBeacon();
        return false;
    }

    TSharedRef<FInternetAddr> TargetAddr = SocketSubsystem->CreateInternetAddr();
    bool bIsValid = false;
    TargetAddr->SetIp(LanBroadcastAddress, bIsValid);
    if (!bIsValid)
    {
        StopLanBeacon();
        return false;
    }
    return true;
}

void UAstrawildLANSessionSubsystem::BroadcastBeacon()
{
    if (!BeaconSocket)
    {
        return;
    }

    UWorld* World = GetGameWorld();
    int32 PlayerCount = 1;
    if (World)
    {
        PlayerCount = 0;
        for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
        {
            if (Cast<APlayerController>(*It))
            {
                ++PlayerCount;
            }
        }
        PlayerCount = FMath::Max(1, PlayerCount);
    }

    const FString Payload = EncodeBeaconPayload(DefaultGamePort, PlayerCount);
    const FTCHARToUTF8 PayloadUtf8(*Payload);

    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (SocketSubsystem)
    {
        TSharedRef<FInternetAddr> Target = SocketSubsystem->CreateInternetAddr();
        bool bIsValid = false;
        Target->SetIp(LanBroadcastAddress, bIsValid);
        Target->SetPort(BeaconPort);
        if (bIsValid)
        {
            int32 BytesSent = 0;
            BeaconSocket->SendTo(reinterpret_cast<const uint8*>(PayloadUtf8.Get()), PayloadUtf8.Length(), BytesSent, *Target);
        }
    }
}

void UAstrawildLANSessionSubsystem::StopLanBeacon()
{
    if (BeaconSocket)
    {
        ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
        if (SocketSubsystem)
        {
            SocketSubsystem->DestroySocket(BeaconSocket);
        }
        BeaconSocket = nullptr;
    }
    bPendingBeaconStart = false;
}

// ---------------------------------------------------------------------------
// Discovery (client)
// ---------------------------------------------------------------------------

void UAstrawildLANSessionSubsystem::StartLanDiscovery()
{
    if (DiscoverySocket)
    {
        return; // already listening
    }

    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        return;
    }

    DiscoverySocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("AstrawildLanListen"), false);
    if (!DiscoverySocket)
    {
        return;
    }
    DiscoverySocket->SetReuseAddress(true); // multiple clients on one test machine
    DiscoverySocket->SetNonBlocking(true);

    TSharedRef<FInternetAddr> ListenAddr = SocketSubsystem->GetLocalBindAddr(*GLog);
    ListenAddr->SetPort(BeaconPort);
    if (!DiscoverySocket->Bind(*ListenAddr))
    {
        UE_LOG(LogAstrawildNetwork, Warning, TEXT("LCP-6: discovery bind failed (another client owns the port?)."));
        StopLanDiscovery();
        return;
    }
    DiscoveredSessions.Reset();
    UE_LOG(LogAstrawildNetwork, Log, TEXT("LCP-6: LAN discovery listening on UDP %d."), BeaconPort);
}

void UAstrawildLANSessionSubsystem::StopLanDiscovery()
{
    if (DiscoverySocket)
    {
        ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
        if (SocketSubsystem)
        {
            SocketSubsystem->DestroySocket(DiscoverySocket);
        }
        DiscoverySocket = nullptr;
    }
    DiscoveredSessions.Reset();
}

void UAstrawildLANSessionSubsystem::TickDiscovery()
{
    if (!DiscoverySocket)
    {
        return;
    }

    const double Now = FPlatformTime::Seconds();

    // Receive every pending datagram (usually 0-2).
    constexpr int32 MaxPacketBytes = 128;
    uint8 Buffer[MaxPacketBytes + 1]; // +1: room for a null terminator
    uint32 PendingSize = 0;
    int32 ReadBytes = 0;

    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    TSharedPtr<FInternetAddr> Source = SocketSubsystem ? SocketSubsystem->CreateInternetAddr() : nullptr;

    while (DiscoverySocket && Source && DiscoverySocket->HasPendingData(PendingSize) && PendingSize > 0)
    {
        ReadBytes = 0;
        if (!DiscoverySocket->RecvFrom(Buffer, MaxPacketBytes, ReadBytes, *Source))
        {
            break;
        }
        if (ReadBytes <= 0)
        {
            break;
        }

        // Fail-closed decode: malformed/foreign packets never enter the list.
        Buffer[ReadBytes] = 0; // the payload is UTF-8 text — terminate, then convert.
        const FString Payload = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Buffer)));
        int32 ListenPort = 0;
        int32 PlayerCount = 0;
        if (DecodeBeaconPayload(Payload, ListenPort, PlayerCount))
        {
            FAstrawildLanSessionInfo Session;
            Session.HostAddress = Source->ToString(false);
            Session.HostPort = ListenPort;
            Session.PlayerCount = PlayerCount;
            Session.MaxPlayers = MaxLanPlayers;
            DiscoveredSessions.Add(Session, Now);
        }
    }

    // Expire stale hosts.
    for (auto It = DiscoveredSessions.CreateIterator(); It; ++It)
    {
        if (Now - It->Value > SessionExpirySeconds)
        {
            It.RemoveCurrent();
        }
    }
}

TArray<FAstrawildLanSessionInfo> UAstrawildLANSessionSubsystem::GetDiscoveredSessions() const
{
    TArray<FAstrawildLanSessionInfo> Out;
    DiscoveredSessions.GetKeys(Out);
    return Out;
}

// ---------------------------------------------------------------------------
// Join
// ---------------------------------------------------------------------------

bool UAstrawildLANSessionSubsystem::JoinSession(const FAstrawildLanSessionInfo& Session)
{
    UWorld* World = GetGameWorld();
    APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
    if (!World || !PC || Session.HostAddress.IsEmpty() || Session.HostPort <= 0)
    {
        return false;
    }
    const FString Address = FString::Printf(TEXT("%s:%d"), *Session.HostAddress, Session.HostPort);
    UE_LOG(LogAstrawildNetwork, Log, TEXT("LCP-6: joining LAN session at %s."), *Address);
    StopLanDiscovery();
    PC->ClientTravel(Address);
    return true;
}

bool UAstrawildLANSessionSubsystem::ConnectDirect(const FString& Address)
{
    FString Host;
    int32 Port = DefaultGamePort;
    if (!ParseDirectAddress(Address, Host, Port))
    {
        UE_LOG(LogAstrawildNetwork, Warning, TEXT("LCP-6: direct address '%s' is malformed."), *Address);
        return false;
    }

    UWorld* World = GetGameWorld();
    APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
    if (!World || !PC)
    {
        return false;
    }
    const FString Target = FString::Printf(TEXT("%s:%d"), *Host, Port);
    UE_LOG(LogAstrawildNetwork, Log, TEXT("LCP-6: direct connect to %s."), *Target);
    StopLanDiscovery();
    PC->ClientTravel(Target);
    return true;
}

// ---------------------------------------------------------------------------
// Ticker
// ---------------------------------------------------------------------------

void UAstrawildLANSessionSubsystem::TickBeaconStartupCheck()
{
    if (!bPendingBeaconStart)
    {
        return;
    }
    UWorld* World = GetGameWorld();
    if (!World)
    {
        return;
    }
    const ENetMode Mode = World->GetNetMode();
    if (Mode == NM_ListenServer)
    {
        if (OpenBeaconSocket())
        {
            UE_LOG(LogAstrawildNetwork, Log, TEXT("LCP-6: LAN beacon live (UDP %d, game port %d)."), BeaconPort, DefaultGamePort);
        }
        bPendingBeaconStart = false;
    }
    else if (Mode == NM_Standalone || Mode == NM_DedicatedServer)
    {
        // Travel did not produce a listen world — drop the pending request.
        bPendingBeaconStart = false;
    }
}

void UAstrawildLANSessionSubsystem::TickBeaconBroadcast(const float DeltaTime)
{
    BeaconBroadcastAccumulator += DeltaTime;
    if (BeaconBroadcastAccumulator >= 1.0f)
    {
        BeaconBroadcastAccumulator = 0.0f;
        BroadcastBeacon();
    }
}

bool UAstrawildLANSessionSubsystem::HandleTicker(const float DeltaTime)
{
    TickBeaconStartupCheck();
    if (BeaconSocket)
    {
        TickBeaconBroadcast(DeltaTime);
    }
    if (DiscoverySocket)
    {
        DiscoveryPollAccumulator += DeltaTime;
        if (DiscoveryPollAccumulator >= 0.5f)
        {
            DiscoveryPollAccumulator = 0.0f;
            TickDiscovery();
        }
    }
    return true; // keep ticking for the subsystem's lifetime
}
