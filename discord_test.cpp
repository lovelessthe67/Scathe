#include "discord-rpc/include/discord_rpc.h"
#include <time.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#pragma comment(lib, "discord-rpc.lib")
#endif

static const char* CLIENT_ID = "1455713142897250556";

void UpdateDiscordPresence(const char* state, const char* details, const char* largeImg, const char* smallImg) {
    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));

    discordPresence.state          = state;
    discordPresence.details        = details;
    discordPresence.startTimestamp = time(NULL);

    discordPresence.largeImageKey  = largeImg;
    discordPresence.smallImageKey  = smallImg;

    Discord_UpdatePresence(&discordPresence);
}

void InitDiscord(const char* clientId) {
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    Discord_Initialize(clientId, &handlers, 1, NULL);
}

void ShutdownDiscord() {
    Discord_Shutdown();
}

void RunRPC() {
    InitDiscord(CLIENT_ID);

    UpdateDiscordPresence(
        "Join Scathe The Best Cheat!",
        "discord.gg/scathe",
        "large_image_key",
        "small_image_key"
    );

    printf("Rich Presence set. Press Enter to exit...\n");
    getchar();

    ShutdownDiscord();
}

int main() {
    RunRPC();
    return 0;
}
