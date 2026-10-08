// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "FlowGraphImporter.h"
#include "FlowGraphExporter.h"
#include "FlowGraphReconciler.h"
#include "FlowGraphRegrapher.h"
#include "FlowCourierConverter.h"
#include "FlowCourierDocument.h"
#include "FlowAsset.h"
#include "Nodes/FlowNode.h"
#include "Nodes/Graph/FlowNode_CustomInput.h"
#include "Nodes/Graph/FlowNode_CustomOutput.h"
#include "AddOns/FlowNodeAddOn.h"
#include "AddOns/FlowNodeAddOn_SwitchCase.h"
#include "Graph/FlowGraph.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Misc/AutomationTest.h"
#include "Editor.h"

#include "Nodes/Actor/FlowNode_ExecuteComponent.h"
#include "Components/BillboardComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FFlowGraphImporterSpec, "FlowGraphCourier.EditorGame.Importer", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext)
	UFlowAsset* TestFlowAsset;
	FString TestImportText;

	// Re-imports a Courier v2 JSON document (as produced by UFlowGraphExporter::ExportFlowGraphToString)
	// into a new asset at TargetAssetPath - the document-based equivalent of feeding exported text
	// back into ImportFlowGraphFromText, which only accepts v1 grammar.
	static UFlowAsset* ImportFromExportedJson(const FString& TargetAssetPath, const FString& ExportedJson, FString& OutErrorMessage)
	{
		FFlowCourierDocument Document;
		TArray<FFlowCourierIssue> Issues;
		if (!FFlowCourierConverter::ParseDocument(ExportedJson, Document, Issues, OutErrorMessage))
		{
			return nullptr;
		}

		TArray<FFlowGraphParsedNode> ParsedNodes;
		TArray<FFlowGraphParsedConnection> ParsedConnections;
		TArray<FGuid> ScopedNodeGuidsUnused;
		TMap<FString, FGuid> AliasMapUnused;
		FFlowCourierConverter::ConvertToParsedGraph(Document, ParsedNodes, ParsedConnections, ScopedNodeGuidsUnused, AliasMapUnused, Issues);

		return UFlowGraphImporter::ImportFlowGraphFromDocument(
			TargetAssetPath, Document.AssetClass, Document.bWorldBound, ParsedNodes, ParsedConnections, OutErrorMessage);
	}

	// Document-based equivalent of UFlowGraphRegrapher::ImportAndRegraphFromText, which only
	// accepts v1 grammar.
	static UFlowAsset* ImportAndRegraphFixtureDocument(const FString& Json, const FString& TargetAssetPath)
	{
		FFlowCourierDocument Document;
		TArray<FFlowCourierIssue> Issues;
		FString ErrorMessage;
		if (!FFlowCourierConverter::ParseDocument(Json, Document, Issues, ErrorMessage))
		{
			return nullptr;
		}
		TArray<FFlowGraphParsedNode> ParsedNodes;
		TArray<FFlowGraphParsedConnection> ParsedConnections;
		TArray<FGuid> ScopedNodeGuidsUnused;
		TMap<FString, FGuid> AliasMapUnused;
		FFlowCourierConverter::ConvertToParsedGraph(Document, ParsedNodes, ParsedConnections, ScopedNodeGuidsUnused, AliasMapUnused, Issues);
		return UFlowGraphRegrapher::ImportAndRegraphFromDocument(TargetAssetPath, Document.AssetClass, Document.bWorldBound, ParsedNodes, ParsedConnections);
	}
END_DEFINE_SPEC(FFlowGraphImporterSpec)

