// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Algo/Unique.h"
#include "Containers/Array.h"
#include "Math/RandomStream.h"

namespace FlowArray
{
	// Alias for inline-allocated TArray 
	//  (NOTE, UE's TArray will reallocate to heap ("secondary allocation")
	//   if the fixed capacity is ever exceeded)

	template <class TInnerType, int Capacity>
	using TInlineArray = TArray<TInnerType, TInlineAllocator<Capacity>>;

	template <class TInnerType, typename InAllocatorType>
	void ReverseArray(TArray<TInnerType>& InOutArray)
	{
		for (int32 FrontIndex = 0, BackIndex = InOutArray.Num() - 1; FrontIndex < BackIndex; ++FrontIndex, --BackIndex)
		{
			InOutArray.Swap(FrontIndex, BackIndex);
		}
	}

	template <typename TInnerType, typename InAllocatorType>
	void ShuffleArray(TArray<TInnerType, InAllocatorType>& Array, FRandomStream& RandomStream)
	{
		// Trivial cases
		if (Array.Num() <= 2)
		{
			if (Array.Num() == 2)
			{
				const bool bShouldSwap = RandomStream.RandRange(0, 1) == 0;
				if (bShouldSwap)
				{
					Array.Swap(0, 1);
				}
			}

			return;
		}

		// Simple shuffle, attempt swaps for each index in the array, once each
		for (int32 FromIndex = 0; FromIndex < Array.Num(); ++FromIndex)
		{
			const int32 IndexOffset = RandomStream.RandRange(1, Array.Num() - 1);
			const int32 OtherIndex = (FromIndex + IndexOffset) % Array.Num();
			check(FromIndex != OtherIndex);

			Array.Swap(FromIndex, OtherIndex);
		}
	}

	template <typename TInnerType>
	bool TrySortAndRemoveDuplicatesFromArrayInPlace(TArray<TInnerType>& InOutArray)
	{
		InOutArray.Sort(TGreater{});

		const int32 SizeBefore = InOutArray.Num();
		const int32 SizeAfter = Algo::Unique(InOutArray);

		if (SizeBefore > SizeAfter)
		{
			InOutArray.SetNum(SizeAfter);

			return true;
		}

		return false;
	}
}