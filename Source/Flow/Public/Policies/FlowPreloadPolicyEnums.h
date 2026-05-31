// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "Types/FlowEnumUtils.h"
#include "FlowPreloadPolicyEnums.generated.h"

/* 
 * Timing for when a preloadable node's content should be preloaded.
 */
UENUM()
enum class EFlowPreloadTiming : uint8
{
	/* Preload content when the graph instance is initialized. */
	OnGraphInitialize,

	/* Preload content when the node activates (just-in-time before execution). */
	OnActivate,

	/* Do not automatically preload; content is ONLY preloaded when the Preload exec pin is triggered. */
	ManualOnly,

	Max     UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowPreloadTiming);

/*
 * Timing for when a preloadable node's content should be flushed.
 */
UENUM()
enum class EFlowFlushTiming : uint8
{
	/* Flush content when the graph instance is deinitialized. */
	OnGraphDeinitialize,

	/* Flush content when the node finishes execution. */
	OnNodeFinish,

	/* Do not automatically flush; content is ONLY flushed when the Flush exec pin is triggered. */
	ManualOnly,

	Max     UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowFlushTiming);

/* 
 * Return value of IFlowPreloadableInterface::PreloadContent().
 * Tells the preload helper whether the node finished synchronously or deferred completion.
 */
UENUM()
enum class EFlowPreloadResult : uint8
{
	/* Preloading completed synchronously. The helper fires AllPreloadsComplete immediately. */
	Completed,

	/* Preloading started but is not yet finished (e.g. async asset streaming).
	 * The node MUST call NotifyPreloadComplete() on itself (game thread) when loading finishes.
	 * The helper fires AllPreloadsComplete only when that call arrives. */
	PreloadInProgress,

	Max     UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowPreloadResult);

/* Return value of FFlowPreloadHelper::OnNodeExecuteInput().
 * Indicates whether the helper consumed the input pin or it should pass through to the node. */
UENUM()
enum class EFlowPreloadInputResult : uint8
{
	/* The helper handled this pin (e.g. Preload or Flush exec). Do not pass it to the node. */
	Handled,

	/* This pin is not a preload pin; pass through to the node's ExecuteInput. */
	Unhandled,

	Max     UMETA(Hidden),
	Invalid UMETA(Hidden),
	Min = 0 UMETA(Hidden),
};
FLOW_ENUM_RANGE_VALUES(EFlowPreloadInputResult);
