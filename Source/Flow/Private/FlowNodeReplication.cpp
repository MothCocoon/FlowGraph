// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "FlowNodeReplication.h"

#include "AddOns/FlowNodeAddOn.h"
#include "FlowAsset.h"
#include "FlowLogChannels.h"
#include "Nodes/FlowNode.h"

#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Serialization/StructuredArchive.h"
#include "Serialization/StructuredArchiveAdapters.h"
#include "UObject/UnrealType.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNodeReplication)

void FFlowNodeReplicatedState::PostReplicatedAdd(const FFlowNodeReplicatedStateArray& Serializer) const
{
	if (UFlowAsset* FlowAsset = Serializer.OwningAsset.Get())
	{
		FlowAsset->ApplyReplicatedNodeState(*this);
	}
}

void FFlowNodeReplicatedState::PostReplicatedChange(const FFlowNodeReplicatedStateArray& Serializer) const
{
	if (UFlowAsset* FlowAsset = Serializer.OwningAsset.Get())
	{
		FlowAsset->ApplyReplicatedNodeState(*this);
	}
}

namespace FlowNodeReplication
{
	bool HasReplicatedProperties(const UFlowNodeBase& Node)
	{
		for (TFieldIterator<FProperty> It(Node.GetClass()); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Net) && It->GetOwnerClass() != UFlowNode::StaticClass())
			{
				return true;
			}
		}

		for (const UFlowNodeAddOn* AddOn : Node.GetFlowNodeAddOnChildren())
		{
			if (IsValid(AddOn) && HasReplicatedProperties(*AddOn))
			{
				return true;
			}
		}

		return false;
	}

	void SerializeReplicatedProperties(UFlowNodeBase& Node, FArchive& Ar)
	{
		// Writing and reading iterate the same class, so properties come in the same order
		for (TFieldIterator<FProperty> It(Node.GetClass()); It; ++It)
		{
			const FProperty* Property = *It;
			if (Property->HasAnyPropertyFlags(CPF_Net))
			{
				for (int32 ArrayIndex = 0; ArrayIndex < Property->ArrayDim; ++ArrayIndex)
				{
					void* ValuePtr = Property->ContainerPtrToValuePtr<void>(&Node, ArrayIndex);
					Property->SerializeItem(FStructuredArchiveFromArchive(Ar).GetSlot(), ValuePtr);
				}
			}
		}

		// Server and client instance AddOns from the same template, so AddOns come in the same order
		for (UFlowNodeAddOn* AddOn : Node.GetFlowNodeAddOnChildren())
		{
			if (IsValid(AddOn))
			{
				SerializeReplicatedProperties(*AddOn, Ar);
			}
		}
	}

	void WriteReplicatedProperties(UFlowNodeBase& Node, TArray<uint8>& OutData)
	{
		OutData.Reset();

		// FNames and object references are written as strings, since FName indices differ between machines
		FMemoryWriter MemoryWriter(OutData, true);
		FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
		SerializeReplicatedProperties(Node, Ar);
	}

	/* Value of a property marked as UPROPERTY(ReplicatedUsing) from before reading replicated properties. */
	struct FRepNotifyPreviousValue
	{
		UFlowNodeBase* Node = nullptr;
		const FProperty* Property = nullptr;
		void* Value = nullptr;
		bool bChanged = false;

		bool IsIdenticalToCurrentValue() const
		{
			for (int32 ArrayIndex = 0; ArrayIndex < Property->ArrayDim; ++ArrayIndex)
			{
				const void* PreviousElement = static_cast<const uint8*>(Value) + Property->GetElementSize() * ArrayIndex;
				if (!Property->Identical(PreviousElement, Property->ContainerPtrToValuePtr<void>(Node, ArrayIndex)))
				{
					return false;
				}
			}

			return true;
		}


		static void CaptureRepNotifyPreviousValues(UFlowNodeBase& Node, TArray<FRepNotifyPreviousValue>& OutPreviousValues)
		{
			for (TFieldIterator<FProperty> It(Node.GetClass()); It; ++It)
			{
				const FProperty* Property = *It;
				if (Property->HasAllPropertyFlags(CPF_Net | CPF_RepNotify))
				{
					void* Value = Property->AllocateAndInitializeValue();
					Property->CopyCompleteValue(Value, Property->ContainerPtrToValuePtr<void>(&Node));
					OutPreviousValues.Add({&Node, Property, Value});
				}
			}

			for (UFlowNodeAddOn* AddOn : Node.GetFlowNodeAddOnChildren())
			{
				if (IsValid(AddOn))
				{
					CaptureRepNotifyPreviousValues(*AddOn, OutPreviousValues);
				}
			}
		}

		void CallRepNotify() const
		{
			UFunction* RepNotifyFunc = Node->FindFunction(Property->RepNotifyFunc);
			if (RepNotifyFunc == nullptr)
			{
				UE_LOG(LogFlow, Warning, TEXT("Can't find RepNotify function %s for property %s on %s."), *Property->RepNotifyFunc.ToString(), *Property->GetName(), *Node->GetName());
			}
			else if (RepNotifyFunc->NumParms == 0)
			{
				Node->ProcessEvent(RepNotifyFunc, nullptr);
			}
			else if (RepNotifyFunc->NumParms == 1)
			{
				// The parameter receives the previous value, bitfield is passed as a native bool
				const FBoolProperty* BoolProperty = CastField<const FBoolProperty>(Property);
				if (BoolProperty && !BoolProperty->IsNativeBool())
				{
					bool bPreviousValue = BoolProperty->GetPropertyValue(Value);
					Node->ProcessEvent(RepNotifyFunc, &bPreviousValue);
				}
				else
				{
					Node->ProcessEvent(RepNotifyFunc, Value);
				}
			}
			else
			{
				UE_LOG(LogFlow, Warning, TEXT("RepNotify function %s on %s has more than one parameter, which isn't supported."), *RepNotifyFunc->GetName(), *Node->GetName());
			}
		}
	};

	void ReadReplicatedProperties(UFlowNodeBase& Node, const TArray<uint8>& Data)
	{
		if (Data.Num() > 0)
		{
			TArray<FRepNotifyPreviousValue> PreviousValues;
			FRepNotifyPreviousValue::CaptureRepNotifyPreviousValues(Node, PreviousValues);

			FMemoryReader MemoryReader(Data, true);
			FObjectAndNameAsStringProxyArchive Ar(MemoryReader, true);
			SerializeReplicatedProperties(Node, Ar);

			// Changes are checked before calling any RepNotify, since RepNotify may modify other properties
			for (FRepNotifyPreviousValue& PreviousValue : PreviousValues)
			{
				PreviousValue.bChanged = !PreviousValue.IsIdenticalToCurrentValue();
			}

			for (const FRepNotifyPreviousValue& PreviousValue : PreviousValues)
			{
				if (PreviousValue.bChanged)
				{
					PreviousValue.CallRepNotify();
				}

				PreviousValue.Property->DestroyAndFreeValue(PreviousValue.Value);
			}
		}
	}
}