void FFlowGraphImporterSpec::Define()
{
	Describe("FlowGraphImporter", [this]()
	{
		BeforeEach([this]()
		{
			TestFlowAsset = nullptr;
			TestImportText.Empty();
		});

		AfterEach([this]()
		{
			if (TestFlowAsset)
			{
				TestFlowAsset->ClearFlags(RF_Standalone);
				TestFlowAsset->MarkAsGarbage();
				TestFlowAsset = nullptr;
			}
		});

		Describe("Basic Import", [this]()
		{
			It("should import a simple flow graph with Start and Finish nodes", [this]()
			{
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"FlowNode_Start\","
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000002\",\"type\":\"FlowNode_Finish\","
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000001\",\"pin\":\"Out\"},"
					"\"target\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000002\",\"pin\":\"In\"}}"
					"]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestImportBasic"),
					TestImportText,
					ErrorMessage
				);

				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset)
				{
					AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage));
					return;
				}

				TestEqual("Asset should have 2 nodes", TestFlowAsset->GetNodes().Num(), 2);
				TestTrue("Asset should be world bound", TestFlowAsset->bWorldBound);

				FGuid StartGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), StartGuid);
				UFlowNode* StartNode = TestFlowAsset->GetNode(StartGuid);
				TestNotNull("Start node should exist", StartNode);

				FGuid FinishGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000002"), FinishGuid);
				UFlowNode* FinishNode = TestFlowAsset->GetNode(FinishGuid);
				TestNotNull("Finish node should exist", FinishNode);

				if (StartNode)
				{
					FConnectedPin Connection = StartNode->GetConnection(FName("Out"));
					TestTrue("Start node should have connection", Connection.NodeGuid.IsValid());
					TestEqual("Connection should point to Finish node", Connection.NodeGuid, FinishGuid);
					TestEqual("Connection should point to In pin", Connection.PinName, FName("In"));
				}
			});

			It("should handle empty import text gracefully", [this]()
			{
				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestImportEmpty"),
					TEXT(""),
					ErrorMessage
				);

				TestNull("Import should fail for empty text", TestFlowAsset);
				TestFalse("Error message should not be empty", ErrorMessage.IsEmpty());
			});

			It("should parse asset metadata correctly", [this]()
			{
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":false,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"FlowNode_Start\","
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]}"
					"]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestImportMetadata"),
					TestImportText,
					ErrorMessage
				);

				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (TestFlowAsset)
				{
					TestFalse("Asset should not be world bound", TestFlowAsset->bWorldBound);
					TestTrue("Asset GUID should be valid", TestFlowAsset->AssetGuid.IsValid());
				}
			});

			It("should default a declared output pin with no explicit type to Exec, not wildcard", [this]()
			{
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"FlowNode_Start\","
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"},{\"name\":\"UntypedEvent\"}]}"
					"]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestImportUntypedPin"),
					TestImportText,
					ErrorMessage
				);

				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset)
				{
					AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage));
					return;
				}

				FGuid StartGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), StartGuid);
				UFlowNode* StartNode = TestFlowAsset->GetNode(StartGuid);
				TestNotNull("Start node should exist", StartNode);
				if (!StartNode)
				{
					return;
				}

				const FFlowPin* UntypedPin = StartNode->GetOutputPins().FindByPredicate([](const FFlowPin& Pin)
				{
					return Pin.PinName == FName("UntypedEvent");
				});
				TestNotNull("Untyped output pin should have been created", UntypedPin);
				if (UntypedPin)
				{
					TestTrue("Untyped output pin on OutputPins should default to Exec", UntypedPin->IsExecPin());
				}
			});
		});

		Describe("Property Import", [this]()
		{
			It("should import array properties with nested structs correctly", [this]()
			{
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":false,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_DefineProperties\","
					"\"properties\":{\"NamedProperties\":\"((Name=\\\"TestProp\\\",DataPinValue=/Script/Flow.FlowDataPinValue_Text(Values=(\\\"Test Value\\\"))))\"},"
					"\"outputPins\":[{\"name\":\"TestProp\",\"type\":\"Text\"}]}]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestImportArrayProperties"),
					TestImportText,
					ErrorMessage
				);

				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset)
				{
					AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage));
					return;
				}

				FGuid DefinePropsGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), DefinePropsGuid);
				UFlowNode* DefinePropsNode = TestFlowAsset->GetNode(DefinePropsGuid);
				TestNotNull("DefineProperties node should exist", DefinePropsNode);

				if (DefinePropsNode)
				{
					// Check that NamedProperties array was imported correctly
					FProperty* NamedPropsProperty = DefinePropsNode->GetClass()->FindPropertyByName(TEXT("NamedProperties"));
					TestNotNull("NamedProperties property should exist", NamedPropsProperty);
					
					if (NamedPropsProperty)
					{
						FArrayProperty* ArrayProperty = CastField<FArrayProperty>(NamedPropsProperty);
						TestNotNull("NamedProperties should be an array property", ArrayProperty);
						
						if (ArrayProperty)
						{
							void* ArrayPtr = ArrayProperty->ContainerPtrToValuePtr<void>(DefinePropsNode);
							FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayPtr);
							
							TestEqual("NamedProperties should have 1 element", ArrayHelper.Num(), 1);
							
							if (ArrayHelper.Num() > 0)
							{
								// Verify the struct has the correct PropertyName
								FStructProperty* InnerStructProp = CastField<FStructProperty>(ArrayProperty->Inner);
								TestNotNull("Inner property should be a struct", InnerStructProp);
								
								if (InnerStructProp)
								{
									void* StructPtr = ArrayHelper.GetRawPtr(0);
									
									// Check Name field
									FProperty* NameFieldProp = InnerStructProp->Struct->FindPropertyByName(TEXT("Name"));
									TestNotNull("Name field should exist", NameFieldProp);
									
									if (NameFieldProp)
									{
										FNameProperty* NameProp = CastField<FNameProperty>(NameFieldProp);
										TestNotNull("Name should be a Name property", NameProp);
										
										if (NameProp)
										{
											FName NameValue = NameProp->GetPropertyValue_InContainer(StructPtr);
											TestEqual("Name should be 'TestProp'", NameValue.ToString(), FString(TEXT("TestProp")));
											TestFalse("Name should not be empty", NameValue.IsNone());
										}
									}
									
									// Check DataPinValue field
									FProperty* DataPinValueProp = InnerStructProp->Struct->FindPropertyByName(TEXT("DataPinValue"));
									TestNotNull("DataPinValue field should exist", DataPinValueProp);
									
									if (DataPinValueProp)
									{
										FStructProperty* DataPinValueStructProp = CastField<FStructProperty>(DataPinValueProp);
										TestNotNull("DataPinValue should be a struct property", DataPinValueStructProp);
										
										if (DataPinValueStructProp)
										{
											// Get the TInstancedStruct value
											void* DataPinValuePtr = DataPinValueStructProp->ContainerPtrToValuePtr<void>(StructPtr);
											
											// Export to text to see what we got
											FString DataPinValueExport;
											DataPinValueStructProp->ExportText_Direct(DataPinValueExport, DataPinValuePtr, nullptr, nullptr, PPF_None);
											UE_LOG(LogFlow, Log, TEXT("DataPinValue exported as: %s"), *DataPinValueExport);
											
											// Check if it's not None
											TestNotEqual("DataPinValue should not be None", DataPinValueExport, FString(TEXT("None")));
										}
									}
									
									// Log the struct for debugging
									FString StructDebugString;
									InnerStructProp->Struct->ExportText(StructDebugString, StructPtr, nullptr, nullptr, PPF_None, nullptr);
									UE_LOG(LogFlow, Log, TEXT("NamedProperties[0] exported as: %s"), *StructDebugString);
								}
							}
						}
					}
				}
			});

			It("should import float properties correctly", [this]()
			{
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":false,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Timer\","
					"\"properties\":{\"CompletionTime\":\"5.0\",\"StepTime\":\"1.0\"},"
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}],"
					"\"outputPins\":[{\"name\":\"Completed\",\"type\":\"Exec\"},{\"name\":\"Step\",\"type\":\"Exec\"}]}]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestImportProperties"),
					TestImportText,
					ErrorMessage
				);

				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset)
				{
					return;
				}

				FGuid TimerGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), TimerGuid);
				UFlowNode* TimerNode = TestFlowAsset->GetNode(TimerGuid);
				TestNotNull("Timer node should exist", TimerNode);

				if (TimerNode)
				{
					FProperty* CompletionTimeProperty = TimerNode->GetClass()->FindPropertyByName(TEXT("CompletionTime"));
					if (CompletionTimeProperty)
					{
						FFloatProperty* FloatProperty = CastField<FFloatProperty>(CompletionTimeProperty);
						if (FloatProperty)
						{
							float Value = FloatProperty->GetPropertyValue_InContainer(TimerNode);
							TestEqual("CompletionTime should be 5.0", Value, 5.0f);
						}
					}

					FProperty* StepTimeProperty = TimerNode->GetClass()->FindPropertyByName(TEXT("StepTime"));
					if (StepTimeProperty)
					{
						FFloatProperty* FloatProperty = CastField<FFloatProperty>(StepTimeProperty);
						if (FloatProperty)
						{
							float Value = FloatProperty->GetPropertyValue_InContainer(TimerNode);
							TestEqual("StepTime should be 1.0", Value, 1.0f);
						}
					}
				}
			});

			It("should import FlowDataPinValue_Text struct properties correctly", [this]()
			{
				// Uses FlowNode_DefineProperties (native, agnostic) with a FlowDataPinValue_Text
				// struct property. The distinct assertion here is that the quoted string content
				// survives the import byte-for-byte, not just the structural shape.
				const FString PhaseInstructionsText = TEXT("The Avarosans need your aid! Seek out the ancient Frostguard watchtower.");
				TestImportText = FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":false,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_DefineProperties\","
					"\"properties\":{\"NamedProperties\":\"((Name=\\\"PhaseInstructions\\\",DataPinValue=/Script/Flow.FlowDataPinValue_Text(Values=(\\\"%s\\\"))))\"},"
					"\"outputPins\":[{\"name\":\"PhaseInstructions\",\"type\":\"Text\"}]}]}"
				), *PhaseInstructionsText);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestImportTextProperties"),
					TestImportText,
					ErrorMessage
				);

				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset)
				{
					return;
				}

				FGuid NodeGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), NodeGuid);
				UFlowNode* Node = TestFlowAsset->GetNode(NodeGuid);
				TestNotNull("Node should exist", Node);

				if (Node)
				{
					FProperty* NamedPropsProperty = Node->GetClass()->FindPropertyByName(TEXT("NamedProperties"));
					FArrayProperty* ArrayProperty = CastField<FArrayProperty>(NamedPropsProperty);
					TestNotNull("NamedProperties should be an array property", ArrayProperty);
					if (!ArrayProperty) { return; }

					void* ArrayPtr = ArrayProperty->ContainerPtrToValuePtr<void>(Node);
					FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayPtr);
					TestEqual("NamedProperties should have 1 element", ArrayHelper.Num(), 1);
					if (ArrayHelper.Num() == 0) { return; }

					FStructProperty* InnerStructProp = CastField<FStructProperty>(ArrayProperty->Inner);
					TestNotNull("Inner property should be a struct", InnerStructProp);
					if (!InnerStructProp) { return; }

					void* NamedPropStructPtr = ArrayHelper.GetRawPtr(0);
					FProperty* DataPinValueProp = InnerStructProp->Struct->FindPropertyByName(TEXT("DataPinValue"));
					TestNotNull("DataPinValue field should exist", DataPinValueProp);
					if (!DataPinValueProp) { return; }

					// DataPinValue is a TInstancedStruct<FFlowDataPinValue> wrapping the
					// FFlowDataPinValue_Text - export to text (same proven pattern as the "should
					// import array properties with nested structs correctly" test above) rather
					// than assume TInstancedStruct's raw memory layout, and assert the exact quoted
					// text content round-tripped, not just that a value is present.
					FStructProperty* DataPinValueStructProp = CastField<FStructProperty>(DataPinValueProp);
					TestNotNull("DataPinValue should be a struct property", DataPinValueStructProp);
					if (!DataPinValueStructProp) { return; }

					void* DataPinValuePtr = DataPinValueStructProp->ContainerPtrToValuePtr<void>(NamedPropStructPtr);
					FString DataPinValueExport;
					DataPinValueStructProp->ExportText_Direct(DataPinValueExport, DataPinValuePtr, nullptr, nullptr, PPF_None);

					TestNotEqual("DataPinValue should not be None", DataPinValueExport, FString(TEXT("None")));
					TestTrue(FString::Printf(TEXT("Exported DataPinValue should contain the exact PhaseInstructions text (got: %s)"), *DataPinValueExport),
						DataPinValueExport.Contains(PhaseInstructionsText));
				}
			});
		});

		// UFlowNode_ExecuteComponent::ComponentTemplate (UPROPERTY(EditAnywhere, Instanced) TObjectPtr<UActorComponent>)
		// is the fixture: a shipped, non-abstract Object property whose class carries CPF_InstancedReference,
		// exactly the shape SetPropertyFromString's new "ClassPath(Field=Value,...)" branch targets. UBillboardComponent
		// is the payload class - already editinlinenew, with plain scalar fields (ScreenSize, bIsScreenSizeScaled) and
		// no asset dependency, so a fixture value never needs a texture reference. Property flags only, not node
		// runtime behavior, are what this Describe block proves; ComponentSource/ComponentClass are left at defaults.
		Describe("Instanced Subobject Properties", [this]()
		{
			It("should construct an instanced subobject from ClassPath(Field=Value) and apply its fields", [this]()
			{
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":false,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_ExecuteComponent\","
					"\"properties\":{\"ComponentTemplate\":\"/Script/Engine.BillboardComponent(ScreenSize=2.500000,bIsScreenSizeScaled=true)\"}}]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(TEXT("/Game/Test/TestInstancedSubobjectConstruct"), TestImportText, ErrorMessage);
				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset)
				{
					AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage));
					return;
				}

				FGuid NodeGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), NodeGuid);
				UFlowNode* Node = TestFlowAsset->GetNode(NodeGuid);
				TestNotNull("ExecuteComponent node should exist", Node);
				if (!Node) { return; }

				FProperty* TemplateProperty = Node->GetClass()->FindPropertyByName(TEXT("ComponentTemplate"));
				FObjectProperty* ObjectProperty = CastField<FObjectProperty>(TemplateProperty);
				TestNotNull("ComponentTemplate should be an object property", ObjectProperty);
				if (!ObjectProperty) { return; }

				UObject* Subobject = ObjectProperty->GetObjectPropertyValue(ObjectProperty->ContainerPtrToValuePtr<void>(Node));
				TestNotNull("ComponentTemplate should have been constructed", Subobject);
				if (!Subobject) { return; }

				TestEqual("Subobject class should be the requested class", Subobject->GetClass(), UBillboardComponent::StaticClass());
				TestEqual("Subobject's Outer should be the owning node, not the asset or package",
					Subobject->GetOuter(), Cast<UObject>(Node));

				UBillboardComponent* Billboard = Cast<UBillboardComponent>(Subobject);
				TestNotNull("Subobject should cast to UBillboardComponent", Billboard);
				if (Billboard)
				{
					TestEqual("ScreenSize field should have been applied", Billboard->ScreenSize, 2.5f);
					TestTrue("bIsScreenSizeScaled field should have been applied", Billboard->bIsScreenSizeScaled != 0);
				}
			});

			It("should apply a nested-struct field without splitting on the field's own inner comma", [this]()
			{
				// U/UL/V are plain floats on UBillboardComponent; the point here is not the values
				// themselves but that a value block containing a parenthesized sub-value with its
				// own comma is not mistaken for two top-level fields. FVector2D exercises this
				// delimiter case using an engine type available in this module.
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":false,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_ExecuteComponent\","
					"\"properties\":{\"ComponentTemplate\":\"/Script/Engine.BillboardComponent(ScreenSize=3.000000,RelativeScale3D=(X=1.000000,Y=2.000000,Z=3.000000))\"}}]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(TEXT("/Game/Test/TestInstancedSubobjectNestedField"), TestImportText, ErrorMessage);
				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset)
				{
					AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage));
					return;
				}

				FGuid NodeGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), NodeGuid);
				UFlowNode* Node = TestFlowAsset->GetNode(NodeGuid);
				TestNotNull("ExecuteComponent node should exist", Node);
				if (!Node) { return; }

				FProperty* TemplateProperty = Node->GetClass()->FindPropertyByName(TEXT("ComponentTemplate"));
				FObjectProperty* ObjectProperty = CastField<FObjectProperty>(TemplateProperty);
				UObject* Subobject = ObjectProperty ? ObjectProperty->GetObjectPropertyValue(ObjectProperty->ContainerPtrToValuePtr<void>(Node)) : nullptr;
				UBillboardComponent* Billboard = Cast<UBillboardComponent>(Subobject);
				TestNotNull("Subobject should cast to UBillboardComponent", Billboard);
				if (!Billboard) { return; }

				TestEqual("The field before the nested struct should still apply", Billboard->ScreenSize, 3.0f);
				TestEqual("Nested struct field X should apply despite its own inner comma", Billboard->GetRelativeScale3D().X, 1.0);
				TestEqual("Nested struct field Y should apply despite its own inner comma", Billboard->GetRelativeScale3D().Y, 2.0);
				TestEqual("Nested struct field Z should apply despite its own inner comma", Billboard->GetRelativeScale3D().Z, 3.0);
			});

			It("rejects an unresolvable bare object path", [this]()
			{
				// A bare value without class-and-fields syntax resolves as an object path. "None"
				// has no object to load, so import reports the load failure.
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":false,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_ExecuteComponent\","
					"\"properties\":{\"ComponentTemplate\":\"None\"}}]}"
				);

				AddExpectedError(TEXT("Failed to load object"), EAutomationExpectedErrorFlags::Contains, 1);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(TEXT("/Game/Test/TestInstancedBarePath"), TestImportText, ErrorMessage);
				TestNull("A bare unresolvable path fails import", TestFlowAsset);
				TestTrue("The error describes the object load failure",
					ErrorMessage.Contains(TEXT("Failed to load object")));
			});

			It("resolves a bare class path for a non-instanced property", [this]()
			{
				// ComponentClass is a TSubclassOf<UActorComponent> property. Its bare class path
				// resolves to the engine's UBillboardComponent class.
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":false,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_ExecuteComponent\","
					"\"properties\":{\"ComponentClass\":\"/Script/Engine.BillboardComponent\"}}]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(TEXT("/Game/Test/TestNonInstancedClassProperty"), TestImportText, ErrorMessage);
				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset)
				{
					AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage));
					return;
				}

				FGuid NodeGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), NodeGuid);
				UFlowNode* Node = TestFlowAsset->GetNode(NodeGuid);
				TestNotNull("ExecuteComponent node should exist", Node);
				if (!Node) { return; }

				FProperty* ClassProperty = Node->GetClass()->FindPropertyByName(TEXT("ComponentClass"));
				FClassProperty* ClassProp = CastField<FClassProperty>(ClassProperty);
				TestNotNull("ComponentClass should be a class property", ClassProp);
				if (!ClassProp) { return; }

				UClass* ResolvedClass = Cast<UClass>(ClassProp->GetObjectPropertyValue(ClassProp->ContainerPtrToValuePtr<void>(Node)));
				TestEqual("ComponentClass should resolve to the requested class, unaffected by this feature",
					ResolvedClass, UBillboardComponent::StaticClass());
			});

			It("should fail with a clear error when the requested class cannot be loaded", [this]()
			{
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":false,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_ExecuteComponent\","
					"\"properties\":{\"ComponentTemplate\":\"/Script/Engine.ThisClassDoesNotExist(ScreenSize=1.000000)\"}}]}"
				);

				AddExpectedError(TEXT("Failed to load class"), EAutomationExpectedErrorFlags::Contains, 1);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(TEXT("/Game/Test/TestInstancedUnresolvedClass"), TestImportText, ErrorMessage);
				TestNull("Import should fail when the subobject class does not resolve", TestFlowAsset);
				TestFalse("An error message should be reported", ErrorMessage.IsEmpty());
			});

			It("should export an owned subobject as ClassPath(Fields) and re-import it into a second asset", [this]()
			{
				// An owned subobject is serialized by class and fields so it can be reconstructed
				// under a different asset owner.
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":false,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_ExecuteComponent\","
					"\"properties\":{\"ComponentTemplate\":\"/Script/Engine.BillboardComponent(ScreenSize=4.000000)\"}}]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(TEXT("/Game/Test/TestInstancedExportSource"), TestImportText, ErrorMessage);
				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset)
				{
					AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage));
					return;
				}

				const FString ExportedText = UFlowGraphExporter::ExportFlowGraphToString(TestFlowAsset);
				TestTrue("Exported document should describe the subobject as ClassPath(Fields), not a bare path into the source asset",
					ExportedText.Contains(TEXT("/Script/Engine.BillboardComponent(")));
				TestFalse("Exported document should not reference the source asset's own package path for the subobject",
					ExportedText.Contains(TEXT("TestInstancedExportSource:")));

				UFlowAsset* ReimportedAsset = ImportFromExportedJson(TEXT("/Game/Test/TestInstancedExportTarget"), ExportedText, ErrorMessage);
				TestNotNull("Re-import into a different asset should succeed", ReimportedAsset);
				if (!ReimportedAsset)
				{
					AddError(FString::Printf(TEXT("Re-import failed: %s"), *ErrorMessage));
					return;
				}

				FGuid NodeGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), NodeGuid);
				UFlowNode* ReimportedNode = ReimportedAsset->GetNode(NodeGuid);
				TestNotNull("Node should exist in the re-imported asset", ReimportedNode);
				if (ReimportedNode)
				{
					FProperty* TemplateProperty = ReimportedNode->GetClass()->FindPropertyByName(TEXT("ComponentTemplate"));
					FObjectProperty* ObjectProperty = CastField<FObjectProperty>(TemplateProperty);
					UObject* Subobject = ObjectProperty ? ObjectProperty->GetObjectPropertyValue(ObjectProperty->ContainerPtrToValuePtr<void>(ReimportedNode)) : nullptr;
					UBillboardComponent* Billboard = Cast<UBillboardComponent>(Subobject);
					TestNotNull("Re-imported subobject should cast to UBillboardComponent", Billboard);
					if (Billboard)
					{
						TestEqual("Re-imported subobject's Outer should be the node in the NEW asset, not the original",
							Billboard->GetOuter(), Cast<UObject>(ReimportedNode));
						TestEqual("Re-imported subobject's field value should match the original", Billboard->ScreenSize, 4.0f);
					}
				}

				ReimportedAsset->ClearFlags(RF_Standalone);
				ReimportedAsset->MarkAsGarbage();
			});
		});

		Describe("Export-Import Roundtrip", [this]()
		{
			It("should successfully roundtrip a test FlowAsset", [this]()
			{
				// Optional project-specific fixture, same as FlowGraphValidation.spec.cpp's
				// RealAssetPaths - a plain informational skip, not a warning, when it isn't present
				// in this project.
				UFlowAsset* OriginalAsset = LoadObject<UFlowAsset>(nullptr, TEXT("/LbExpeditions/EncounterDefinitions/Tests/FA_ZTest_FlowGraph.FA_ZTest_FlowGraph"));
				if (!OriginalAsset)
				{
					AddInfo(TEXT("Could not load test asset - skipping roundtrip test"));
					return;
				}

				FString ExportedText = UFlowGraphExporter::ExportFlowGraphToString(OriginalAsset);
				TestFalse("Exported text should not be empty", ExportedText.IsEmpty());

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(TEXT("/Game/Test/TestImportRoundtrip"), ExportedText, ErrorMessage);

				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset)
				{
					AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage));
					return;
				}

				TestEqual("Node count should match", TestFlowAsset->GetNodes().Num(), OriginalAsset->GetNodes().Num());

				for (const TPair<FGuid, UFlowNode*>& OriginalNodePair : OriginalAsset->GetNodes())
				{
					UFlowNode* OriginalNode = OriginalNodePair.Value;
					UFlowNode* ImportedNode = TestFlowAsset->GetNode(OriginalNodePair.Key);

					TestNotNull(FString::Printf(TEXT("Imported node %s should exist"), *OriginalNodePair.Key.ToString()), ImportedNode);
					if (ImportedNode)
					{
						TestEqual(FString::Printf(TEXT("Node %s type should match"), *OriginalNodePair.Key.ToString()),
							ImportedNode->GetClass(), OriginalNode->GetClass());
					}
				}

				// Verify connections match by checking each original connection exists in imported asset
				for (const TPair<FGuid, UFlowNode*>& OriginalNodePair : OriginalAsset->GetNodes())
				{
					UFlowNode* OriginalNode = OriginalNodePair.Value;
					UFlowNode* ImportedNode = TestFlowAsset->GetNode(OriginalNodePair.Key);

					if (!ImportedNode)
					{
						continue;
					}

					// Check all output connections (exec and data)
					for (const FFlowPin& OutputPin : OriginalNode->GetOutputPins())
					{
						FConnectedPin OriginalConnection = OriginalNode->GetConnection(OutputPin.PinName);
						if (OriginalConnection.NodeGuid.IsValid())
						{
							FConnectedPin ImportedConnection = ImportedNode->GetConnection(OutputPin.PinName);
							TestTrue(FString::Printf(TEXT("Node %s output pin %s should have connection"), 
								*OriginalNodePair.Key.ToString(), *OutputPin.PinName.ToString()),
								ImportedConnection.NodeGuid.IsValid());

							if (ImportedConnection.NodeGuid.IsValid())
							{
								TestEqual(FString::Printf(TEXT("Node %s output pin %s connection target should match"),
									*OriginalNodePair.Key.ToString(), *OutputPin.PinName.ToString()),
									ImportedConnection.NodeGuid, OriginalConnection.NodeGuid);

								TestEqual(FString::Printf(TEXT("Node %s output pin %s connection pin should match"),
									*OriginalNodePair.Key.ToString(), *OutputPin.PinName.ToString()),
									ImportedConnection.PinName, OriginalConnection.PinName);
							}
						}
					}

					// Check all data input connections
					for (const FFlowPin& InputPin : OriginalNode->GetInputPins())
					{
						if (!InputPin.IsExecPin())
						{
							FConnectedPin OriginalConnection = OriginalNode->GetConnection(InputPin.PinName);
							if (OriginalConnection.NodeGuid.IsValid())
							{
								FConnectedPin ImportedConnection = ImportedNode->GetConnection(InputPin.PinName);
								TestTrue(FString::Printf(TEXT("Node %s input pin %s should have connection"), 
									*OriginalNodePair.Key.ToString(), *InputPin.PinName.ToString()),
									ImportedConnection.NodeGuid.IsValid());

								if (ImportedConnection.NodeGuid.IsValid())
								{
									TestEqual(FString::Printf(TEXT("Node %s input pin %s connection source should match"),
										*OriginalNodePair.Key.ToString(), *InputPin.PinName.ToString()),
										ImportedConnection.NodeGuid, OriginalConnection.NodeGuid);

									TestEqual(FString::Printf(TEXT("Node %s input pin %s connection pin should match"),
										*OriginalNodePair.Key.ToString(), *InputPin.PinName.ToString()),
										ImportedConnection.PinName, OriginalConnection.PinName);
								}
							}
						}
					}
				}
			});

			It("should create editor graph nodes after import using Regrapher", [this]()
			{
				// Log the editor state for debugging
				UE_LOG(LogFlow, Log, TEXT("Regrapher test: GEditor=%p, IsRunningCommandlet=%d, IsUnattended=%d"),
					(void*)GEditor, IsRunningCommandlet(), FApp::IsUnattended());
				
				if (GEditor)
				{
					UE_LOG(LogFlow, Log, TEXT("Regrapher test: GEditor->PlayWorld=%p"), (void*)GEditor->PlayWorld.Get());
				}

				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000002\",\"type\":\"/Script/Flow.FlowNode_Finish\","
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000001\",\"pin\":\"Out\"},"
					"\"target\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000002\",\"pin\":\"In\"}}"
					"]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestImportWithRegrapher"),
					TestImportText,
					ErrorMessage
				);

				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset)
				{
					return;
				}

				bool bRegraphSuccess = UFlowGraphRegrapher::RegraphFlowAsset(TestFlowAsset);
				TestTrue("Regrapher should succeed", bRegraphSuccess);

				UFlowGraph* FlowGraph = Cast<UFlowGraph>(TestFlowAsset->GetGraph());
				TestNotNull("FlowGraph should exist after regraphing", FlowGraph);

				if (FlowGraph)
				{
					FGuid StartGuid;
					FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), StartGuid);
					UFlowNode* StartNode = TestFlowAsset->GetNode(StartGuid);
					
					if (StartNode)
					{
						UEdGraphNode* GraphNode = StartNode->GetGraphNode();
						TestNotNull("Start node should have a graph node after regraphing", GraphNode);

						if (GraphNode == nullptr)
						{
							return;
						}
						const FString CommentPatch = FString::Printf(
							TEXT("{\"formatVersion\":2,\"mode\":\"Patch\",\"ops\":[")
							TEXT("{\"kind\":\"UpsertNode\",\"guid\":\"%s\",\"comment\":\"Existing node comment\"}]}"),
							*StartGuid.ToString());
						FString ReconcileError;
						const TSharedPtr<FFlowReconcileExecutionPlan> Plan =
							FFlowGraphReconciler::ComputeReconcilePlan(
								TestFlowAsset->GetPathName(), CommentPatch, ReconcileError);
						TestTrue("Comment patch should produce a reconcile plan", Plan.IsValid());
						if (Plan.IsValid())
						{
							const FFlowReconcileResult ReconcileResult =
								FFlowGraphReconciler::ExecuteReconcilePlan(
									*Plan, TestFlowAsset->GetPathName(), ReconcileError);
							TestTrue("Comment patch should apply", ReconcileResult.bSuccess);
						}
						TestEqual("Existing node comment should match", GraphNode->NodeComment,
							FString(TEXT("Existing node comment")));
					}
				}
			});

			It("should import and regraph in one call using ImportAndRegraphFromDocument", [this]()
			{
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000002\",\"type\":\"/Script/Flow.FlowNode_Finish\","
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000001\",\"pin\":\"Out\"},"
					"\"target\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000002\",\"pin\":\"In\"}}"
					"]}"
				);

				UFlowAsset* ImportedAsset = ImportAndRegraphFixtureDocument(
					TestImportText,
					TEXT("/Game/Test/TestImportAndRegraph")
				);

				TestNotNull("Imported and regraphed asset should not be null", ImportedAsset);
				
				if (ImportedAsset)
				{
					// Verify the asset has a graph
					UFlowGraph* FlowGraph = Cast<UFlowGraph>(ImportedAsset->GetGraph());
					TestNotNull("FlowGraph should exist", FlowGraph);

					// Verify nodes have graph nodes
					FGuid StartGuid;
					FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), StartGuid);
					UFlowNode* StartNode = ImportedAsset->GetNode(StartGuid);
					
					if (StartNode)
					{
						UEdGraphNode* GraphNode = StartNode->GetGraphNode();
						TestNotNull("Start node should have a graph node", GraphNode);
						
						if (GraphNode)
						{
							// Verify the graph node has pins
							TestTrue("Graph node should have pins", GraphNode->Pins.Num() > 0);
						}
					}

					FGuid FinishGuid;
					FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000002"), FinishGuid);
					UFlowNode* FinishNode = ImportedAsset->GetNode(FinishGuid);
					
					if (FinishNode)
					{
						UEdGraphNode* GraphNode = FinishNode->GetGraphNode();
						TestNotNull("Finish node should have a graph node", GraphNode);
					}
				}
			});

			It("should properly connect data pins in EdGraphNodes after import and regraph", [this]()
			{
				// Uses FlowNode_FormatText (native, agnostic): it has both a real static output
				// data pin ("Formatted Text", Text) and a property-meta-driven input data pin
				// ("FormatText", Text via DefaultForInputFlowPin), exercising the full node-to-node
				// data pin wiring path without any unsynced Blueprint content.
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_FormatText\","
					"\"outputPins\":[{\"name\":\"Formatted Text\",\"type\":\"Text\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000002\",\"type\":\"/Script/Flow.FlowNode_FormatText\","
					"\"inputPins\":[{\"name\":\"FormatText\",\"type\":\"Text\"}],"
					"\"outputPins\":[{\"name\":\"Formatted Text\",\"type\":\"Text\"}]},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000001\",\"pin\":\"Formatted Text\"},"
					"\"target\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000002\",\"pin\":\"FormatText\"}}"
					"]}"
				);

				UFlowAsset* ImportedAsset = ImportAndRegraphFixtureDocument(
					TestImportText,
					TEXT("/Game/Test/TestDataPinImportAndRegraph")
				);

				TestNotNull("Imported and regraphed asset should not be null", ImportedAsset);
				
				if (ImportedAsset)
				{
					// Verify the asset has a graph
					UFlowGraph* FlowGraph = Cast<UFlowGraph>(ImportedAsset->GetGraph());
					TestNotNull("FlowGraph should exist", FlowGraph);

					// Get the nodes
					FGuid SourceGuid;
					FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), SourceGuid);
					UFlowNode* SourceNode = ImportedAsset->GetNode(SourceGuid);

					FGuid TargetGuid;
					FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000002"), TargetGuid);
					UFlowNode* TargetNode = ImportedAsset->GetNode(TargetGuid);

					TestNotNull("Source FormatText node should exist", SourceNode);
					TestNotNull("Target FormatText node should exist", TargetNode);

					if (SourceNode && TargetNode)
					{
						// Get the EdGraphNodes
						UEdGraphNode* SourceGraphNode = SourceNode->GetGraphNode();
						UEdGraphNode* TargetGraphNode = TargetNode->GetGraphNode();

						TestNotNull("Source node should have a graph node", SourceGraphNode);
						TestNotNull("Target node should have a graph node", TargetGraphNode);

						if (SourceGraphNode && TargetGraphNode)
						{
							// Find the "Formatted Text" output pin on the source node
							UEdGraphPin* TextOutputPin = nullptr;
							for (UEdGraphPin* Pin : SourceGraphNode->Pins)
							{
								if (Pin && Pin->PinName == TEXT("Formatted Text") && Pin->Direction == EGPD_Output)
								{
									TextOutputPin = Pin;
									break;
								}
							}

							// Find the "FormatText" input pin on the target node
							UEdGraphPin* FormatTextInputPin = nullptr;
							for (UEdGraphPin* Pin : TargetGraphNode->Pins)
							{
								if (Pin && Pin->PinName == TEXT("FormatText") && Pin->Direction == EGPD_Input)
								{
									FormatTextInputPin = Pin;
									break;
								}
							}

							TestNotNull("Source node should have a Formatted Text output pin", TextOutputPin);
							TestNotNull("Target node should have a FormatText input pin", FormatTextInputPin);

							if (TextOutputPin && FormatTextInputPin)
							{
								// Verify the data pin connection exists in the EdGraph
								bool bFoundConnection = false;
								for (UEdGraphPin* LinkedPin : TextOutputPin->LinkedTo)
								{
									if (LinkedPin == FormatTextInputPin)
									{
										bFoundConnection = true;
										break;
									}
								}

								TestTrue("Formatted Text output pin should be linked to FormatText input pin", bFoundConnection);

								// Also verify the reverse connection
								bool bFoundReverseConnection = false;
								for (UEdGraphPin* LinkedPin : FormatTextInputPin->LinkedTo)
								{
									if (LinkedPin == TextOutputPin)
									{
										bFoundReverseConnection = true;
										break;
									}
								}

								TestTrue("FormatText input pin should be linked to Formatted Text output pin", bFoundReverseConnection);
							}
						}
					}
				}

			});
		});
		Describe("AddOn Import Export", [this]()
		{
			// Helpers ----------------------------------------------------------
			// Build a minimal single-node flow graph text with one SwitchCase addon.
			// UFlowNodeAddOn_SwitchCase is a concrete, native, non-abstract addon in
			// /Script/Flow - no Blueprint asset or asset registry required.
			auto MakeSwitchCaseAddonText = [](const FString& CaseName) -> FString
			{
				return FString::Printf(TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":false,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertAddon\",\"newAlias\":\"case\",\"parentGuid\":\"00000000-0000-0000-0000-000000000001\","
					"\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"%s\"}}"
					"]}"
				), *CaseName);
			};

			It("should import a node with a single AddOn attached", [this, MakeSwitchCaseAddonText]()
			{
				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestAddOnImportSingle"),
					MakeSwitchCaseAddonText(TEXT("CaseA")),
					ErrorMessage
				);

				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset)
				{
					AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage));
					return;
				}

				FGuid NodeGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), NodeGuid);
				UFlowNode* StartNode = TestFlowAsset->GetNode(NodeGuid);
				TestNotNull("Start node should exist", StartNode);
				if (!StartNode)
				{
					return;
				}

				const TArray<UFlowNodeAddOn*>& AddOns = StartNode->GetFlowNodeAddOnChildren();
				TestEqual("Node should have exactly one AddOn", AddOns.Num(), 1);
				if (AddOns.Num() > 0)
				{
					TestNotNull("AddOn should not be null", AddOns[0]);
					if (AddOns[0])
					{
						TestTrue("AddOn should be a UFlowNodeAddOn_SwitchCase",
							AddOns[0]->IsA(UFlowNodeAddOn_SwitchCase::StaticClass()));
					}
				}
			});

			It("should preserve imported AddOns through regraph and save update", [this, MakeSwitchCaseAddonText]()
			{
				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestAddOnRegraph"),
					MakeSwitchCaseAddonText(TEXT("CaseRegraph")),
					ErrorMessage
				);
				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset)
				{
					return;
				}
				TestTrue("Regraph should succeed", UFlowGraphRegrapher::RegraphFlowAsset(TestFlowAsset));
				UFlowGraph* const FlowGraph = Cast<UFlowGraph>(TestFlowAsset->GetGraph());
				TestNotNull("Editor graph should exist", FlowGraph);
				if (!FlowGraph)
				{
					return;
				}
				FlowGraph->OnSave();
				FGuid NodeGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), NodeGuid);
				UFlowNode* const StartNode = TestFlowAsset->GetNode(NodeGuid);
				TestNotNull("Start node should exist", StartNode);
				if (!StartNode)
				{
					return;
				}
				const TArray<UFlowNodeAddOn*>& AddOns = StartNode->GetFlowNodeAddOnChildren();
				TestEqual("Runtime AddOn should survive save update", AddOns.Num(), 1);
				UFlowGraphNode* EditorNode = nullptr;
				for (UEdGraphNode* const Candidate : FlowGraph->Nodes)
				{
					UFlowGraphNode* const FlowCandidate = Cast<UFlowGraphNode>(Candidate);
					if (FlowCandidate && FlowCandidate->GetFlowNodeBase() == StartNode)
					{
						EditorNode = FlowCandidate;
						break;
					}
				}
				TestNotNull("Editor node should exist", EditorNode);
				if (EditorNode)
				{
					TestEqual("Editor node should have one AddOn subnode", EditorNode->SubNodes.Num(), 1);
					if (EditorNode->SubNodes.Num() == 1 && AddOns.Num() == 1)
					{
						TestEqual("Editor AddOn should mirror runtime AddOn",
							EditorNode->SubNodes[0]->GetFlowNodeBase(), static_cast<UFlowNodeBase*>(AddOns[0]));
						UFlowGraphNode* const OriginalSubNode = EditorNode->SubNodes[0];
						TestTrue("Second regraph should succeed", UFlowGraphRegrapher::RegraphFlowAsset(TestFlowAsset));
						TestEqual("Second regraph should reuse the AddOn subnode", EditorNode->SubNodes.Num(), 1);
						if (EditorNode->SubNodes.Num() == 1)
						{
							TestEqual("Second regraph should preserve subnode identity",
								EditorNode->SubNodes[0].Get(), OriginalSubNode);
						}
						StartNode->GetFlowNodeAddOnChildrenByEditor().Reset();
						TestTrue("Regraph after runtime removal should succeed",
							UFlowGraphRegrapher::RegraphFlowAsset(TestFlowAsset));
						FlowGraph->OnSave();
						TestTrue("Removed runtime AddOn should stay removed",
							StartNode->GetFlowNodeAddOnChildren().IsEmpty());
						TestTrue("Removed AddOn editor subnode should be pruned", EditorNode->SubNodes.IsEmpty());
					}
				}
				TestFlowAsset->GetPackage()->SetDirtyFlag(false);
			});

			It("should preserve a runtime AddOn reparent in either direction", [this]()
			{
				const FString Document = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":false,\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\"},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000002\",\"type\":\"/Script/Flow.FlowNode_Finish\"},"
					"{\"kind\":\"UpsertAddon\",\"newAlias\":\"case\",\"parentGuid\":\"00000000-0000-0000-0000-000000000001\","
					"\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"ReparentCase\"}}]}");
				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(TEXT("/Game/Test/TestAddOnReparent"), Document, ErrorMessage);
				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset || !UFlowGraphRegrapher::RegraphFlowAsset(TestFlowAsset))
				{
					return;
				}
				FGuid StartGuid;
				FGuid FinishGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), StartGuid);
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000002"), FinishGuid);
				UFlowNode* const StartNode = TestFlowAsset->GetNode(StartGuid);
				UFlowNode* const FinishNode = TestFlowAsset->GetNode(FinishGuid);
				if (!StartNode || !FinishNode || StartNode->GetFlowNodeAddOnChildren().Num() != 1)
				{
					AddError(TEXT("Imported reparent fixture is incomplete."));
					return;
				}
				UFlowNodeAddOn* const AddOn = StartNode->GetFlowNodeAddOnChildren()[0];
				auto MoveAndVerify = [this, AddOn](UFlowNode* FromNode, UFlowNode* ToNode, const TCHAR* Label)
				{
					FromNode->GetFlowNodeAddOnChildrenByEditor().Reset();
					ToNode->GetFlowNodeAddOnChildrenByEditor().Add(AddOn);
					TestTrue(FString::Printf(TEXT("%s regraph succeeds"), Label),
						UFlowGraphRegrapher::RegraphFlowAsset(TestFlowAsset));
					CastChecked<UFlowGraph>(TestFlowAsset->GetGraph())->OnSave();
					TestTrue(FString::Printf(TEXT("%s old parent stays empty"), Label),
						FromNode->GetFlowNodeAddOnChildren().IsEmpty());
					TestEqual(FString::Printf(TEXT("%s new parent owns AddOn"), Label),
						ToNode->GetFlowNodeAddOnChildren().Num(), 1);
					const UFlowGraphNode* const AddOnGraphNode = Cast<UFlowGraphNode>(AddOn->GetGraphNode());
					TestTrue(FString::Printf(TEXT("%s editor subnode points at the new parent"), Label),
						IsValid(AddOnGraphNode) && IsValid(AddOnGraphNode->GetParentNode())
							&& AddOnGraphNode->GetParentNode()->GetFlowNodeBase() == ToNode);
				};
				MoveAndVerify(StartNode, FinishNode, TEXT("Forward"));
				MoveAndVerify(FinishNode, StartNode, TEXT("Reverse"));
				TestFlowAsset->GetPackage()->SetDirtyFlag(false);
			});

			It("should reject one runtime AddOn owned by multiple parents before mutation", [this]()
			{
				const FString Document = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":false,\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\"},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000002\",\"type\":\"/Script/Flow.FlowNode_Finish\"},"
					"{\"kind\":\"UpsertAddon\",\"newAlias\":\"case\",\"parentGuid\":\"00000000-0000-0000-0000-000000000001\","
					"\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"SharedCase\"}}]}");
				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(TEXT("/Game/Test/TestAddOnMultipleParents"), Document, ErrorMessage);
				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset || !UFlowGraphRegrapher::RegraphFlowAsset(TestFlowAsset))
				{
					return;
				}
				FGuid StartGuid;
				FGuid FinishGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), StartGuid);
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000002"), FinishGuid);
				UFlowNode* const StartNode = TestFlowAsset->GetNode(StartGuid);
				UFlowNode* const FinishNode = TestFlowAsset->GetNode(FinishGuid);
				if (!StartNode || !FinishNode || StartNode->GetFlowNodeAddOnChildren().Num() != 1)
				{
					AddError(TEXT("Imported multiple-parent fixture is incomplete."));
					return;
				}
				UFlowNodeAddOn* const AddOn = StartNode->GetFlowNodeAddOnChildren()[0];
				UFlowGraphNode* const OriginalWrapper = Cast<UFlowGraphNode>(AddOn->GetGraphNode());
				UFlowGraphNode* const OriginalParent = IsValid(OriginalWrapper)
					? OriginalWrapper->GetParentNode() : nullptr;
				FinishNode->GetFlowNodeAddOnChildrenByEditor().Add(AddOn);
				AddExpectedError(TEXT("has more than one parent"), EAutomationExpectedErrorFlags::Contains, 1);
				TestFalse("Multiple runtime parents should fail regraph",
					UFlowGraphRegrapher::RegraphFlowAsset(TestFlowAsset));
				UFlowGraph* const FlowGraph = Cast<UFlowGraph>(TestFlowAsset->GetGraph());
				TestFalse("Failed preflight should leave graph unlocked", IsValid(FlowGraph) && FlowGraph->IsLocked());
				TestEqual("Failed preflight should preserve wrapper identity",
					Cast<UFlowGraphNode>(AddOn->GetGraphNode()), OriginalWrapper);
				TestEqual("Failed preflight should preserve editor parent",
					IsValid(OriginalWrapper) ? OriginalWrapper->GetParentNode() : nullptr, OriginalParent);
				TestFlowAsset->GetPackage()->SetDirtyFlag(false);
			});

			It("should preflight invalid runtime endpoints before editor mutation", [this]()
			{
				const FString Document = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":false,\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\"},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000002\",\"type\":\"/Script/Flow.FlowNode_Finish\"}]}");
				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(TEXT("/Game/Test/TestRegraphInvalidEndpoint"), Document, ErrorMessage);
				if (!TestFlowAsset || !UFlowGraphRegrapher::RegraphFlowAsset(TestFlowAsset))
				{
					AddError(TEXT("Imported endpoint fixture is incomplete."));
					return;
				}
				FGuid StartGuid;
				FGuid FinishGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), StartGuid);
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000002"), FinishGuid);
				UFlowNode* const StartNode = TestFlowAsset->GetNode(StartGuid);
				UFlowNode* const FinishNode = TestFlowAsset->GetNode(FinishGuid);
				if (!StartNode || !FinishNode || StartNode->GetOutputPins().IsEmpty()
					|| FinishNode->GetInputPins().IsEmpty())
				{
					AddError(TEXT("Endpoint fixture nodes have no default pins."));
					return;
				}
				const FName OutputName = StartNode->GetOutputPins()[0].PinName;
				TMap<FName, FConnectedPin> InvalidConnections;
				InvalidConnections.Add(OutputName, FConnectedPin(FinishGuid, TEXT("MissingInput")));
				StartNode->SetConnections(InvalidConnections);
				AddExpectedError(TEXT("targets missing exec input"), EAutomationExpectedErrorFlags::Contains, 1);
				TestFalse("Missing runtime target pin should fail preflight",
					UFlowGraphRegrapher::RegraphFlowAsset(TestFlowAsset));
				UFlowGraph* const FlowGraph = Cast<UFlowGraph>(TestFlowAsset->GetGraph());
				TestFalse("Failed endpoint preflight should leave graph unlocked",
					IsValid(FlowGraph) && FlowGraph->IsLocked());
				TestEqual("Failed endpoint preflight should preserve runtime target pin",
					StartNode->GetConnection(OutputName).PinName, FName(TEXT("MissingInput")));

				TMap<FName, FConnectedPin> ValidConnections;
				ValidConnections.Add(OutputName,
					FConnectedPin(FinishGuid, FinishNode->GetInputPins()[0].PinName));
				StartNode->SetConnections(ValidConnections);
				TestTrue("Corrected endpoint should regraph", UFlowGraphRegrapher::RegraphFlowAsset(TestFlowAsset));
				TestFlowAsset->GetPackage()->SetDirtyFlag(false);
			});

			It("should preserve AddOn property value through import", [this, MakeSwitchCaseAddonText]()
			{
				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestAddOnImportProperty"),
					MakeSwitchCaseAddonText(TEXT("MyCaseName")),
					ErrorMessage
				);

				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset)
				{
					AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage));
					return;
				}

				FGuid NodeGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), NodeGuid);
				UFlowNode* StartNode = TestFlowAsset->GetNode(NodeGuid);
				if (!StartNode)
				{
					return;
				}

				const TArray<UFlowNodeAddOn*>& AddOns = StartNode->GetFlowNodeAddOnChildren();
				if (AddOns.Num() == 0 || !AddOns[0])
				{
					return;
				}

				UFlowNodeAddOn_SwitchCase* SwitchCase = Cast<UFlowNodeAddOn_SwitchCase>(AddOns[0]);
				TestNotNull("AddOn should cast to UFlowNodeAddOn_SwitchCase", SwitchCase);
				if (SwitchCase)
				{
					TestEqual("CaseName should be preserved", SwitchCase->CaseName, FName(TEXT("MyCaseName")));
				}
			});

			It("should export a node with an AddOn as an AddOn: block", [this]()
			{
				// Build the asset via importer so we have a known addon attached.
				const FString ImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":false,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertAddon\",\"newAlias\":\"case\",\"parentGuid\":\"00000000-0000-0000-0000-000000000001\","
					"\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"ExportedCase\"}}"
					"]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestAddOnExport"),
					ImportText,
					ErrorMessage
				);

				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset)
				{
					AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage));
					return;
				}

				const FString ExportedText = UFlowGraphExporter::ExportFlowGraphToString(TestFlowAsset);

				TestFalse("Exported text should not be empty", ExportedText.IsEmpty());
				TestTrue("Exported text should contain an UpsertAddon op",
					ExportedText.Contains(TEXT("\"kind\": \"UpsertAddon\"")));
				TestTrue("Exported text should contain the SwitchCase class path",
					ExportedText.Contains(TEXT("FlowNodeAddOn_SwitchCase")));
			});

			It("should roundtrip a node with a single AddOn preserving type and property", [this, MakeSwitchCaseAddonText]()
			{
				FString ErrorMessage;
				UFlowAsset* OriginalAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestAddOnRoundtripOrig"),
					MakeSwitchCaseAddonText(TEXT("RoundtripCase")),
					ErrorMessage
				);

				TestNotNull("Original import should succeed", OriginalAsset);
				if (!OriginalAsset)
				{
					AddError(FString::Printf(TEXT("First import failed: %s"), *ErrorMessage));
					return;
				}

				const FString ExportedText = UFlowGraphExporter::ExportFlowGraphToString(OriginalAsset);
				TestFalse("Exported text should not be empty", ExportedText.IsEmpty());

				TestFlowAsset = ImportFromExportedJson(TEXT("/Game/Test/TestAddOnRoundtripReimport"), ExportedText, ErrorMessage);

				// Clean up original (not tracked by TestFlowAsset)
				OriginalAsset->ClearFlags(RF_Standalone);
				OriginalAsset->MarkAsGarbage();

				TestNotNull("Re-imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset)
				{
					AddError(FString::Printf(TEXT("Re-import failed: %s"), *ErrorMessage));
					return;
				}

				FGuid NodeGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), NodeGuid);
				UFlowNode* StartNode = TestFlowAsset->GetNode(NodeGuid);
				TestNotNull("Start node should survive roundtrip", StartNode);
				if (!StartNode)
				{
					return;
				}

				const TArray<UFlowNodeAddOn*>& AddOns = StartNode->GetFlowNodeAddOnChildren();
				TestEqual("Addon count should be 1 after roundtrip", AddOns.Num(), 1);
				if (AddOns.Num() > 0 && AddOns[0])
				{
					UFlowNodeAddOn_SwitchCase* SwitchCase = Cast<UFlowNodeAddOn_SwitchCase>(AddOns[0]);
					TestNotNull("Re-imported AddOn should cast to UFlowNodeAddOn_SwitchCase", SwitchCase);
					if (SwitchCase)
					{
						TestEqual("CaseName should survive roundtrip", SwitchCase->CaseName, FName(TEXT("RoundtripCase")));
					}
				}
			});

			It("should roundtrip multiple AddOns on the same node preserving order", [this]()
			{
				const FString ImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":false,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertAddon\",\"newAlias\":\"caseAlpha\",\"parentGuid\":\"00000000-0000-0000-0000-000000000001\","
					"\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"Alpha\"}},"
					"{\"kind\":\"UpsertAddon\",\"newAlias\":\"caseBeta\",\"parentGuid\":\"00000000-0000-0000-0000-000000000001\","
					"\"type\":\"/Script/Flow.FlowNodeAddOn_SwitchCase\",\"properties\":{\"CaseName\":\"Beta\"}}"
					"]}"
				);

				FString ErrorMessage;
				UFlowAsset* OriginalAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestMultiAddOnOrig"),
					ImportText,
					ErrorMessage
				);

				TestNotNull("Original import should succeed", OriginalAsset);
				if (!OriginalAsset)
				{
					AddError(FString::Printf(TEXT("First import failed: %s"), *ErrorMessage));
					return;
				}

				// Verify two addons were attached before roundtrip
				FGuid NodeGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), NodeGuid);
				UFlowNode* StartNode = OriginalAsset->GetNode(NodeGuid);
				TestNotNull("Start node should exist after first import", StartNode);

				if (StartNode)
				{
					TestEqual("First import should have 2 AddOns", StartNode->GetFlowNodeAddOnChildren().Num(), 2);
				}

				const FString ExportedText = UFlowGraphExporter::ExportFlowGraphToString(OriginalAsset);

				OriginalAsset->ClearFlags(RF_Standalone);
				OriginalAsset->MarkAsGarbage();

				TestFlowAsset = ImportFromExportedJson(TEXT("/Game/Test/TestMultiAddOnReimport"), ExportedText, ErrorMessage);

				TestNotNull("Re-imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset)
				{
					AddError(FString::Printf(TEXT("Re-import failed: %s"), *ErrorMessage));
					return;
				}

				StartNode = TestFlowAsset->GetNode(NodeGuid);
				TestNotNull("Start node should survive roundtrip", StartNode);
				if (!StartNode)
				{
					return;
				}

				const TArray<UFlowNodeAddOn*>& AddOns = StartNode->GetFlowNodeAddOnChildren();
				TestEqual("Both AddOns should survive roundtrip", AddOns.Num(), 2);

				if (AddOns.Num() >= 2)
				{
					UFlowNodeAddOn_SwitchCase* Case0 = Cast<UFlowNodeAddOn_SwitchCase>(AddOns[0]);
					UFlowNodeAddOn_SwitchCase* Case1 = Cast<UFlowNodeAddOn_SwitchCase>(AddOns[1]);

					TestNotNull("First re-imported AddOn should cast to SwitchCase", Case0);
					TestNotNull("Second re-imported AddOn should cast to SwitchCase", Case1);

					if (Case0)
					{
						TestEqual("First AddOn CaseName should be Alpha", Case0->CaseName, FName(TEXT("Alpha")));
					}
					if (Case1)
					{
						TestEqual("Second AddOn CaseName should be Beta", Case1->CaseName, FName(TEXT("Beta")));
					}
				}
			});

			It("should fail import gracefully when AddOn class path is unknown", [this]()
			{
				const FString ImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":false,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertAddon\",\"newAlias\":\"case\",\"parentGuid\":\"00000000-0000-0000-0000-000000000001\","
					"\"type\":\"/Script/Flow.FlowNodeAddOn_DoesNotExist\"}"
					"]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestAddOnUnknownClass"),
					ImportText,
					ErrorMessage
				);

				TestNull("Import should fail when AddOn class is unknown", TestFlowAsset);
				TestFalse("Error message should not be empty on unknown AddOn class", ErrorMessage.IsEmpty());
			});
		});

		Describe("Dynamic Pin Resolution", [this]()
		{
			It("should wire declared [Exec] output pins beyond CDO defaults as exec connections", [this]()
			{
				// ExecutionSequence declares pins beyond the CDO's default 0 and 1. They retain
				// their Exec type so connections are stored on the source node.
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000002\",\"type\":\"/Script/Flow.FlowNode_ExecutionSequence\","
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}],"
					"\"outputPins\":[{\"name\":\"0\",\"type\":\"Exec\"},{\"name\":\"1\",\"type\":\"Exec\"},{\"name\":\"2\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000003\",\"type\":\"/Script/Flow.FlowNode_Finish\","
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000004\",\"type\":\"/Script/Flow.FlowNode_Finish\","
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000005\",\"type\":\"/Script/Flow.FlowNode_Finish\","
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000001\",\"pin\":\"Out\"},"
					"\"target\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000002\",\"pin\":\"In\"}},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000002\",\"pin\":\"0\"},"
					"\"target\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000003\",\"pin\":\"In\"}},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000002\",\"pin\":\"1\"},"
					"\"target\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000004\",\"pin\":\"In\"}},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000002\",\"pin\":\"2\"},"
					"\"target\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000005\",\"pin\":\"In\"}}"
					"]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestDynamicExecPin"),
					TestImportText,
					ErrorMessage
				);

				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset)
				{
					AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage));
					return;
				}

				FGuid SeqGuid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000002"), SeqGuid);
				UFlowNode* SeqNode = TestFlowAsset->GetNode(SeqGuid);
				TestNotNull("ExecutionSequence node should exist", SeqNode);
				if (!SeqNode)
				{
					return;
				}

				FGuid Finish3Guid;
				FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000005"), Finish3Guid);

				// Pin 2 connection must be stored on the source (SeqNode), not the target,
				// which is only the case when the importer correctly treats it as an exec pin.
				FConnectedPin Pin2Connection = SeqNode->GetConnection(FName("2"));
				TestTrue("ExecutionSequence pin 2 should have a valid connection", Pin2Connection.NodeGuid.IsValid());
				if (Pin2Connection.NodeGuid.IsValid())
				{
					TestEqual("ExecutionSequence pin 2 should connect to Finish node 5", Pin2Connection.NodeGuid, Finish3Guid);
					TestEqual("ExecutionSequence pin 2 connection target pin should be 'In'", Pin2Connection.PinName, FName("In"));
				}
			});
		});

		// Declared pins retain their names and types through import.
		Describe("Declared pin reconstruction", [this]()
		{
			It("ParsePinDecl parses name, type, and sub-category path correctly", [this]()
			{
				FName Name; FString Type; FString SubCat;

				TestTrue("'Value [Vector]' parses", UFlowGraphImporter::ParsePinDecl(TEXT("Value [Vector]"), Name, Type, SubCat));
				TestEqual("name is Value", Name, FName("Value"));
				TestEqual("type is Vector", Type, FString("Vector"));
				TestTrue("no sub-category for plain type", SubCat.IsEmpty());

				TestTrue("'Out [Exec]' parses", UFlowGraphImporter::ParsePinDecl(TEXT("Out [Exec]"), Name, Type, SubCat));
				TestEqual("name is Out", Name, FName("Out"));
				TestEqual("type is Exec", Type, FString("Exec"));
				TestTrue("no sub-category for Exec", SubCat.IsEmpty());

				// A typed struct pin carries a sub-category path.
				TestTrue("'Pos [Struct:/Script/CoreUObject.Vector]' parses", UFlowGraphImporter::ParsePinDecl(TEXT("Pos [Struct:/Script/CoreUObject.Vector]"), Name, Type, SubCat));
				TestEqual("name is Pos", Name, FName("Pos"));
				TestEqual("type is Struct", Type, FString("Struct"));
				TestEqual("sub-category path", SubCat, FString("/Script/CoreUObject.Vector"));

				TestTrue("bare name parses", UFlowGraphImporter::ParsePinDecl(TEXT("MyPin"), Name, Type, SubCat));
				TestEqual("bare name is MyPin", Name, FName("MyPin"));
				TestTrue("bare name has empty type", Type.IsEmpty());
				TestTrue("bare name has empty sub-category", SubCat.IsEmpty());

				TestFalse("empty string does not parse", UFlowGraphImporter::ParsePinDecl(TEXT(""), Name, Type, SubCat));
			});

			It("declared unconnected data pin is reconstructed on import", [this]()
			{
				// An unconnected declared data output remains on the imported node.
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"},{\"name\":\"DataOut\",\"type\":\"Vector\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000002\",\"type\":\"/Script/Flow.FlowNode_Finish\","
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"},{\"name\":\"DataIn\",\"type\":\"Int\"}]},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000001\",\"pin\":\"Out\"},"
					"\"target\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000002\",\"pin\":\"In\"}}"
					"]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestDeclaredPinRecon"),
					TestImportText,
					ErrorMessage
				);

				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset) { AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage)); return; }

				FGuid StartGuid; FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), StartGuid);
				FGuid FinishGuid; FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000002"), FinishGuid);
				UFlowNode* StartNode  = TestFlowAsset->GetNode(StartGuid);
				UFlowNode* FinishNode = TestFlowAsset->GetNode(FinishGuid);
				TestNotNull("Start node must exist", StartNode);
				TestNotNull("Finish node must exist", FinishNode);
				if (!StartNode || !FinishNode) { return; }

				// DataOut on Start - unconnected declared data pin
				bool bDataOutFound = false;
				FName DataOutTypeName;
				for (const FFlowPin& Pin : StartNode->GetOutputPins())
				{
					if (Pin.PinName == FName("DataOut"))
					{
						bDataOutFound = true;
						DataOutTypeName = Pin.GetPinTypeName().Name;
						break;
					}
				}
				TestTrue("Start.DataOut pin must be present after import", bDataOutFound);
				TestEqual("Start.DataOut pin type must be Vector", DataOutTypeName, FName("Vector"));

				// DataIn on Finish - unconnected declared data pin
				bool bDataInFound = false;
				FName DataInTypeName;
				for (const FFlowPin& Pin : FinishNode->GetInputPins())
				{
					if (Pin.PinName == FName("DataIn"))
					{
						bDataInFound = true;
						DataInTypeName = Pin.GetPinTypeName().Name;
						break;
					}
				}
				TestTrue("Finish.DataIn pin must be present after import", bDataInFound);
				TestEqual("Finish.DataIn pin type must be Int", DataInTypeName, FName("Int"));
			});

			It("connected data pin retains declared type (not wildcard)", [this]()
			{
				// A connected declared data pin retains its type.
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"},{\"name\":\"Score\",\"type\":\"Float\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000002\",\"type\":\"/Script/Flow.FlowNode_Finish\","
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"},{\"name\":\"Score\",\"type\":\"Float\"}]},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000001\",\"pin\":\"Out\"},"
					"\"target\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000002\",\"pin\":\"In\"}},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000001\",\"pin\":\"Score\"},"
					"\"target\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000002\",\"pin\":\"Score\"}}"
					"]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestConnectedPinType"),
					TestImportText,
					ErrorMessage
				);

				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset) { AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage)); return; }

				FGuid StartGuid; FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), StartGuid);
				UFlowNode* StartNode = TestFlowAsset->GetNode(StartGuid);
				TestNotNull("Start node must exist", StartNode);
				if (!StartNode) { return; }

				bool bScorePinFound = false;
				FName ScoreTypeName;
				for (const FFlowPin& Pin : StartNode->GetOutputPins())
				{
					if (Pin.PinName == FName("Score"))
					{
						bScorePinFound = true;
						ScoreTypeName = Pin.GetPinTypeName().Name;
						break;
					}
				}
				TestTrue("Start.Score output pin must exist", bScorePinFound);
				TestEqual("Start.Score pin type must be Float (not wildcard)", ScoreTypeName, FName("Float"));
			});

			// A declared exec pin is created even when the node CDO does not declare it.
			It("declared exec pin absent from the node's own CDO is reconstructed on import", [this]()
			{
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"inputPins\":[{\"name\":\"Trigger\",\"type\":\"Exec\"}],"
					"\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]}]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestDeclaredExecPinRecon"),
					TestImportText,
					ErrorMessage
				);

				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset) { AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage)); return; }

				FGuid StartGuid; FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), StartGuid);
				UFlowNode* StartNode = TestFlowAsset->GetNode(StartGuid);
				TestNotNull("Start node must exist", StartNode);
				if (!StartNode) { return; }

				bool bTriggerFound = false;
				FName TriggerTypeName;
				for (const FFlowPin& Pin : StartNode->GetInputPins())
				{
					if (Pin.PinName == FName("Trigger"))
					{
						bTriggerFound = true;
						TriggerTypeName = Pin.GetPinTypeName().Name;
						break;
					}
				}
				TestTrue("Start.Trigger input pin must be present even though the CDO never declares it", bTriggerFound);
				TestEqual("Start.Trigger pin type must be Exec", TriggerTypeName, FName("Exec"));
			});

			// Pure whitespace is a valid pin name and must survive connection parsing.
			It("a connection to/from a pure-whitespace pin name round-trips instead of collapsing to None", [this]()
			{
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_Start\","
					"\"outputPins\":[{\"name\":\" \",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000002\",\"type\":\"/Script/Flow.FlowNode_Finish\","
					"\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000001\",\"pin\":\" \"},"
					"\"target\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000002\",\"pin\":\"In\"}}"
					"]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestBlankPinNameRoundTrip"),
					TestImportText,
					ErrorMessage
				);

				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset) { AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage)); return; }

				FGuid StartGuid; FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), StartGuid);
				FGuid FinishGuid; FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000002"), FinishGuid);
				UFlowNode* StartNode = TestFlowAsset->GetNode(StartGuid);
				TestNotNull("Start node must exist", StartNode);
				if (!StartNode) { return; }

				bool bBlankPinFound = false;
				for (const FFlowPin& Pin : StartNode->GetOutputPins())
				{
					if (Pin.PinName == FName(TEXT(" ")))
					{
						bBlankPinFound = true;
						break;
					}
				}
				TestTrue("Start's single-space-named output pin must exist (not collapsed to None)", bBlankPinFound);

				FConnectedPin Connection = StartNode->GetConnection(FName(TEXT(" ")));
				TestEqual("The blank-named pin's connection must target the Finish node", Connection.NodeGuid, FinishGuid);
				TestEqual("The blank-named pin's connection must target Finish.In", Connection.PinName, FName("In"));
			});
		});

		// Subgraph interface names survive import.
		Describe("Subgraph interface round-trip", [this]()
		{
			It("CustomInputs and CustomOutputs are populated after importing a subgraph asset", [this]()
			{
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":["
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_CustomInput\","
					"\"properties\":{\"EventName\":\"Cancel\"},\"outputPins\":[{\"name\":\"Out\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000002\",\"type\":\"/Script/Flow.FlowNode_CustomOutput\","
					"\"properties\":{\"EventName\":\"Done\"},\"inputPins\":[{\"name\":\"In\",\"type\":\"Exec\"}]},"
					"{\"kind\":\"AddConnection\",\"source\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000001\",\"pin\":\"Out\"},"
					"\"target\":{\"nodeGuid\":\"00000000-0000-0000-0000-000000000002\",\"pin\":\"In\"}}"
					"]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestSubgraphInterface"),
					TestImportText,
					ErrorMessage
				);

				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset) { AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage)); return; }

