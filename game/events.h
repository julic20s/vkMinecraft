#pragma once
#ifndef VKMC_EVENTS_H_
#define VKMC_EVENTS_H_

namespace events {

constexpr auto kChunkLoaded = "chunk_load";
constexpr auto kChunkUnloaded = "chunk_unload";
constexpr auto kChunkUpdate = "chunk_update";

} // namespace events

#endif // VKMC_EVENTS_H_
