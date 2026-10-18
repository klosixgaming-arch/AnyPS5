#include "Ngs2Test.hpp"

#include <iostream>
#include <stdexcept>

static void Status(int actual, int expected, const char* message) {
    if (actual != expected) throw std::runtime_error(message);
}

int main() {
    try {
        constexpr int systemError = static_cast<int>(0x804A8201u);
        constexpr int rackError = static_cast<int>(0x804A8202u);
        constexpr int outputError = static_cast<int>(0x804A8010u);
        constexpr uintptr_t sentinel = 0x5a5a;
        uintptr_t value = sentinel;
        Status(sceNgs2SystemGetUserData(0x1234, &value), systemError, "invalid system handle returned a non-PS5 error");
        Require(value == sentinel);
        Status(sceNgs2SystemDestroy(0x1234, nullptr), systemError, "system destroy returned a non-PS5 handle error");
        Status(sceNgs2RackGetVoiceHandle(0x1234, 0, &value), rackError, "invalid rack handle returned a non-PS5 error");
        Require(value == sentinel);
        Status(sceNgs2RackGetVoiceHandle(0x1234, 0, nullptr), rackError, "voice lookup checked output before the rack handle");
        Status(sceNgs2RackDestroy(0x1234, nullptr), rackError, "rack destroy returned a non-PS5 handle error");
        const auto system = CreateSystem();
        const auto rack = CreateRack(system, SCE_NGS2_RACK_ID_REVERB);
        Status(sceNgs2SystemGetUserData(rack, &value), systemError, "rack handle was accepted as a system");
        Require(value == sentinel);
        Status(sceNgs2RackGetVoiceHandle(system, 0, &value), rackError, "system handle was accepted as a rack");
        Require(value == sentinel);
        Status(sceNgs2RackGetVoiceHandle(system, 0, nullptr), rackError, "wrong-kind rack did not take precedence over null output");
        Status(sceNgs2RackGetVoiceHandle(rack, 0, nullptr), outputError, "valid rack lost null-output validation");
        Status(sceNgs2RackGetVoiceHandle(rack, 0, &value), SCE_NGS2_OK, "valid voice lookup failed");
        Require(value != 0 && value != sentinel);
        constexpr int voiceError = static_cast<int>(0x804A8203u);
        Ngs2VoiceParamHeader header{};
        Ngs2VoiceState state{};
        Ngs2VoicePortInfo portInfo{};
        Ngs2VoiceChannelsInfo channels{};
        uint32_t flags = 0;
        for (const uintptr_t handle : {uintptr_t{0}, uintptr_t{0x1234}, rack, system}) {
            Status(sceNgs2VoiceControl(handle, &header), voiceError, "voice control accepted an invalid voice handle");
            Status(sceNgs2VoiceRunCommands(handle, nullptr, 0), voiceError, "voice commands accepted an invalid voice handle");
            Status(sceNgs2VoiceGetState(handle, &state, sizeof(state)), voiceError, "voice state accepted an invalid voice handle");
            Status(sceNgs2VoiceGetPortInfo(handle, 0, &portInfo, sizeof(portInfo)), voiceError, "voice port info accepted an invalid voice handle");
            Status(sceNgs2VoiceQueryInfo(handle, SCE_NGS2_VOICE_INFO_CHANNELS, &channels, sizeof(channels)), voiceError, "voice info accepted an invalid voice handle");
            Status(sceNgs2VoiceGetStateFlags(handle, &flags), voiceError, "voice state flags accepted an invalid voice handle");
        }
        const uintptr_t liveVoice = value;
        Status(sceNgs2VoiceGetStateFlags(liveVoice, &flags), SCE_NGS2_OK, "valid voice lost state flags");
        Status(sceNgs2RackDestroy(rack, nullptr), SCE_NGS2_OK, "valid rack destroy failed");
        Status(sceNgs2VoiceGetStateFlags(liveVoice, &flags), voiceError, "voice of a destroyed rack was still accepted");
        value = sentinel;
        Status(sceNgs2RackGetVoiceHandle(rack, 0, &value), rackError, "retired rack returned the wrong error");
        Require(value == sentinel);
        Status(sceNgs2RackGetVoiceHandle(rack, 0, nullptr), rackError, "retired rack did not take precedence over null output");
        Status(sceNgs2SystemDestroy(system, nullptr), SCE_NGS2_OK, "valid system destroy failed");
        Status(sceNgs2SystemGetUserData(system, &value), systemError, "retired system returned the wrong error");
        Require(value == sentinel);
        std::cout << "NGS2 PS5 handle errors and validation precedence checked\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