#if WITH_EDITOR
				const TArray<FName>& Inputs  = TestFlowAsset->GetCustomInputs();
				const TArray<FName>& Outputs = TestFlowAsset->GetCustomOutputs();

				TestEqual("CustomInputs should have one entry", Inputs.Num(), 1);
				if (Inputs.Num() == 1) { TestEqual("CustomInput name should be Cancel", Inputs[0], FName("Cancel")); }

				TestEqual("CustomOutputs should have one entry", Outputs.Num(), 1);
				if (Outputs.Num() == 1) { TestEqual("CustomOutput name should be Done", Outputs[0], FName("Done")); }
#endif
			});
		});

		// DefaultForInputFlowPin values are reflected node properties.
		Describe("DefaultForInputFlowPin value round-trip", [this]()
		{
			It("FText DefaultForInputFlowPin property value survives import via Properties block", [this]()
			{
				// FlowNode_FormatText.FormatText is UPROPERTY(meta=(DefaultForInputFlowPin, FlowPinType=Text)).
				// Its value is just a reflected FText property, so it must round-trip through Properties:
				// exactly like any other node property - no special handling needed.
				TestImportText = TEXT(
					"{\"formatVersion\":2,\"mode\":\"Full\",\"assetClass\":\"/Script/Flow.FlowAsset\",\"bWorldBound\":true,"
					"\"ops\":[{\"kind\":\"UpsertNode\",\"guid\":\"00000000-0000-0000-0000-000000000001\",\"type\":\"/Script/Flow.FlowNode_FormatText\","
					"\"properties\":{\"FormatText\":\"Hello {Name}\"},"
					"\"outputPins\":[{\"name\":\"Formatted Text\",\"type\":\"Text\"}]}]}"
				);

				FString ErrorMessage;
				TestFlowAsset = ImportFromExportedJson(
					TEXT("/Game/Test/TestDefaultForInputFlowPin"),
					TestImportText,
					ErrorMessage
				);

				TestNotNull("Imported asset should not be null", TestFlowAsset);
				if (!TestFlowAsset) { AddError(FString::Printf(TEXT("Import failed: %s"), *ErrorMessage)); return; }

				FGuid NodeGuid; FGuid::Parse(TEXT("00000000-0000-0000-0000-000000000001"), NodeGuid);
				UFlowNode* FormatNode = TestFlowAsset->GetNode(NodeGuid);
				TestNotNull("FormatText node must exist", FormatNode);
				if (!FormatNode) { return; }

				// The FormatText property is private on UFlowNode_FormatText, so verify via reflection.
				FProperty* FormatTextProp = FindFProperty<FProperty>(FormatNode->GetClass(), TEXT("FormatText"));
				TestNotNull("FormatText property must be found via reflection", FormatTextProp);
				if (!FormatTextProp) { return; }

				FString ExportedValue;
				FormatTextProp->ExportTextItem_Direct(ExportedValue, FormatTextProp->ContainerPtrToValuePtr<void>(FormatNode), nullptr, nullptr, PPF_None);
				TestTrue("FormatText value should contain 'Hello'", ExportedValue.Contains(TEXT("Hello")));
				TestTrue("FormatText value should contain '{Name}'", ExportedValue.Contains(TEXT("{Name}")));
			});
		});

		// Comment round-trip is covered by the Courier format and search fixtures, which read
		// UEdGraphNode::NodeComment after regraphing a commented node.
	});
}

#endif
